/*
 * EdgeOfTimeRecomp for the PS5 - the title's program.
 *
 * ps5/src/main.cpp calls this in place of the samples once the platform layer
 * is up (klog, the pad, the splash) and volk points at the RADV linked in.
 *
 * For now the program is one test: the ReXGlue fork's guest memory
 * (rexglue-sdk's src/core/memory_ps5.cpp), checked by its ps5/tests/
 * memory_ps5_test.cpp inside a title's process, which has the console's
 * 12 GiB of direct memory where a payload's has next to none. The results go to
 * /app0/rexglue-memory-test.txt (the title's folder, readable over FTP) and to
 * klog, and the title ends when the test does. The game takes this place once
 * the runtime and the recompiled code are linked in.
 *
 * Copyright (C) 2026 KongaTime
 * SPDX-License-Identifier: MIT
 */

#include "platform.h"

extern "C" int memory_ps5_test_main(void);

extern "C" int ps5_title_main(void)
{
	say("EdgeOfTimeRecomp: the guest memory test; results in /app0/rexglue-memory-test.txt");
	const int status = memory_ps5_test_main();
	say("EdgeOfTimeRecomp: the guest memory test %s", status == 0 ? "passed" : "failed");
	return status;
}
