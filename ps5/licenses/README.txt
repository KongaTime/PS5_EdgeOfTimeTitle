Spider-Man Edge Of Time - legal use
===================================

PIRACY IS NOT CONDONED. This title contains no games, no BIOS files, no
console firmware and no decryption keys, and none will ever be provided or
linked to. Use only games you own and files from hardware you own.
Downloading or sharing games, BIOS files, firmware or keys you do not own is
piracy: do not do it, and do not ask for help with it.

Licences: the title's own code and Sascha Willems' Vulkan examples are MIT.
It links the PS5 platform layer, which is GPL-3.0-or-later, so the
platform-covered combined work is distributed under GPL-3.0-or-later.
The older component/source inventory below is incomplete; see
../../RELEASE-SOURCES.md for the required rebuild and source review. Entries are listed
in licenses/README.txt; the assets and theirs in assets/NOTICES.txt.

Not affiliated with or endorsed by Sony Interactive Entertainment.
"PlayStation" and "PS5" are trademarks of Sony Interactive Entertainment Inc.
Vulkan is a registered trademark of the Khronos Group Inc.; the RADV port is
not a conformant product (its conformanceVersion is 0.0.0.0).


This file and components.json are the earlier v1.0.0 inventory. Additional BSD
and LLVM licence texts have since been added to this repository. Rebuild the
title to regenerate the full inventory; see ../../RELEASE-SOURCES.md. Do not
infer missing dependency build revisions from the licence-text source versions.

The parts of this title
=======================

Spider-Man Edge Of Time: its program, the foundation (Sascha Willems' Vulkan examples and base class, the PS5 layer) and the libraries in its tree (Dear ImGui, libktx, tinygltf with stb_image and nlohmann json, volk, the Vulkan headers, glm)
  licence:   MIT (the title, the examples, Dear ImGui, tinygltf, volk, glm); Apache-2.0 (libktx); Apache-2.0 OR MIT (the Vulkan headers)
  copyright: Copyright (c) 2016-2026 Sascha Willems; Copyright (C) 2026 Mihawk; and each library's authors
  in:        eboot.bin, shaders/
  texts:     licenses/title/
  source:    https://github.com/KongaTime/PS5_EdgeOfTimeTitle at fac446190626a7435a2727f93e5a172e33b291ec

the PS5 platform layer (libps5platform.a) and the SDK headers, from my payload SDK fork, with musl's regex and dlmalloc
  licence:   GPL-3.0-or-later; musl's regex MIT; dlmalloc public domain
  copyright: Copyright (C) John Törnblom and the ps5-payload-dev contributors; Copyright (C) 2026 Mihawk
  in:        eboot.bin
  texts:     licenses/platform/
  source:    https://github.com/mihawk-99/PS5_PayloadSDK at 1de8b37d583de476ad07cde9a0d562b75fb1be8f

PS5_Vulkan: the RADV link recipe, the C runtime start-up, the AGC import stubs and sce_module/libc.prx
  licence:   GPL-3.0-or-later
  copyright: Copyright (C) 2026 Mihawk
  in:        eboot.bin, sce_module/libc.prx
  texts:     licenses/ps5-vulkan/
  source:    https://github.com/KongaTime/ps5_vulkan at 5b5e4fc2d80fdb67a4f61ba9e6a8a424026bb8a5

RADV, Mesa's Vulkan driver (with ACO, NIR and Mesa's Vulkan runtime), from my PS5_Mesa fork
  licence:   MIT, with other licences stated per file (licenses/radv/Mesa-license.rst)
  copyright: Copyright the Mesa contributors (per-file notices in the source)
  in:        eboot.bin
  texts:     licenses/radv/
  source:    https://github.com/mihawk-99/PS5_Mesa at 7b59ef27c1b09b9671bc4153c41940c3155c3af2

LLVM libc++, libc++abi, libunwind and compiler-rt builtins (linked into eboot.bin)
  licence:   Apache-2.0 WITH LLVM-exception
  copyright: Copyright (c) 2003-2026 University of Illinois at Urbana-Champaign and the LLVM project contributors
  in:        eboot.bin
  texts:     licenses/llvm-runtime/
  source:    ps5-payload-dev SDK v0.42 release archives: https://github.com/ps5-payload-dev/sdk/releases/tag/v0.42

The assets: assets/NOTICES.txt, with their licence texts in assets/LICENSES/.
