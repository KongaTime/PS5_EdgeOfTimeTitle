#!/usr/bin/env bash
# EdgeOfTimeRecomp for the PS5 - the title's checks on the console, start to end.
#
#   reeot/run-tests.sh            build, upload, launch, follow klog, fetch the results
#   reeot/run-tests.sh --fetch    only fetch the results (after a launch from the home screen)
#
# The title is built (ps5/tools/build.sh) and uploaded (ps5/tools/deploy.sh),
# launched through PS5_Vulkan's control payload with klog followed until the title
# ends (ps5/tools/run.sh --menu), and the checks' results file comes back from the
# title's folder over FTP (/app0/reeot-test-results.txt to the title,
# /data/homebrew/<TITLE_ID>/ on the console) into klog/<run>/, and is printed.
# The console's address and ports are PS5_Vulkan's .env, as for every console tool.
#
# Copyright (C) 2026 KongaTime
# SPDX-License-Identifier: MIT
set -euo pipefail

root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
vulkan=$(cd -- "${PS5_VULKAN_DIR:-$root/../PS5_Vulkan}" && pwd)
title_id=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["titleId"])' "$root/ps5/sce_sys/param.json")

run_dir=
if [[ ${1:-} != --fetch ]]; then
    "$root/ps5/tools/build.sh"
    "$root/ps5/tools/deploy.sh"
    "$root/ps5/tools/run.sh" --menu || echo "run.sh did not see the title end; fetching what the checks wrote" >&2
    # run.sh's klog/<run>/, the newest
    run_dir=$(find "$root/klog" -mindepth 1 -maxdepth 1 -type d 2>/dev/null | sort | tail -1) || true
fi
[[ -n $run_dir ]] || run_dir="$root/klog/$(date +%Y%m%d-%H%M%S)"
mkdir -p "$run_dir"
results="$run_dir/reeot-test-results.txt"
python3 - "$vulkan/tools" "/data/homebrew/$title_id/reeot-test-results.txt" "$results" <<'PY'
import sys
from ftplib import FTP
sys.path.insert(0, sys.argv[1])
import ps5_console
settings = ps5_console.load_settings()
ftp = FTP()
ftp.connect(settings["host"], settings["ftp_port"], timeout=15)
ftp.login(settings["ftp_user"], settings["ftp_password"])
with open(sys.argv[3], "wb") as results:
    ftp.retrbinary(f"RETR {sys.argv[2]}", results.write)
ftp.quit()
PY
cat "$results"
echo "saved: $results"
