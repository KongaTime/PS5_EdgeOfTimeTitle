# EdgeOfTimeRecomp for the PS5 - the title's program (included by ps5/CMakeLists.txt).
#
# The entry (reeot/title_main.cpp) joins the title's objects and runs in place of
# the samples (PS5_TITLE_MAIN, ps5/src/main.cpp). It runs the ReXGlue app the link
# holds, through the runtime's PS5 entry (rex_ps5_app_main): for now test app 3
# (reeot/rex_test_app.cpp).
#
# The runtime is the ReXGlue fork's (../rexglue-sdk, or REXGLUE_SRC), built for the
# PS5 by EdgeOfTimeRecomp's ps5/tools/build-rexglue.sh (../EdgeOfTimeRecomp, or
# REXGLUE_PS5_BUILD for its build folder): its archives in the fork's
# out/ps5-amd64/, linked whole into the title with the SDK's libc++experimental.
#
# The renderer is EdgeOfTimeRecomp's plume (../EdgeOfTimeRecomp, or
# EDGEOFTIMERECOMP_SRC: its thirdparty/plume, with
# ps5/patches/plume-ps5.patch applied), built here as reeot_plume, apart from the
# samples so it keeps its own headers, and using the title's volk
# (PLUME_NO_VOLK_IMPLEMENTATION).
#
# Copyright (C) 2026 KongaTime
# SPDX-License-Identifier: MIT

set(reeot_rexglue "$ENV{REXGLUE_SRC}")
if(NOT reeot_rexglue)
	set(reeot_rexglue ${ROOT}/../rexglue-sdk)
endif()
get_filename_component(reeot_rexglue "${reeot_rexglue}" ABSOLUTE)
set(reeot_runtime_build "$ENV{REXGLUE_PS5_BUILD}")
if(NOT reeot_runtime_build)
	set(reeot_runtime_build ${ROOT}/../EdgeOfTimeRecomp/build-ps5/rexglue)
endif()
get_filename_component(reeot_runtime_build "${reeot_runtime_build}" ABSOLUTE)
set(reeot_runtime_dir ${reeot_rexglue}/out/ps5-amd64)
set(reeot_eotr "$ENV{EDGEOFTIMERECOMP_SRC}")
if(NOT reeot_eotr)
	set(reeot_eotr ${ROOT}/../EdgeOfTimeRecomp)
endif()
get_filename_component(reeot_eotr "${reeot_eotr}" ABSOLUTE)
set(reeot_plume ${reeot_eotr}/thirdparty/plume)
if(NOT EXISTS ${reeot_plume}/contrib/volk/volk.h)
	message(FATAL_ERROR "No plume with its submodules at ${reeot_plume}: git -C ${reeot_eotr} submodule update "
		"--init thirdparty/plume, then git -C ${reeot_plume} submodule update --init contrib/volk "
		"contrib/Vulkan-Headers contrib/VulkanMemoryAllocator")
endif()
file(READ ${reeot_plume}/plume_vulkan.cpp reeot_plume_source)
if(NOT reeot_plume_source MATCHES "createProsperoDisplaySurface")
	message(FATAL_ERROR "plume at ${reeot_plume} has no PS5 support: git -C ${reeot_plume} apply "
		"${reeot_eotr}/ps5/patches/plume-ps5.patch")
endif()
if(NOT EXISTS ${reeot_runtime_dir}/librexruntime.a OR NOT EXISTS ${reeot_runtime_build}/include/rex/version.h)
	message(FATAL_ERROR "No ReXGlue runtime built for the PS5 at ${reeot_runtime_dir} (build folder "
		"${reeot_runtime_build}): run ps5/tools/build-rexglue.sh in EdgeOfTimeRecomp beside this "
		"repository, or set REXGLUE_SRC and REXGLUE_PS5_BUILD")
endif()
message(STATUS "EdgeOfTimeRecomp: the ReXGlue runtime from ${reeot_runtime_dir}")

# The app the title runs: the game (reeot/reeot_game.cmake), or test app 5
set(REEOT_TITLE_APP game CACHE STRING "What the title runs: game, or test (reeot/rex_test_app.cpp)")
set_property(CACHE REEOT_TITLE_APP PROPERTY STRINGS game test)
if(REEOT_TITLE_APP STREQUAL "test")
	target_sources(samples PRIVATE ${ROOT}/reeot/title_main.cpp ${ROOT}/reeot/rex_test_app.cpp)
else()
	# The game alone: the template's start-up (its platform layer and volk) and
	# reeot/title_entry.cpp, without the Vulkan samples, whose ImGui 1.65 would
	# meet the runtime's ImGui 1.92 in the link
	get_target_property(reeot_samples_sources samples SOURCES)
	list(FILTER reeot_samples_sources INCLUDE REGEX "(^|/)(src/platform\\.c|third_party/volk/volk\\.c)$")
	set_property(TARGET samples PROPERTY SOURCES ${reeot_samples_sources} ${ROOT}/reeot/title_entry.cpp
		${ROOT}/reeot/title_main.cpp)
