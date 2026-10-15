# Preparing release notices and source

The root MIT licence covers the title's own MIT-licensed sources and its
foundation. It does not replace third-party licences or grant rights to the
original commercial game's code, shaders, data or artwork. The combined build
also links GPL-covered platform code. The source inventory and licence texts
must describe the actual release; GitHub detecting MIT alone is not an audit
of the complete binary's licensing.

## Build a new release

1. Commit the title's changes and rebuild the dependencies and title from the
   source revisions you intend to publish. Keep the sibling checkouts used by
   the build. If plume has the PS5 patch applied, commit those modifications in
   your public fork before packaging so the recorded source includes them.
2. Build the runtime with `EdgeOfTimeRecomp/ps5/tools/build-rexglue.sh`, then
   build this title with `ps5/tools/build.sh`. CMake records the configured
   ReXGlue and EdgeOfTimeRecomp paths in the build directory. An existing build
   will reconfigure when its CMake files change.
3. Check `dist/PPSA99300/licenses/README.txt` and `components.json`. They must
   include ReXGlue, EdgeOfTimeRecomp, their initialized submodules, the platform,
   Vulkan, RADV and LLVM. Each referenced licence directory must contain text.
   The tool copies upstream notices verbatim. Uninitialized optional desktop
   submodules are not inventoried; confirm every dependency actually linked on
   PS5 has been initialized and is included.
4. Run `python3 ps5/tools/package-release.py v1.0.1` (choose the actual new
   version). It checks notice directories and refuses recorded dirty source
   trees, including submodules. It creates the ZIP, source archives,
   `source/SOURCES.txt` and checksums under `dist/release-v1.0.1/`.
5. Review the source archives and generated material before publishing. The
   archives contain committed repository source, including nested submodules;
   `git archive` does not include ignored generated game code. This inventory
   alone therefore does not establish complete corresponding-source coverage
   for a game-derived binary, or permission to redistribute it. Resolve those
   questions for the actual release separately. Do not upload original game
   files or assume an open-source runtime licence covers game-derived material.
6. Publish the reviewed source archives, `SOURCES.txt` and checksums alongside
   the ZIP, with clear build instructions and source locations. Increase
   `contentVersion` before the build and use a new release tag and ZIP. Update
   the catalog version, URL and SHA-256 for that new asset.

These tools inspect files and recorded checkout revisions. They do not prove
that previously compiled archives match the current checkout: rebuild the
runtime after changing its source and preserve its build records.

## Existing v1.0.0

The checked-in `ps5/licenses/components.json` is the earlier release inventory.
It does not record ReXGlue or EdgeOfTimeRecomp build revisions. The additional
BSD licence files in `ps5/licenses/` document upstream notices, not proof that
those latest revisions were used in the existing binary. Regenerate the
inventory from a fresh build rather than filling in guessed revisions.

Adding files to this repository does not update the published ZIP. Do not claim
the current v1.0.0 package has been repaired until the actual release files have
been rebuilt or corrected and verified. Keep already cataloged assets unchanged;
publish a new version instead.

## Licence text provenance

- `ps5/licenses/rexglue/LICENSE`: unmodified from
  `KongaTime/rexglue-sdk` commit `d3ddb0b859679eebff7f7b9a44768cbb76685091`.
- `ps5/licenses/edgeoftime/LICENSE`: unmodified from
  `KongaTime/EdgeOfTimeRecomp` commit `744d009424bb7066cdee6dfd86a73460818327cd`.
- `ps5/licenses/llvm-runtime/LICENSE.TXT`: unmodified from LLVM's
  `llvmorg-18.1.0` tag, `llvm/LICENSE.TXT`. The SDK v0.42 archive's libc++
  `__config` reports `_LIBCPP_VERSION 180100`. If the runtime/toolchain changes,
  update the matching notice too. A local `PS5_LLVM/llvm/LICENSE.TXT` takes
  precedence when that source checkout is present.
