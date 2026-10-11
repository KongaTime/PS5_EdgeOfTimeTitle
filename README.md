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

## What it runs now: the guest memory test

The game needs ReXGlue's guest memory: a 4.5 GiB arena with nine aliased views at
`0x10_0000_0000`, backed by the console's direct memory. That is the fork's
`src/core/memory_ps5.cpp`. Its test (`ps5/tests/memory_ps5_test.cpp` in the fork)
passes on the build host. On the console it needs a title's process, because a
payload's process has no direct memory. So for now the title's program is that test,
compiled into the title (`reeot/reeot.cmake`). It writes its lines to
`/app0/rexglue-memory-test.txt`, which is `/data/homebrew/PPSA99300/` on the console,
and the title ends when the test does.

The test answers three questions:

- whether the console accepts no-access protection on the shared views (the guest's MMIO windows)
- how long backing the physical heap's 512 MiB takes
- whether protection stays per view on the real kernel

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
reeot/run-memory-test.sh                    # build, upload, launch, follow klog, fetch the results
```

`reeot/run-memory-test.sh` runs `ps5/tools/build.sh` (into `dist/PPSA99300/`),
`ps5/tools/deploy.sh` (FTP) and `ps5/tools/run.sh --menu` (a launch through the
control payload, with klog in `klog/<run>/klog.log`). Then it fetches the results
file into `klog/<run>/rexglue-memory-test.txt` and prints it. The test's last line is
`==== N failed ====`.

If the console's klog port is closed, `run.sh` cannot follow the title. In that case,
launch the title from the home screen, wait for it to close, and run
`reeot/run-memory-test.sh --fetch`.

## Licences

The foundation is MIT (`LICENSE.md`, Sascha Willems'; the PS5 layer is under the same
licence), and so is `reeot/`. The fork's sources compiled in keep ReXGlue's BSD-style licence.
The built title links the PS5 platform layer of the payload SDK fork, which is
GPL-3.0, so the title as distributed is under GPL-3.0.
