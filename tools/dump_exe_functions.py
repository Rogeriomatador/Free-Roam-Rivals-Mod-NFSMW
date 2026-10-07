#!/usr/bin/env python3
"""Read-only dump of MW05 function prologues from the supported speed.exe.

Purpose: let the project verify, offline and without touching the game, the
ABI of the engine functions the SDKs claim for vehicle creation. The mod must
not call PVehicle::Construct until the bytes below confirm calling convention,
argument layout and callee stack cleanup.

Usage:
    python tools/dump_exe_functions.py "D:\\...\\speed.exe" [-o report.txt]

Fail-closed: refuses any executable that is not the supported v1.3 target.
Optional: `pip install capstone` adds a disassembly listing.
"""
import argparse
import hashlib
import struct
import sys
from pathlib import Path

SUPPORTED_SIZE = 6029312
SUPPORTED_MD5 = "C0516B485065FABDD69579816B5DF763"
DUMP_BYTES = 192

# name -> (virtual address, evidence source). Sources are SDK leads, not proof.
TARGETS = {
    "PVehicle::Construct (generic Sim factory)": (0x689820, "MWSDK/NFSPluginSDK PVehicle.h; same VA also used by ResetCar with BehaviorParams"),
    "PVehicle::PVehicle": (0x689020, "MWSDK mw05.hpp registry comment"),
    "IVehicle::GetDriverClass": (0x6880B0, "MWSDK mw05.hpp (verified vtable slot 22)"),
    "VehicleParams::AddTypeName": (0x4040F0, "MWSDK VehicleParams.h"),
    "Smackable factory (comparison)": (0x6895A0, "MWSDK Smackable.h"),
    "CreateAIVehicleRacerInstance": (0x43EF70, "repo docs/ENGINE_INTEGRATION_MAP.md"),
    "AIGoalRacer constructor entry (reported)": (0x43D330, "Claude dump findings; original dump not independently reproduced"),
    "AIGoalRacer interior address (do not call)": (0x43D388, "Claude dump findings; not a callable entry"),
    "AI goal racer pool wrapper (found in dump v1)": (0x43D400, "dump v1: function starting after ret 4 of 0x43D330"),
    "Object creation callee of 0x689820": (0x4E4EA0, "dump v1: call at 0x6898BC"),
    "Param type-check failure callee": (0x45CD20, "dump v1: call at 0x689870"),
}


class PE:
    def __init__(self, data):
        self.data = data
        if data[:2] != b"MZ":
            raise ValueError("not a PE file (no MZ)")
        pe = struct.unpack_from("<I", data, 0x3C)[0]
        if data[pe:pe + 4] != b"PE\0\0":
            raise ValueError("not a PE file (no PE signature)")
        nsec, = struct.unpack_from("<H", data, pe + 6)
        optsz, = struct.unpack_from("<H", data, pe + 20)
        opt = pe + 24
        if struct.unpack_from("<H", data, opt)[0] != 0x10B:
            raise ValueError("not a PE32 (32-bit) image")
        self.image_base, = struct.unpack_from("<I", data, opt + 28)
        self.sections = []
        sec = opt + optsz
        for i in range(nsec):
            o = sec + 40 * i
            name = data[o:o + 8].rstrip(b"\0").decode("ascii", "replace")
            vsize, vaddr, rawsize, rawptr = struct.unpack_from("<IIII", data, o + 8)
            self.sections.append((name, vaddr, max(vsize, rawsize), rawptr, rawsize))

    def va_to_offset(self, va):
        rva = va - self.image_base
        for name, vaddr, size, rawptr, rawsize in self.sections:
            if vaddr <= rva < vaddr + size:
                off = rawptr + (rva - vaddr)
                if rva - vaddr >= rawsize:
                    return None, name
                return off, name
        return None, None


def build_report(path, data, nbytes=DUMP_BYTES):
    if not 16 <= nbytes <= 4096:
        raise ValueError("bytes per target must be between 16 and 4096")
    md5 = hashlib.md5(data).hexdigest().upper()
    if len(data) != SUPPORTED_SIZE or md5 != SUPPORTED_MD5:
        raise SystemExit(
            "REFUSED: not the supported speed.exe (size %d md5 %s)." % (len(data), md5))
    pe = PE(data)
    try:
        import capstone
        md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    except ImportError:
        md = None
    out = ["FRR_EXE_DUMP_V2", "file=%s" % path, "size=%d" % len(data), "md5=%s" % md5,
           "bytesPerTarget=%d" % nbytes,
           "imageBase=0x%X" % pe.image_base, "capstone=%s" % ("yes" if md else "no"), ""]
    out.append("ABI verification: NOT performed. Linear disassembly can cross function boundaries; return candidates do not prove a calling convention.")
    for name, (va, source) in TARGETS.items():
        off, sec = pe.va_to_offset(va)
        out.append("== %s VA=0x%X source=%s" % (name, va, source))
        if off is None:
            out.append("   NOT MAPPED (section=%s)" % sec)
            continue
        # Do not cross the raw section boundary or hash a silently short window.
        section = next(s for s in pe.sections if s[0] == sec and s[3] <= off < s[3] + s[4])
        remaining = min(section[3] + section[4], len(data)) - off
        code = data[off:off + min(nbytes, remaining)]
        out.append("   section=%s fileOffset=0x%X" % (sec, off))
        out.append("   actualBytes=%d truncated=%s" % (len(code), "yes" if len(code) != nbytes else "no"))
        out.append("   sha256=%s" % hashlib.sha256(code).hexdigest())
        for i in range(0, len(code), 16):
            out.append("   +%03X  %s" % (i, code[i:i + 16].hex(" ")))
        if md:
            out.append("   disassembly:")
            for ins in md.disasm(code, va):
                out.append("   0x%X: %s %s" % (ins.address, ins.mnemonic, ins.op_str))
                if ins.mnemonic in ("ret", "retf"):
                    out.append("   decodedReturnCandidate=0x%X operand=%s (not a function-boundary or ABI proof)" % (ins.address, ins.op_str or "none"))
        out.append("")
    return "\n".join(out)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("exe")
    ap.add_argument("-o", "--output", default="FRR_exe_dump.txt")
    ap.add_argument("-n", "--bytes", type=int, default=DUMP_BYTES,
                    help="bytes dumped per function (default %d, max 4096)" % DUMP_BYTES)
    args = ap.parse_args(argv)
    exe_path = Path(args.exe)
    output_path = Path(args.output)
    if exe_path.resolve() == output_path.resolve():
        ap.error("output must not be the input executable")
    if output_path.exists():
        ap.error("output already exists; choose a new report name (no files are overwritten)")
    with open(args.exe, "rb") as f:
        data = f.read()
    report = build_report(args.exe, data, max(16, min(args.bytes, 4096)))
    with open(args.output, "x", encoding="utf-8") as f:
        f.write(report + "\n")
    print("Wrote %s" % args.output)
    return 0


if __name__ == "__main__":
    sys.exit(main())
