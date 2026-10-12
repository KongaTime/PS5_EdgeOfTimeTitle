/*
 * EdgeOfTimeRecomp for the PS5 - can the title read its own code?
 *
 * ReXGlue's MMIO handler reads the instruction that faulted, to emulate the
 * access (rexglue-sdk's src/system/mmio_handler.cpp, TryDecodeLoadStore).
 * PS5_RPCS3 found the console maps a title's code execute-only: reading it
 * ended the title at once (SYSTEM_XO_VIOLATION, 0xa0020328), with no signal,
 * so it reads a copy of the code written beside eboot.bin instead. The Simpsons
 * Game's PS5 port, on ReXGlue too, reads the code where it is. This check says
 * which holds for this title, and whether sceKernelMprotect can make code
 * readable, which would make the copy unnecessary:
 *
 * 1. what the kernel says of the title's code and of a system module's code;
 * 2. if the title's code is not readable, sceKernelMprotect read-execute on a
 *    page of code kept for this (one ret, then int3s), and a call into it;
 * 3. a read of that page;
 * 4. sceKernelMprotect read-execute on the whole code segment, as the ReXGlue
 *    fork's ExceptionHandler::Install does, and a read of the title's other code.
 *
 * Every line is in the results file before the step it describes, so a file
 * that ends on "reading ..." means that read ended the title.
 *
 * On the console (2026-10-12, reeot/results/2026-10-12.txt): the code segment
 * (0x400000-0xd3c000) and a system module's both answer 0x4 (execute only);
 * sceKernelMprotect read-execute on the probe page succeeded, the page still
 * ran and read back its real bytes, yet still answered 0x4; reading untouched
 * code ended the title.
 *
 * Copyright (C) 2026 KongaTime
 * SPDX-License-Identifier: MIT
 */

#include <csetjmp>
#include <csignal>
#include <cstdarg>
#include <cstdint>
#include <cstdio>

#include <ps5platform/kernel.h>

#include "platform.h"

extern "C" int ps5_title_main(void);

/* The page the check may change: 16 KiB of its own, a ret and then int3s */
__asm__(".pushsection .text.reeot_code_probe,\"ax\",@progbits\n"
	".balign 16384\n"
	".globl reeot_code_probe\n"
	".type reeot_code_probe,@function\n"
	"reeot_code_probe:\n"
	"\tret\n"
	"\t.fill 16383, 1, 0xcc\n"
	".size reeot_code_probe, 16384\n"
	".popsection\n");
extern "C" void reeot_code_probe(void);

namespace
{
constexpr size_t kProbePage = 16384;

const char *g_results;
sigjmp_buf g_fault_jump;
volatile sig_atomic_t g_fault_signal;

void note(const char *format, ...) __attribute__((format(printf, 1, 2)));
void note(const char *format, ...)
{
	char line[320];
	va_list args;
	va_start(args, format);
	vsnprintf(line, sizeof line, format, args);
	va_end(args);
	say("%s", line);
	if (FILE *file = fopen(g_results, "a")) {
		fprintf(file, "%s\n", line);
		fclose(file);
	}
}

void on_fault(int signal_number)
{
	g_fault_signal = signal_number;
	siglongjmp(g_fault_jump, 1);
}

/* What the kernel says of the memory at code: true when it says readable */
bool query(const char *what, const void *code)
{
	void *start = nullptr;
	void *end = nullptr;
	uint32_t protection = 0;
	const int32_t result = sceKernelQueryMemoryProtection(const_cast<void *>(code), &start, &end, &protection);
	const bool readable = result == 0 && (protection & PS5_KERNEL_PROT_CPU_READ);
	note("code read: %s at %p: query 0x%08x, %p-%p, protection 0x%x (%s)", what, code,
		static_cast<unsigned>(result), start, end, protection,
		result != 0 ? "no answer" : readable ? "readable" : "not readable");
	return readable;
}

/* Read 16 bytes of code; a fault that raises a signal is reported, one that
 * ends the title leaves the "reading" line last in the file */
void read_code(const char *what, const void *code, bool expect_probe)
{
	note("code read: reading 16 bytes of %s (if this is the file's last line, the read ended the title)", what);
	struct sigaction action {}, previous_segv {}, previous_bus {};
	action.sa_handler = on_fault;
	sigemptyset(&action.sa_mask);
	sigaction(SIGSEGV, &action, &previous_segv);
	sigaction(SIGBUS, &action, &previous_bus);
	uint8_t bytes[16] = {};
	g_fault_signal = 0;
	if (sigsetjmp(g_fault_jump, 1) == 0) {
		const volatile uint8_t *p = static_cast<const volatile uint8_t *>(code);
		for (int i = 0; i < 16; i++)
			bytes[i] = p[i];
		bool probe_bytes = bytes[0] == 0xc3;
		for (int i = 1; i < 16; i++)
			probe_bytes = probe_bytes && bytes[i] == 0xcc;
		note("code read: %s is readable: %02x %02x %02x %02x %02x %02x %02x %02x ...%s", what, bytes[0], bytes[1],
			bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7],
			expect_probe ? (probe_bytes ? " (the probe's ret and int3s: the real code)" : " (NOT the probe's bytes)")
				     : "");
	} else {
		note("code read: reading %s raised signal %d", what, static_cast<int>(g_fault_signal));
	}
	sigaction(SIGSEGV, &previous_segv, nullptr);
	sigaction(SIGBUS, &previous_bus, nullptr);
}
} // namespace

extern "C" void reeot_code_read_check(const char *results)
{
	g_results = results;
	note("==== code read check ====");
	const void *probe = reinterpret_cast<const void *>(&reeot_code_probe);
	const bool readable = query("this title's code (the probe page)", probe);
	query("a system module's code (sceKernelQueryMemoryProtection)",
		reinterpret_cast<const void *>(&sceKernelQueryMemoryProtection));

	if (!readable) {
		const int32_t result = sceKernelMprotect(probe, kProbePage,
			PS5_KERNEL_PROT_CPU_READ | PS5_KERNEL_PROT_CPU_EXEC);
		note("code read: sceKernelMprotect(probe page, read+execute): 0x%08x", static_cast<unsigned>(result));
		if (result == 0) {
			query("the probe page after sceKernelMprotect", probe);
			note("code read: calling the probe page (if this is the file's last line, it no longer runs)");
			reeot_code_probe();
			note("code read: the probe page still runs");
		}
	}
	read_code("the probe page", probe, true);

	/* What the fork does: the whole segment, from the bounds the kernel gives */
	void *start = nullptr;
	void *end = nullptr;
	uint32_t protection = 0;
	const void *main_code = reinterpret_cast<const void *>(&ps5_title_main);
	if (sceKernelQueryMemoryProtection(const_cast<void *>(main_code), &start, &end, &protection) == 0) {
		const int32_t result = sceKernelMprotect(start, static_cast<size_t>(static_cast<char *>(end) -
							static_cast<char *>(start)),
			PS5_KERNEL_PROT_CPU_READ | PS5_KERNEL_PROT_CPU_EXEC);
		note("code read: sceKernelMprotect(the code segment %p-%p, read+execute): 0x%08x", start, end,
			static_cast<unsigned>(result));
	}
	read_code("this title's other code (ps5_title_main), after the segment's sceKernelMprotect", main_code, false);
	note("==== code read check: done ====");
}
