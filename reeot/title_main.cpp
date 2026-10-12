/*
 * EdgeOfTimeRecomp for the PS5 - the title's program.
 *
 * ps5/src/main.cpp calls this in place of the samples once the platform layer
 * is up (klog, the pad, the splash) and volk points at the RADV linked in.
 *
 * For now the program is two checks, run inside a title's process:
 * - the ReXGlue fork's guest memory (rexglue-sdk's src/core/memory_ps5.cpp),
 *   checked by its ps5/tests/memory_ps5_test.cpp: a title's process has the
 *   console's 12 GiB of direct memory where a payload's has next to none;
 * - whether the title can read its own code (reeot/code_read_check.cpp), which
 *   ReXGlue's MMIO handler does.
 * Both write REEOT_TEST_RESULTS (/app0/reeot-test-results.txt, the title's
 * folder, readable over FTP) and klog, and the title ends when they do. The
 * game takes this place once the runtime and the recompiled code are linked in.
 *
 * Copyright (C) 2026 KongaTime
 * SPDX-License-Identifier: MIT
 */

#include "platform.h"

extern "C" int memory_ps5_test_main(void);
extern "C" void reeot_code_read_check(const char *results);

extern "C" int ps5_title_main(void)
{
	say("EdgeOfTimeRecomp: the guest memory test and the code read check; results in %s", REEOT_TEST_RESULTS);
	const int status = memory_ps5_test_main();
	say("EdgeOfTimeRecomp: the guest memory test %s", status == 0 ? "passed" : "failed");
	reeot_code_read_check(REEOT_TEST_RESULTS);
	return status;
}
