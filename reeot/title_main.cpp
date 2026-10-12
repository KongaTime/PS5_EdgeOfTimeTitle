/*
 * EdgeOfTimeRecomp for the PS5 - the title's program.
 *
 * ps5/src/main.cpp calls this in place of the samples once the platform layer
 * is up (klog, the pad, the splash) and volk points at the RADV linked in.
 *
 * It runs the ReXGlue app the runtime holds (REX_DEFINE_APP) through the
 * fork's PS5 entry, rex_ps5_app_main, on this thread. For now that app is test
 * app 3 (reeot/rex_test_app.cpp); the game takes its place once the recompiled
 * code is linked in.
 *
 * The title gives the runtime its startup log (prospero_startup_log, which the
 * fork defines weakly): each marker is a line of REEOT_TEST_RESULTS
 * (/app0/reeot-test-results.txt, the title's folder, readable over FTP) and of
 * klog, written before the step it names, so a run that stops says where.
 *
 * Copyright (C) 2026 KongaTime
 * SPDX-License-Identifier: MIT
 */

#include <cstdarg>
#include <cstdio>

#include <sys/stat.h>

#include "platform.h"

extern "C" int rex_ps5_app_main(int argc, char **argv);

extern "C" void reeot_note(const char *format, ...) __attribute__((format(printf, 1, 2)));
extern "C" void reeot_note(const char *format, ...)
{
	char line[512];
	va_list args;
	va_start(args, format);
	vsnprintf(line, sizeof line, format, args);
	va_end(args);
	say("%s", line);
	if (FILE *file = fopen(REEOT_TEST_RESULTS, "a")) {
		fprintf(file, "%s\n", line);
		fclose(file);
	}
}

extern "C" void prospero_startup_log(const char *message)
{
	reeot_note("startup: %s", message ? message : "(null)");
}

extern "C" int ps5_title_main(void)
{
	// A fresh results file each run, readable and writable over FTP
	if (FILE *file = fopen(REEOT_TEST_RESULTS, "w")) {
		fclose(file);
		chmod(REEOT_TEST_RESULTS, 0666);
	}
	char program[] = "/app0/eboot.bin";
	char *argv[] = {program, nullptr};
	const int status = rex_ps5_app_main(1, argv);
	reeot_note("rex_ps5_app_main returned %d", status);
	return status;
}
