# EdgeOfTimeRecomp for the PS5 - the title's program (included by ps5/CMakeLists.txt).
#
# The entry (reeot/title_main.cpp) joins the title's objects and runs in place of
# the samples (PS5_TITLE_MAIN, ps5/src/main.cpp). For now it runs the ReXGlue
# fork's guest-memory test: src/core/memory_ps5.cpp and ps5/tests/
# memory_ps5_test.cpp, compiled from the fork's checkout beside this repository
# (../rexglue-sdk, the same one EdgeOfTimeRecomp builds with, or REXGLUE_SRC).
# Both link against the platform layer the title already links.
#
# Copyright (C) 2026 KongaTime
# SPDX-License-Identifier: MIT

set(reeot_rexglue "$ENV{REXGLUE_SRC}")
if(NOT reeot_rexglue)
	set(reeot_rexglue ${ROOT}/../rexglue-sdk)
endif()
get_filename_component(reeot_rexglue "${reeot_rexglue}" ABSOLUTE)
if(NOT EXISTS ${reeot_rexglue}/src/core/memory_ps5.cpp)
	message(FATAL_ERROR "No ReXGlue fork with the PS5 target at ${reeot_rexglue}: "
		"git clone https://github.com/KongaTime/rexglue-sdk beside this repository, or set REXGLUE_SRC")
endif()
if(NOT EXISTS ${reeot_rexglue}/thirdparty/fmt/include/fmt/format.h OR
   NOT EXISTS ${reeot_rexglue}/thirdparty/simde/simde)
	message(FATAL_ERROR "The ReXGlue fork's fmt and simde submodules are not checked out: "
		"git -C ${reeot_rexglue} submodule update --init --depth 1 thirdparty/fmt thirdparty/simde")
endif()
message(STATUS "EdgeOfTimeRecomp: the guest memory test from ${reeot_rexglue}")

target_sources(samples PRIVATE ${ROOT}/reeot/title_main.cpp)
target_compile_definitions(samples PRIVATE PS5_TITLE_MAIN)

# The fork's sources take its own headers (the test's logging stand-in first) and
# C++23; the rest of the title is untouched
set(reeot_memory_sources
	${reeot_rexglue}/src/core/memory_ps5.cpp
	${reeot_rexglue}/ps5/tests/memory_ps5_test.cpp)
target_sources(samples PRIVATE ${reeot_memory_sources})
set_source_files_properties(${reeot_memory_sources} PROPERTIES
	INCLUDE_DIRECTORIES "${reeot_rexglue}/ps5/tests/stub;${reeot_rexglue}/include;${reeot_rexglue}/thirdparty/fmt/include;${reeot_rexglue}/thirdparty/simde"
	COMPILE_DEFINITIONS "MEMORY_PS5_TEST_EMBEDDED;FMT_HEADER_ONLY"
	COMPILE_OPTIONS "-std=c++23"
	SKIP_PRECOMPILE_HEADERS ON)
