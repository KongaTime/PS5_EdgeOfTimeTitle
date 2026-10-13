/*
 * EdgeOfTimeRecomp for the PS5 - the shader dump app (REEOT_TITLE_APP=shaderdump).
 *
 * The game's shaders are recompiled ahead of time (XenosRecomp), from the
 * game's packs unpacked. The packs are on the console already, in the game
 * folder: this reads each, a sector at a time, and keeps only the shader
 * containers, in /app0/shader-dump.bin (EdgeOfTimeRecomp's
 * src/gpu/shaders/shader_dump.cpp, which the game build runs as well).
 *
 * The game build does more: a build with no shaders dumps them on its first
 * launch, Default.xex's own with the packs' (reeot_app.cpp), so the first
 * build with shaders draws everything. This app reads the packs only.
 *
 * Copyright (C) 2026 KongaTime
 * SPDX-License-Identifier: MIT
 */

#include "gpu/shaders/shader_dump.h"

#ifndef REEOT_DUMP_GAME_FOLDER
#define REEOT_DUMP_GAME_FOLDER "/app0/gamedata"
#define REEOT_DUMP_OUTPUT "/app0/shader-dump.bin"
#endif

extern "C" void reeot_note(const char *format, ...) __attribute__((format(printf, 1, 2)));

static void NoteLine(const char *line)
{
	reeot_note("%s", line);
}

extern "C" int rex_ps5_app_main(int, char **)
{
	eot::gpu::shader_dump::Totals totals;
	const bool ok = eot::gpu::shader_dump::Run(REEOT_DUMP_GAME_FOLDER, REEOT_DUMP_OUTPUT, {}, NoteLine, &totals);
	return ok ? 0 : (totals.packs ? 2 : 1);
}
