# EdgeOfTimeRecomp for the PS5 (title)

The PS5 homebrew title (`PPSA99300`) for
[EdgeOfTimeRecomp](https://github.com/KongaTime/EdgeOfTimeRecomp), the static
recompilation of the Xbox 360 game on the ReXGlue SDK. The plan is
EdgeOfTimeRecomp's `docs/PS5-PORTING.md`. The runtime is the ReXGlue fork with
the PS5 target, [KongaTime/rexglue-sdk](https://github.com/KongaTime/rexglue-sdk).

The title is made from the
[PS5 Vulkan Template](https://github.com/mihawk-99/PS5_VulkanTemplate) foundation
at `f827f0a5`: Sascha Willems' Vulkan example base class with its PS5 hooks, and the
PS5 layer (the launch, the pad, klog, the build and the console tools). It links RADV
the same way. Its program is in `reeot/`, and runs in place of the template's samples
(`PS5_TITLE_MAIN` in `ps5/src/main.cpp`).

## What it runs now: test app 5

The title links the ReXGlue fork's runtime and runs the ReXGlue app the link holds
through the runtime's PS5 entry, `rex_ps5_app_main` (`reeot/title_main.cpp`). For now
that app is test app 5 (`reeot/rex_test_app.cpp`), which checks, in order:

- the window: the display surface's size;
- the picture: EdgeOfTimeRecomp's renderer, plume (its `thirdparty/plume` with
  `ps5/patches/plume-ps5.patch`), on the PS5 display: plume's device, a command queue and
  a swap chain on a `VK_KHR_display` surface, cleared to red, green, blue and white for a
  second each (`reeot/plume_test.cpp`). plume gets Vulkan from the RADV linked into the
  title; its messages go into the results file;
- the guest memory: `Memory::Initialize` and a read of the app's code;
- the sound: a one-second 440 Hz tone through the ported `sceAudioOut` driver.

The build takes plume from EdgeOfTimeRecomp's checkout (`EDGEOFTIMERECOMP_SRC`, or
`../EdgeOfTimeRecomp`), with its submodules and the patch applied.

Test app 5 ran on the console on 2026-10-12 (`reeot/results/2026-10-12-test5.txt`): plume
made a 3840x2160 swap chain of three B8G8R8A8 images on the display and presented 244
frames in four seconds (the display's 60 Hz); the four colors showed, then the tone.

Test apps 3 and 4 (2026-10-12, `reeot/results/`) checked the window, the memory, the
sound, every button, stick and trigger of the controller, the rumble and the fault
records (`/app0/logs/fault.log`, with a backtrace); their controller and crash steps are
in `reeot/rex_test_app.cpp`'s history (`a6dad73`).

It writes `/app0/reeot-test-results.txt` (`/data/homebrew/PPSA99300/` on the console),
with the runtime's start-up stages (`prospero_startup_log`, which the title gives the
runtime) among its lines. Each line is written before its step, so a file that stops
says where.

Earlier checks, now part of the runtime: the guest memory test passed on the console
on 2026-10-11 (the fork's `ps5/tests/results/2026-10-11-title.txt`). The code read check
(`reeot/results/2026-10-12.txt`; its code is in this repository's history, `9e0d568`)
found that untouched code ends the title when read, and that `sceKernelMprotect`
read+execute makes it readable, which the fork now does when its fault handler
installs.

## Building and running

It builds against the PS5 stack, checked out beside it:

| Beside it | What for |
| --- | --- |
| `PS5_Vulkan` | the RADV release archive, the link recipe, the native tool, `libc.prx`, the control payload, and `.env` (the console's address) |
| `PS5_Mesa` | RADV's source, for `PS5_Vulkan` to build the archive |
| `PS5_PayloadSDK` | the payload SDK fork; `ps5/tools/setup-sdk.sh` installs it at `1de8b37`, the revision PS5_RPCS3Title and EdgeOfTimeRecomp pin |
| `rexglue-sdk` | the ReXGlue fork (`REXGLUE_SRC` overrides) |
| `EdgeOfTimeRecomp` | its `ps5/tools/build-rexglue.sh` builds the fork's runtime for the PS5, into `rexglue-sdk/out/ps5-amd64/` (`REXGLUE_PS5_BUILD` names its build folder) |

```bash
git clone https://github.com/KongaTime/rexglue-sdk
git clone -b claude/nifty-knuth-s27y9o https://github.com/KongaTime/EdgeOfTimeRecomp
git -C PS5_PayloadSDK pull                  # 1de8b37 or later
EdgeOfTimeRecomp/ps5/tools/build-rexglue.sh # the runtime, for the PS5
cd PS5_EdgeOfTimeTitle
ps5/tools/bootstrap.sh                      # checks the host and everything beside it
reeot/run-tests.sh                          # build, upload, launch, follow klog, fetch the results
```

`reeot/run-tests.sh` runs `ps5/tools/build.sh` (into `dist/PPSA99300/`),
`ps5/tools/deploy.sh` (FTP) and `ps5/tools/run.sh --menu` (a launch through the
control payload, with klog in `klog/<run>/klog.log`). Then it fetches the results
file into `klog/<run>/reeot-test-results.txt` and prints it. Test app 4 ends on its
crash on purpose; its record is `logs/fault.log` in the title's folder.

If the console's klog port is closed, `run.sh` cannot follow the title. In that case,
launch the title from the home screen, wait for it to close, and run
`reeot/run-tests.sh --fetch`.

## Licences

The foundation is MIT (`LICENSE.md`, Sascha Willems'; the PS5 layer is under the same
licence), and so is `reeot/`. The ReXGlue runtime linked in is BSD 3-Clause (the fork's
`LICENSE` and `ps5/PORTED.md`).
The built title links the PS5 platform layer of the payload SDK fork, which is
GPL-3.0, so the title as distributed is under GPL-3.0.
