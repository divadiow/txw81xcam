#!/usr/bin/env python3
from __future__ import annotations
import hashlib
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "diag" / "out"
OUT.mkdir(parents=True, exist_ok=True)
LOG = OUT / "build.log"
LOG.write_text("")


def log(text: object) -> None:
    with LOG.open("a", encoding="utf-8", errors="replace") as f:
        f.write(str(text) + "\n")


def run(args: list[object], timeout: int = 900) -> subprocess.CompletedProcess[str]:
    cmd = [str(x) for x in args]
    log("$ " + " ".join(shlex.quote(x) for x in cmd))
    p = subprocess.run(cmd, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=timeout)
    log(p.stdout)
    log(f"rc={p.returncode}")
    return p


def locate_tool(name: str) -> Path:
    p = shutil.which(name)
    if p:
        return Path(p)
    matches = [x for x in ROOT.parent.rglob(name) if x.is_file()]
    if not matches:
        raise RuntimeError(f"tool not found: {name}")
    matches[0].chmod(matches[0].stat().st_mode | 0o111)
    return matches[0]


gcc = None
for candidate in ("csky-abiv2-elf-gcc", "csky-elfabiv2-gcc"):
    try:
        gcc = locate_tool(candidate)
        break
    except RuntimeError:
        pass
if gcc is None:
    for p in ROOT.parent.rglob("*gcc"):
        if p.is_file() and "csky" in p.name.lower():
            p.chmod(p.stat().st_mode | 0o111)
            gcc = p
            break
if gcc is None:
    raise RuntimeError("C-SKY compiler not found")

prefix = str(gcc)[:-3]

def sibling(name: str) -> Path:
    p = Path(prefix + name)
    if p.exists():
        p.chmod(p.stat().st_mode | 0o111)
        return p
    q = shutil.which(p.name)
    if q:
        return Path(q)
    raise RuntimeError(f"tool not found: {name}")

nm = sibling("nm")
readelf = sibling("readelf")
objcopy = sibling("objcopy")
objdump = sibling("objdump")

source = ROOT / "diag" / "txw817_810_diag.c"
archives = sorted(ROOT.rglob("*.a"))
linker_scripts = sorted(ROOT.rglob("*.ld"), key=lambda p: (0 if "romcode_org" in str(p).lower() else 1 if "romcode" in str(p).lower() else 2, len(str(p))))
if not archives or not linker_scripts:
    raise RuntimeError("SDK archives/linker scripts not found")

include_dirs = {ROOT / "sdk" / "include", ROOT / "sdk" / "include" / "chip" / "txw81x", ROOT / "project", ROOT / "csky" / "csi_core" / "include", ROOT / "csky" / "configs"}
for p in ROOT.rglob("*.h"):
    include_dirs.add(p.parent)
include_args = [f"-I{x}" for x in sorted(include_dirs, key=str)]

flag_candidates = [
    ["-mcpu=ck803"],
    ["-mcpu=ck803s"],
    ["-mcpu=ck803", "-msoft-float"],
    ["-mcpu=ck803", "-mhard-float"],
    [],
]
objects: list[tuple[Path, list[str]]] = []
for index, flags in enumerate(flag_candidates):
    obj = OUT / f"diag_{index}.o"
    p = run([gcc, *flags, "-std=gnu99", "-Os", "-ffunction-sections", "-fdata-sections", "-fno-common", "-fno-builtin", *include_args, "-c", source, "-o", obj], 300)
    if p.returncode == 0:
        objects.append((obj, flags))
if not objects:
    raise RuntimeError("diagnostic source did not compile")

# Resolve likely entry points from linker scripts and archive symbols.
entries: list[str] = []
for ld in linker_scripts:
    entries.extend(re.findall(r"ENTRY\s*\(\s*([^\s)]+)", ld.read_text(errors="ignore")))
entries.extend(["_start", "__start", "Reset_Handler", "reset_handler", "csky_start"])
entries = list(dict.fromkeys(entries))

