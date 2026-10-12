/*
 * EdgeOfTimeRecomp for the PS5 - the shader dump app (REEOT_TITLE_APP=shaderdump).
 *
 * The game's shaders are recompiled ahead of time (XenosRecomp), from the
 * game's packs unpacked. The packs are on the console already, in the game
 * folder, so this reads each .pkz there, inflates it a sector at a time, and
 * keeps only the shader containers it finds, the way XenosRecomp's own scan
 * finds them (main.cpp): a container header on a 4-byte step from the start of
 * the unpacked pack, flags 0x102A11xx, words 0x1C and 0x20 zero, and its
 * virtual and physical sizes inside the pack. Each distinct container goes to
 * /app0/shader-dump.bin, 16 zero bytes after each, which XenosRecomp reads as
 * it would the packs: the same bytes, so the same hashes the game looks up.
 *
 * Copyright (C) 2026 KongaTime
 * SPDX-License-Identifier: MIT
 */

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_set>
#include <vector>

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <BaseUtils/BUDecompress.h>

extern "C" void reeot_note(const char *format, ...) __attribute__((format(printf, 1, 2)));

namespace {

#ifndef REEOT_DUMP_GAME_FOLDER
#define REEOT_DUMP_GAME_FOLDER "/app0/gamedata"
#define REEOT_DUMP_OUTPUT "/app0/shader-dump.bin"
#endif
constexpr const char *kGameFolder = REEOT_DUMP_GAME_FOLDER;
constexpr const char *kOutput = REEOT_DUMP_OUTPUT;
constexpr size_t kHeader = 36;  // sizeof(XenosRecomp's ShaderContainer)
// A real container is a few KiB; a larger "container" is pack data that
// happens to look like a header
constexpr size_t kLargest = 8u << 20;

uint32_t ReadBE32(const uint8_t *p)
{
	return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | p[3];
}

uint64_t Fnv1a(const uint8_t *p, size_t n)
{
	uint64_t h = 0xCBF29CE484222325ull;
	for (size_t i = 0; i < n; ++i)
		h = (h ^ p[i]) * 0x100000001B3ull;
	return h ^ n;
}

struct Dump {
	FILE *out = nullptr;
	std::unordered_set<uint64_t> seen;
	size_t containers = 0;  // distinct, written
	size_t found = 0;       // all, duplicates too
	uint64_t bytes = 0;

	void Emit(const uint8_t *data, size_t size)
	{
		++found;
		if (!seen.insert(Fnv1a(data, size)).second)
			return;
		static const uint8_t kPad[16] = {};
		fwrite(data, 1, size, out);
		fwrite(kPad, 1, sizeof(kPad), out);
		++containers;
		bytes += size + sizeof(kPad);
	}
};

// XenosRecomp's scan over one unpacked pack, fed in pieces
class Scanner {
public:
	Scanner(Dump &dump, uint64_t total) : dump_(dump), total_(total) {}

	void Feed(const uint8_t *data, size_t size)
	{
		buffer_.insert(buffer_.end(), data, data + size);
		Scan();
		// Keep only what the scan has not passed
		const uint64_t drop = at_ - base_;
		if (drop) {
			buffer_.erase(buffer_.begin(), buffer_.begin() + std::min<uint64_t>(drop, buffer_.size()));
			base_ += drop;
		}
	}

private:
	void Scan()
	{
		const uint64_t end = base_ + buffer_.size();
		while (total_ > kHeader && at_ < total_ - kHeader - 1) {
			if (at_ + kHeader > end)
				return;
			const uint8_t *header = buffer_.data() + (at_ - base_);
			const uint32_t flags = ReadBE32(header);
			const uint64_t size = uint64_t(ReadBE32(header + 4)) + ReadBE32(header + 8);
			if ((flags & 0xFFFFFF00u) == 0x102A1100u && size <= total_ - at_ && ReadBE32(header + 0x1C) == 0 &&
			    ReadBE32(header + 0x20) == 0 && size >= kHeader && size <= kLargest) {
				if (at_ + size > end)
					return;  // the rest of it comes with the next piece
				dump_.Emit(header, size_t(size));
				at_ += size;
			} else {
				at_ += 4;
			}
		}
	}

