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

## What it runs now: two checks

For now the title's program is two checks, compiled into the title
(`reeot/reeot.cmake`). They write their lines to `/app0/reeot-test-results.txt`, which
is `/data/homebrew/PPSA99300/` on the console, and the title ends when they do.

- **The guest memory test.** The game needs ReXGlue's guest memory: a 4.5 GiB arena
  with nine aliased views at `0x10_0000_0000`, backed by the console's direct memory.
  That is the fork's `src/core/memory_ps5.cpp`, checked by its
  `ps5/tests/memory_ps5_test.cpp`, which needs a title's process (a payload's has no
  direct memory). It passed on the console on 2026-10-11: 12 GiB of direct memory, the
  512 MiB commit in 25.5 ms, no-access and per-view protection holding on the shared
  views (the fork's `ps5/tests/results/2026-10-11-title.txt`).
- **The code read check** (`reeot/code_read_check.cpp`). ReXGlue's MMIO handler reads
  the instruction that faulted. PS5_RPCS3 found a title's code execute-only (reading it
  ended the title, `SYSTEM_XO_VIOLATION`); the Simpsons Game's PS5 port reads it where it
  is. The check records what the kernel says of the title's code, whether
  `sceKernelMprotect` can make a page of it readable, and then reads it. Each line is
  written before its step, so a file ending on "reading ..." means that read ended the
  title.

## Building and running

It builds against the PS5 stack, checked out beside it:

| Beside it | What for |
| --- | --- |
| `PS5_Vulkan` | the RADV release archive, the link recipe, the native tool, `libc.prx`, the control payload, and `.env` (the console's address) |
| `PS5_Mesa` | RADV's source, for `PS5_Vulkan` to build the archive |
| `PS5_PayloadSDK` | the payload SDK fork; `ps5/tools/setup-sdk.sh` installs it at `1de8b37`, the revision PS5_RPCS3Title and EdgeOfTimeRecomp pin |
| `rexglue-sdk` | the ReXGlue fork (`REXGLUE_SRC` overrides), with its `fmt` and `simde` submodules |

```bash
git clone https://github.com/KongaTime/rexglue-sdk
git -C rexglue-sdk submodule update --init --depth 1 thirdparty/fmt thirdparty/simde
git -C PS5_PayloadSDK pull                  # 1de8b37 or later
cd PS5_EdgeOfTimeTitle
ps5/tools/bootstrap.sh                      # checks the host and everything beside it
reeot/run-tests.sh                          # build, upload, launch, follow klog, fetch the results
```

`reeot/run-tests.sh` runs `ps5/tools/build.sh` (into `dist/PPSA99300/`),
`ps5/tools/deploy.sh` (FTP) and `ps5/tools/run.sh --menu` (a launch through the
control payload, with klog in `klog/<run>/klog.log`). Then it fetches the results
file into `klog/<run>/reeot-test-results.txt` and prints it. The memory test ends on
`==== N failed ====`, the code read check on `==== code read check: done ====` (unless a
read ended the title).

If the console's klog port is closed, `run.sh` cannot follow the title. In that case,
launch the title from the home screen, wait for it to close, and run
`reeot/run-tests.sh --fetch`.

## Licences

The foundation is MIT (`LICENSE.md`, Sascha Willems'; the PS5 layer is under the same
licence), and so is `reeot/`. The fork's sources compiled in keep ReXGlue's BSD-style licence.
The built title links the PS5 platform layer of the payload SDK fork, which is
GPL-3.0, so the title as distributed is under GPL-3.0.
