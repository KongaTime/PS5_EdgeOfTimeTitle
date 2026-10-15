# Spider-Man: Edge of Time on the PS5

An unofficial native port of *Spider-Man: Edge of Time* (Xbox 360, Beenox, 2011) to
jailbroken PS5 consoles, as a homebrew title (`PPSA99300`). The game's PowerPC code is
recompiled ahead of time to native x86-64 code with the ReXGlue SDK, and its graphics
run on the PS5's GPU through Vulkan (RADV). It isn't an emulator. As far as we know,
this is the first port of Edge of Time to the PS5.

It is built on [EdgeOfTimeRecompiled](https://github.com/goliathret/EdgeOfTimeRecomp),
the static recompilation of the game for PC. This project adds the PS5: the ReXGlue
runtime for the console, the title that runs it, and the tools to make the game's
shaders on the console. See [Credits](#credits).

**No game files are included.** You need your own copy of the game. Download the
release, add your game files, copy it to the PS5 and play: see
[Install the download](#install-the-download). Building it yourself is optional.

## Status

- **Playable from beginning to end.** The game has been played through on one PS5 with
  sound, the DualSense and full graphics. The last session ran 3 hours 40 minutes,
  through the ending, without a crash.
- Every shader that playthrough used was in the shader cache, made from the game's
  packs and `Default.xex` as below. Nothing was missing.
- The first minute or two can stutter while the PS5 builds each shader the first time
  it is used.

## What you need

**The console:** a jailbroken PS5 with:
- [etaHEN](https://github.com/etaHEN/etaHEN);
- [ShadowMountPlus](https://github.com/drakmor/ShadowMountPlus), which puts
  `/data/homebrew` titles on the home screen;
- an FTP server, such as [ftpsrv](https://github.com/ps5-payload-dev/ftpsrv) on port
  2121.

**The game:** your own copy of the Xbox 360 game, with its Title Update. Either:
- your disc image (`.iso`) and the Title Update package, as they are: the title
  installs from them the first time it runs; or
- the disc's files, extracted from your disc image (for example with
  [extract-xiso](https://github.com/XboxDev/extract-xiso)), with `Default.xexp` and
  `Data/GameLogic.dllp` from the Title Update.

A game folder from a PC install with
[EdgeOfTimeRecompiled](https://github.com/goliathret/EdgeOfTimeRecomp)'s installer works
too: it holds the same files.

The port's own packages (its menus, achievements, button icons and suits) aren't in the
download, because three of them are made partly from the game's own files. The first
time it runs, the title makes them from your game files, in `gamedata/Data/custom/`.
That takes a few seconds, once per update.

## Install the download

The v1.0.0 release asset is named `Spider.Man.Edge.Of.Time.zip`.

1. Download the zip under **Assets** on the
   [latest release](https://github.com/KongaTime/PS5_EdgeOfTimeTitle/releases/latest)
   and unzip it. It holds a `PPSA99300` folder.
2. Copy your disc image (`.iso`) and the Title Update package into
   `PPSA99300/gamedata/`. The first launch installs the game from them: it shows its
   progress in notifications and takes a few minutes. You can delete both files
   afterwards.

   Or copy the disc's files into `PPSA99300/gamedata/`, then the Title Update's
   `Default.xexp` beside `Default.xex` and its `GameLogic.dllp` into `Data/`, so that
   `Default.xex` is at `PPSA99300/gamedata/Default.xex`. From a PC install, copy
   everything inside its `game` folder instead.
3. Copy the whole `PPSA99300` folder to `/data/homebrew/` on the PS5 over FTP, so that
   you have `/data/homebrew/PPSA99300/eboot.bin`.
4. ShadowMountPlus adds *Spider-Man Edge Of Time* to the home screen about 30 seconds
   after the folder appears. Launch it from there.

The first few minutes can stutter while the PS5 prepares each shader the first time it
is used.

Your saves, settings and logs are in the title's folder, under `profiles/` and `logs/`.
Back up `profiles/` before you update or remove the game. Never delete the title from
the home screen while its folder is still in `/data/homebrew`. To update, replace
`eboot.bin` and `shader-cache.zst` together.

## Build it yourself (optional)

For publishing a build, also follow [release preparation](RELEASE-SOURCES.md)
to collect the matching dependency notices and source inventory.

Only needed to change the code or make your own build.

**A Linux machine to build on** (it was built on Ubuntu; WSL2 may work but is untested),
with:
- clang 20 or later with lld (`llvm-config-20`);
- CMake 3.25+, Ninja, git, wget, unzip, make and zip;
- Python 3 with numpy, plus Pillow and etcpak if you make your own title art;
- `dxc` ([DirectXShaderCompiler](https://github.com/microsoft/DirectXShaderCompiler/releases))
  on the PATH;
- ReXGlue's host tools, `rexglue`, from release
  [`nightly-20261002-bd833a2a`](https://github.com/rexglue/rexglue-sdk/releases/tag/nightly-20261002-bd833a2a)
  (the linux-amd64 zip). Use exactly this version: the PS5 runtime runs only the code
  this version generates.

### 1. Get the code

Check these out side by side in one folder:

```bash
git clone https://github.com/KongaTime/PS5_EdgeOfTimeTitle
git clone https://github.com/KongaTime/rexglue-sdk
git clone https://github.com/KongaTime/EdgeOfTimeRecomp
git clone https://github.com/KongaTime/ps5_vulkan PS5_Vulkan
git clone https://github.com/KongaTime/ps5_mesa PS5_Mesa
git clone https://github.com/KongaTime/ps5_payloadsdk PS5_PayloadSDK

git -C EdgeOfTimeRecomp submodule update --init --recursive
git -C EdgeOfTimeRecomp/thirdparty/plume apply ../../ps5/patches/plume-ps5.patch
```

### 2. Build the PS5 side of the stack (once)

```bash
(cd PS5_Vulkan && tools/setup-native-dependencies.sh && tools/build-radv.sh release)
EdgeOfTimeRecomp/ps5/tools/build-rexglue.sh     # the ReXGlue runtime for the PS5
PS5_EdgeOfTimeTitle/ps5/tools/bootstrap.sh      # checks the host and everything beside it
```

### 3. Recompile the game's code

From your game files (above), copy `Default.xex`, `Default.xexp`,
`Data/GameLogic.dll` and `Data/GameLogic.dllp` into `EdgeOfTimeRecomp/assets/`, keeping
`Data/`. Then:

```bash
cd EdgeOfTimeRecomp && rexglue codegen reeot_manifest.toml && cd ..
```

This writes `EdgeOfTimeRecomp/generated/`. Git ignores it, because it is made from your
game.

### 4. Build it once, and let it find the shaders

```bash
cd PS5_EdgeOfTimeTitle && ps5/tools/build.sh && cd ..
```

This first build has no shaders yet. A build without shaders doesn't run the game:
it finds the shaders instead. It reads them from the game's packs and from inside
`Default.xex`, which the PS5 build can read once the runtime has loaded it.

On the console:

1. Copy the whole `PS5_EdgeOfTimeTitle/dist/PPSA99300/` folder to
   `/data/homebrew/PPSA99300/`.
2. Copy your game files there as `gamedata/`.
3. Launch it from the home screen. ShadowMountPlus adds it about 30 seconds after the
   folder appears.
4. The screen stays black for about a minute, then the app closes by itself.
   `reeot-test-results.txt` then has a `dump: DONE` line (all 235 packs, about 14,400
   shaders, about 31 MB).
5. Copy `/data/homebrew/PPSA99300/shader-dump.bin` back to the build machine.

### 5. Recompile the shaders

```bash
git -C EdgeOfTimeRecomp/thirdparty/XenosRecomp submodule update --init --recursive
cmake -S EdgeOfTimeRecomp/thirdparty/XenosRecomp -B build-xenosrecomp -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build build-xenosrecomp --target XenosRecomp

mkdir -p shaders-in && cp shader-dump.bin shaders-in/    # and any shader-misses.bin
build-xenosrecomp/XenosRecomp/XenosRecomp shaders-in \
  EdgeOfTimeRecomp/generated/shader_cache.cpp \
  EdgeOfTimeRecomp/thirdparty/XenosRecomp/XenosRecomp/shader_common.h
python3 EdgeOfTimeRecomp/ps5/tools/split-shader-cache.py
```

XenosRecomp's input must be a folder. It takes about 30 minutes on four cores.
`split-shader-cache.py` writes `generated/shader-cache.zst` (about 64 MB), the file that
goes beside `eboot.bin`, and the table the build links.

### 6. Build the game again, with its shaders

```bash
cd PS5_EdgeOfTimeTitle && ps5/tools/build.sh
```

This writes the title folder `dist/PPSA99300/` again, now with an `eboot.bin` of about
77 MB that runs the game.

## Install your own build

Your folder on the console ends up like this:

```
/data/homebrew/PPSA99300/
  eboot.bin, sce_sys/, sce_module/, port/, assets/, ...      from dist/PPSA99300/
  shader-cache.zst                                           from EdgeOfTimeRecomp/generated/
  gamedata/                                                  your game files
```

Copy the new `eboot.bin` from `dist/PPSA99300/` over the first one, and
`shader-cache.zst` beside it. Then launch *Spider-Man Edge Of Time* from the home
screen.

`eboot.bin` and `shader-cache.zst` are a pair: always update them together. With a
mismatched pair, the game logs that the shader file doesn't match and draws nothing.

Your saves, settings and logs are in the title's folder, under `profiles/` and `logs/`.
Back up `profiles/` before you move or remove anything. Never delete the title from the
home screen while its folder is still in `/data/homebrew`.

### Title art

The home-screen name is in `ps5/sce_sys/param.json`. For your own icon and background,
put an `icon-source` and a `background-source` image (.png, .jpg or .webp) in
`reeot/title-art/`, run `python3 reeot/title-art.py`, and rebuild. Git ignores that
folder, so art taken from the game stays with you.

The PS5 keeps the name and art it read when the title was first registered. To make it
read the new files: upload a `PPSA99300` folder that holds only the new `sce_sys/` with
ps5upload, overriding the installed game. The title registers again with the new name
and art, and the rest of the folder stays as it was.

## Shaders the game finds while playing

Step 4's dump should catch every shader the game has: a full playthrough needed none
besides the packs' and `Default.xex`'s. As a safety net, whenever the game uses a
shader that isn't in the cache, it appends it to
`/data/homebrew/PPSA99300/shader-misses.bin`. If that file appears, something drawn
with that shader was skipped.

To add them: put `shader-misses.bin` in `shaders-in/` beside `shader-dump.bin`, run
steps 5 and 6 again, and install the new `eboot.bin` and `shader-cache.zst` together.

## Troubleshooting

- **Black screen.** Check the newest file in `logs/` for `[shaders]`. Look for a missing
  `shader-cache.zst`, a shader file that doesn't match this `eboot.bin`, or an empty cache.
- **It closes at once.** `reeot-test-results.txt` names the step it stopped at. If
  `gamedata/Default.xex` is missing, it says so: the game files were copied one folder
  too deep, or not at all.
- **It doesn't appear on the home screen.** Check ShadowMountPlus's
  `/data/shadowmount/debug.log`.

## Never share game files

The download has no game files, and none may be shared. Don't upload or pass on your
`gamedata/` folder, your copy of the game, `EdgeOfTimeRecomp/assets/` or
`shader-dump.bin`, and don't ask others for theirs. Point people to the release instead,
so that they add their own copy of the game.

## Credits

This port stands on a lot of other people's work.

**[EdgeOfTimeRecompiled](https://github.com/goliathret/EdgeOfTimeRecomp)**: the
recompilation of the game this port is built from. Its credits, in full in its README:
- [Graine25](https://github.com/Graine25), its creator and a maintainer of the ReXGlue
  SDK;
- Serjar, for the reversing of the game and its PAK format;
- [Maff](https://github.com/spyrosadventure), for most of the PKZLib pipeline;
- [hellomemy](https://x.com/KnowNoEvry), for the suit overhauls;
- [Boma](https://github.com/goldislead), who fixed Edge of Time on Xenia and is the
  biggest reason the project exists;
- its playtesters and translators.

**[ReXGlue SDK](https://github.com/rexglue/rexglue-sdk)**: the Xbox 360 recompiler and
runtime. This port runs its PS5 fork,
[KongaTime/rexglue-sdk](https://github.com/KongaTime/rexglue-sdk).

**Mihawk ([mihawk-99](https://github.com/mihawk-99))**: Vulkan on the PS5, which every
frame of the game is drawn through. [PS5_Vulkan](https://github.com/mihawk-99/PS5_Vulkan)
and the [PS5_Mesa](https://github.com/mihawk-99/PS5_Mesa) fork bring Mesa's RADV driver
to the console. The [PS5 Vulkan Template](https://github.com/mihawk-99/PS5_VulkanTemplate)
is the title's foundation: its platform layer, link and console tools, with Sascha
Willems' Vulkan examples. The [payload SDK fork](https://github.com/mihawk-99/PS5_PayloadSDK)
gives the title its platform layer.

**[The Simpsons Game PS5 port](https://github.com/LoZazaMastro/TheSimpsonsGame-PS5-Builder)**
by LoZazaMastro, built on YesterMester's
[TheSimpsonsGameRecomp](https://github.com/YesterMester/TheSimpsonsGameRecomp): the
earlier ReXGlue game on the PS5, and the reference for this port's PS5 runtime.

**Also built on:**
- [Xenia](https://github.com/xenia-project/xenia) and
  [Xenia Canary](https://github.com/xenia-canary/xenia-canary), whose work on the Xbox
  360 is under all of this;
- [XenosRecomp](https://github.com/hedge-dev/XenosRecomp) and
  [Unleashed Recompiled](https://github.com/hedge-dev/UnleashedRecomp) (hedge-dev), for
  the shader recompiler (through goliathret's
  [fork](https://github.com/goliathret/reeot-XenosRecomp)) and for showing what a
  static recompilation can be;
- [PKZLib](https://github.com/goliathret/PKZLib), which the shader dump reads the packs
  with;
- [plume](https://github.com/renderbag/plume), the renderer, through zolaware's
  [fork](https://github.com/zolaware/plume);
- the [PS5 payload SDK](https://github.com/ps5-payload-dev/sdk) (John Törnblom and the
  ps5-payload-dev contributors), which Mihawk's fork builds on;
- [Mesa](https://mesa3d.org)'s RADV Vulkan driver;
- [etaHEN](https://github.com/etaHEN/etaHEN),
  [ShadowMountPlus](https://github.com/drakmor/ShadowMountPlus) and
  [ftpsrv](https://github.com/ps5-payload-dev/ftpsrv);
- zstd, xxHash, SMOL-V and DirectXShaderCompiler.

*Spider-Man: Edge of Time* was developed by Beenox and published by Activision. Spider-Man
is a Marvel character. This is an unofficial fan project, with no connection to any of
them.

PS5 port: [KongaTime](https://github.com/KongaTime).