	Dump &dump_;
	uint64_t total_;
	std::vector<uint8_t> buffer_;
	uint64_t base_ = 0;  // the pack offset of buffer_[0]
	uint64_t at_ = 0;    // the scan's pack offset
};

bool ReadAt(int fd, uint64_t offset, uint8_t *data, size_t size)
{
	while (size) {
		const ssize_t got = pread(fd, data, size, off_t(offset));
		if (got <= 0)
			return false;
		data += got;
		size -= size_t(got);
		offset += uint64_t(got);
	}
	return true;
}

// A BUCompress container (.pkz): the header and its tables, then one zlib
// stream per sector, each a sector long in the file
bool ScanCompressed(Dump &dump, int fd, uint64_t file_size, const char *path)
{
	std::vector<uint8_t> header(64);
	if (!ReadAt(fd, 0, header.data(), header.size()))
		return false;
	const bool big32 = ReadBE32(header.data()) == BUDecompress::kMagic;
	const uint32_t header_size = big32 ? ReadBE32(header.data() + 8)
	                                   : (uint32_t(header[8]) | uint32_t(header[9]) << 8 |
	                                      uint32_t(header[10]) << 16 | uint32_t(header[11]) << 24);
	if (header_size < 28 || header_size > file_size || header_size > (64u << 20)) {
		reeot_note("dump: %s: bad header size %u", path, header_size);
		return false;
	}
	header.resize(header_size);
	if (!ReadAt(fd, 0, header.data(), header.size()))
		return false;
	BUDecompress d;
	try {
		d.ParseHeader(header.data(), header.size());
	} catch (const std::exception &e) {
		reeot_note("dump: %s: %s", path, e.what());
		return false;
	}
	const uint32_t sector_size = d.mCompHeader.uiSectorSize;
	Scanner scanner(dump, d.mCompHeader.uiUncompFileSize);
	std::vector<uint8_t> in(sector_size);
	std::vector<uint8_t> out(std::max<uint32_t>(d.mCompHeader.uiBigChunkSize, sector_size * 8));
	for (uint32_t sector = 0; sector < d.GetNbSectors(); ++sector) {
		const uint64_t begin = uint64_t(header_size) + uint64_t(sector) * sector_size;
		const size_t avail = size_t(std::min<uint64_t>(sector_size, file_size - begin));
		const uint64_t want = d.SectorEnd(sector) - d.SectorStart(sector);
		if (begin >= file_size || want > out.size() || !ReadAt(fd, begin, in.data(), avail)) {
			reeot_note("dump: %s: sector %u unreadable", path, sector);
			return false;
		}
		z_stream strm{};
		if (inflateInit(&strm) != Z_OK)
			return false;
		strm.next_in = in.data();
		strm.avail_in = uInt(avail);
		strm.next_out = out.data();
		strm.avail_out = uInt(out.size());
		int rc = Z_OK;
		while (rc == Z_OK && strm.avail_in && strm.avail_out)
			rc = inflate(&strm, Z_NO_FLUSH);
		const uint64_t produced = strm.total_out;
		inflateEnd(&strm);
		if (produced < want) {
			reeot_note("dump: %s: sector %u inflated to %llu of %llu bytes (zlib %d)", path, sector,
			           (unsigned long long)produced, (unsigned long long)want, rc);
			return false;
		}
		scanner.Feed(out.data(), size_t(want));
	}
	return true;
}

bool ScanRaw(Dump &dump, int fd, uint64_t file_size)
{
	Scanner scanner(dump, file_size);
	std::vector<uint8_t> chunk(1u << 20);
	for (uint64_t at = 0; at < file_size;) {
		const size_t n = size_t(std::min<uint64_t>(chunk.size(), file_size - at));
		if (!ReadAt(fd, at, chunk.data(), n))
			return false;
		scanner.Feed(chunk.data(), n);
		at += n;
	}
	return true;
}

bool EndsWith(const std::string &s, const char *suffix)
{
	const size_t n = strlen(suffix);
	if (s.size() < n)
		return false;
	for (size_t i = 0; i < n; ++i)
		if (tolower(static_cast<unsigned char>(s[s.size() - n + i])) != suffix[i])
			return false;
	return true;
}

void ListPacks(const std::string &folder, std::vector<std::string> &packs)
{
	DIR *dir = opendir(folder.c_str());
	if (!dir)
		return;
	while (dirent *entry = readdir(dir)) {
		if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
			continue;
		const std::string path = folder + "/" + entry->d_name;
		struct stat st;
		if (stat(path.c_str(), &st) != 0)
			continue;
		if (S_ISDIR(st.st_mode))
			ListPacks(path, packs);
		else if (EndsWith(path, ".pkz") || EndsWith(path, ".pak"))
			packs.push_back(path);
	}
	closedir(dir);
}

}  // namespace

extern "C" int rex_ps5_app_main(int, char **)
{
	reeot_note("dump: looking for the game's packs in %s", kGameFolder);
	std::vector<std::string> packs;
	ListPacks(kGameFolder, packs);
	if (packs.empty()) {
		reeot_note("dump: FAILED: no .pkz or .pak files in %s", kGameFolder);
		return 1;
	}
	reeot_note("dump: %zu packs", packs.size());

	Dump dump;
	dump.out = fopen(kOutput, "wb");
	if (!dump.out) {
		reeot_note("dump: FAILED: cannot write %s", kOutput);
		return 1;
	}
	size_t failed = 0;
	for (size_t i = 0; i < packs.size(); ++i) {
		const std::string &path = packs[i];
		const int fd = open(path.c_str(), O_RDONLY);
		struct stat st;
		if (fd < 0 || fstat(fd, &st) != 0) {
			reeot_note("dump: %s: cannot open", path.c_str());
			if (fd >= 0)
				close(fd);
			++failed;
			continue;
		}
		const uint64_t size = uint64_t(st.st_size);
		uint8_t magic[64] = {};
		const bool read = ReadAt(fd, 0, magic, size_t(std::min<uint64_t>(sizeof(magic), size)));
		const size_t before = dump.containers;
		bool ok = false;
		if (read && BUDecompress::IsCompressed(magic, size_t(std::min<uint64_t>(sizeof(magic), size))))
			ok = ScanCompressed(dump, fd, size, path.c_str());
		else if (read)
			ok = ScanRaw(dump, fd, size);
		close(fd);
		if (!ok)
			++failed;
		reeot_note("dump: [%zu/%zu] %s (%llu MB): %zu new shaders, %zu in all%s", i + 1, packs.size(),
		           path.c_str() + strlen(kGameFolder) + 1, (unsigned long long)(size >> 20),
		           dump.containers - before, dump.containers, ok ? "" : " - FAILED");
		fflush(dump.out);
	}
	fclose(dump.out);
	chmod(kOutput, 0666);
	reeot_note("dump: DONE: %zu shaders (%zu found with repeats), %llu KB in %s; %zu pack(s) failed",
	           dump.containers, dump.found, (unsigned long long)(dump.bytes >> 10), kOutput, failed);
	return failed ? 2 : 0;
}
