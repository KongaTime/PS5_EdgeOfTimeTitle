#!/usr/bin/env python3
"""EdgeOfTimeRecomp for the PS5 - one recompiled DLL module as one object a title links.

On the desktop each of the game's DLL modules (GameLogic.dll) is a shared library
of its own, so its names never meet the executable's: both define PPCImageConfig
and PPCFuncMappings, for one. A title links everything into one executable, so
this joins a module's objects into one relocatable object (ld -r) and gives every
name it defines strongly a prefix of its own. The module's references to those
names follow them; what it refers to elsewhere (the runtime, the executable's
exports) and what it defines weakly (inline functions, templates, shared alike)
keep their names. ReXModule_Register and ReXModule_GetImageInfo, the two names
the runtime looks up in a module (kernel_state.cpp), take the prefix too:
reeot/gamelogic_static.cpp gives them back to it by their own names
(rex::platform::RegisterStaticLibrary).

usage: prelink_module.py --bin <SDK bin> --prefix <prefix> --out <object> <objects...>

Copyright (C) 2026 KongaTime
SPDX-License-Identifier: MIT
"""
import argparse
import os
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--bin", required=True, help="the payload SDK's bin (ld.lld, llvm-nm, llvm-objcopy)")
    parser.add_argument("--prefix", required=True)
    parser.add_argument("--out", required=True)
    parser.add_argument("objects", nargs="+")
    args = parser.parse_args()

    joined = args.out + ".joined.o"
    subprocess.run([os.path.join(args.bin, "ld.lld"), "-r", "-o", joined, *args.objects], check=True)

    symbols = subprocess.run([os.path.join(args.bin, "llvm-nm"), "-g", "--defined-only", joined],
                             check=True, capture_output=True, text=True).stdout
    renames = []
    for line in symbols.splitlines():
        fields = line.split()
        # value, type, name; strong definitions only (T D B R and the like, not W V)
        if len(fields) == 3 and fields[1] in "TDBRSACG":
            renames.append(f"{fields[2]} {args.prefix}{fields[2]}")
    if not any(r.startswith("ReXModule_Register ") for r in renames):
        sys.exit("prelink_module.py: the module defines no ReXModule_Register")

    rename_file = args.out + ".renames"
    with open(rename_file, "w") as f:
        f.write("\n".join(renames) + "\n")
    subprocess.run([os.path.join(args.bin, "llvm-objcopy"), f"--redefine-syms={rename_file}", joined, args.out],
                   check=True)
    os.remove(joined)
    print(f"prelink_module.py: {len(args.objects)} objects, {len(renames)} names given {args.prefix}")


if __name__ == "__main__":
    main()