endif()
message(STATUS "EdgeOfTimeRecomp: the title runs the ${REEOT_TITLE_APP} app")

# plume, and the picture test that drives it
add_library(reeot_plume OBJECT ${reeot_plume}/plume_vulkan.cpp ${ROOT}/reeot/plume_test.cpp)
target_include_directories(reeot_plume PRIVATE ${reeot_plume} ${reeot_plume}/contrib/volk
	${reeot_plume}/contrib/Vulkan-Headers/include ${reeot_plume}/contrib/VulkanMemoryAllocator/include)
target_compile_definitions(reeot_plume PRIVATE PLUME_NO_VOLK_IMPLEMENTATION NDEBUG)
target_compile_options(reeot_plume PRIVATE -std=c++20 -O2 -fno-omit-frame-pointer)
target_compile_definitions(samples PRIVATE PS5_TITLE_MAIN "REEOT_TEST_RESULTS=\"/app0/reeot-test-results.txt\"")

# The app takes the runtime's headers, defines and language settings (its
# rexui objects'); the rest of the title is untouched
set_source_files_properties(${ROOT}/reeot/rex_test_app.cpp PROPERTIES
	INCLUDE_DIRECTORIES "${reeot_rexglue}/include;${reeot_runtime_build}/include;${reeot_rexglue}/thirdparty/fmt/include;${reeot_rexglue}/thirdparty/spdlog/include;${reeot_rexglue}/thirdparty/simde;${reeot_rexglue}/thirdparty/simde/simde;${reeot_rexglue}/thirdparty/utfcpp/source;${reeot_rexglue}/thirdparty/imgui;${reeot_rexglue}/thirdparty/vulkan-headers/include;${reeot_rexglue}/thirdparty/tomlplusplus/include"
	COMPILE_DEFINITIONS "SPDLOG_COMPILED_LIB;SPDLOG_FMT_EXTERNAL;REX_HAS_VULKAN=1"
	COMPILE_OPTIONS "-std=c++23;-fexperimental-library;-fno-char8_t;-fno-strict-aliasing"
	SKIP_PRECOMPILE_HEADERS ON)

# The runtime and its libraries in one group (they refer to each other)
file(GLOB reeot_runtime_archives ${reeot_runtime_dir}/*.a)
list(REMOVE_ITEM reeot_runtime_archives ${reeot_runtime_dir}/librexruntime.a)

# libc names the runtime imports that no module a title loads exports (null at
# run time: link-title.sh refuses them), or that the console refuses a title,
# bound to the platform layer's, as PS5_RPCS3Title's rpcs3/rpcs3.cmake binds
# them: FFmpeg's isatty, libc++'s pathconf (std::filesystem), and realpath, which
# std::filesystem's canonical paths are built on (EPERM to a title), and fork,
# which the game's desktop paths name (a title starts no process)
set(reeot_libc_bindings)
foreach(name isatty pathconf realpath fork)
	list(APPEND reeot_libc_bindings --defsym=${name}=ps5_${name})
endforeach()
# A bound name the SDK's stubs also define would be exported from the title to
# override theirs, and the native tool refuses a title's exports: every name
# bound here stays local, as PS5_Vulkan's recipe keeps its own
set(reeot_local_map ${CMAKE_BINARY_DIR}/reeot-local.map)
set(reeot_local_names)
foreach(flag ${reeot_libc_bindings})
	string(REGEX REPLACE "^--defsym=([^=]+)=.*$" "\\1" name "${flag}")
	string(APPEND reeot_local_names "        ${name};\n")
endforeach()
file(WRITE ${reeot_local_map} "{\n    local:\n${reeot_local_names}};\n")

set(reeot_game_link_inputs)
set(reeot_game_link_depends)
if(NOT REEOT_TITLE_APP STREQUAL "test")
	include(${ROOT}/reeot/reeot_game.cmake)
endif()

set(PS5_TITLE_LINK_INPUTS
	$<TARGET_OBJECTS:reeot_plume> ${reeot_game_link_inputs}
	--start-group ${reeot_runtime_dir}/librexruntime.a ${reeot_runtime_archives}
	${PS5_PAYLOAD_SDK}/target/lib/libc++experimental.a --end-group
	${reeot_libc_bindings} --version-script ${reeot_local_map}
	# abort() through the runtime's fault records (its __wrap_abort)
	--wrap=abort)
set(PS5_TITLE_LINK_DEPENDS ${reeot_runtime_dir}/librexruntime.a ${reeot_runtime_archives} reeot_plume
	${reeot_game_link_depends})