core_archives = [a for a in archives if "libcore" in a.name.lower()]
link_modes = [
    ([], [], ["-lc", "-lgcc"]),
    (["-nostartfiles"], [], ["-lc", "-lgcc"]),
    (["-nostartfiles"], ["-Wl,--whole-archive", *map(str, core_archives), "-Wl,--no-whole-archive"], ["-lc", "-lgcc"]),
    (["-nostdlib"], [], []),
]

success = None
attempt = 0
for obj, flags in objects:
    for ld in linker_scripts[:24]:
        script_text = ld.read_text(errors="ignore")
        m = re.search(r"ENTRY\s*\(\s*([^\s)]+)", script_text)
        forced = [m.group(1)] if m else entries[:5]
        for startup_opts, pre_group, runtime in link_modes:
            for entry in forced or [None]:
                attempt += 1
                elf = OUT / f"candidate_{attempt}.elf"
                mapfile = OUT / f"candidate_{attempt}.map"
                entry_arg = [f"-Wl,-u,{entry}"] if entry else []
                args = [
                    gcc, *flags, *startup_opts, str(obj),
                    f"-Wl,-T,{ld}", f"-Wl,-Map,{mapfile}",
                    "-Wl,--gc-sections", "-Wl,--allow-multiple-definition",
                    *entry_arg, *pre_group,
                    "-Wl,--start-group", *map(str, archives), *runtime, "-Wl,--end-group",
                    "-o", elf,
                ]
                p = run(args, 1200)
                if p.returncode == 0 and elf.exists() and elf.stat().st_size:
                    success = (elf, mapfile, obj, flags, ld, startup_opts, runtime, entry)
                    break
            if success:
                break
        if success:
            break
    if success:
        break

if not success:
    raise RuntimeError("no valid TXW81x ELF link was found")

elf, mapfile, obj, flags, ld, startup_opts, runtime, entry = success
final_elf = OUT / "txw817_810_diag_v0.1.elf"
final_map = OUT / "txw817_810_diag_v0.1.map"
shutil.copy2(elf, final_elf)
shutil.copy2(mapfile, final_map)

nm_text = run([nm, "-n", final_elf], 120).stdout
for required in ("main", "system_goto_boot"):
    if not re.search(r"\b" + re.escape(required) + r"\b", nm_text):
        raise RuntimeError(f"linked ELF is missing {required}")
header = run([readelf, "-h", final_elf], 120).stdout
sections = run([readelf, "-S", final_elf], 120).stdout
segments = run([readelf, "-l", final_elf], 120).stdout
if "C-SKY" not in header and "CSKY" not in header.upper():
    raise RuntimeError("output is not a C-SKY ELF")

hex_file = OUT / "txw817_810_diag_v0.1.hex"
raw_file = OUT / "txw817_810_diag_v0.1.raw.bin"
if run([objcopy, "-O", "ihex", final_elf, hex_file], 120).returncode:
    raise RuntimeError("Intel HEX generation failed")
if run([objcopy, "-O", "binary", final_elf, raw_file], 120).returncode:
    raise RuntimeError("raw binary generation failed")
(OUT / "txw817_810_diag_v0.1.disasm.txt").write_text(run([objdump, "-d", "-S", final_elf], 300).stdout, encoding="utf-8", errors="replace")
(OUT / "validation.txt").write_text(header + "\n" + sections + "\n" + segments + "\n" + nm_text, encoding="utf-8", errors="replace")

artifacts = {}
for p in (final_elf, final_map, hex_file, raw_file):
    artifacts[p.name] = {"size": p.stat().st_size, "sha256": hashlib.sha256(p.read_bytes()).hexdigest()}
report = {
    "success": True,
    "build_validated": True,
    "hardware_validated": False,
    "compiler": str(gcc),
    "flags": flags,
    "linker_script": str(ld.relative_to(ROOT)),
    "startup_options": startup_opts,
    "runtime": runtime,
    "forced_entry": entry,
    "attempts": attempt,
    "artifacts": artifacts,
}
(OUT / "build_report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
(OUT / "SUCCESS").write_text("Build-validated C-SKY ELF; hardware validation pending.\n", encoding="utf-8")
print(json.dumps(report, indent=2))
