#!/usr/bin/env python3
"""Verify the distributable Win32 PE has no dependency on dynamic MSVC CRT.

Uses only Python's standard library; called on the actual linked .asi in CI.
Does NOT inspect transitive imports of other plugins or Windows DLLs.
"""
from __future__ import annotations
import argparse
import pathlib
import struct


class AuditError(Exception):
    pass


def u16(data: bytes, off: int) -> int:
    if off < 0 or off + 2 > len(data):
        raise AuditError("truncated PE u16")
    return struct.unpack_from("<H", data, off)[0]


def u32(data: bytes, off: int) -> int:
    if off < 0 or off + 4 > len(data):
        raise AuditError("truncated PE u32")
    return struct.unpack_from("<I", data, off)[0]


def imports(path: pathlib.Path) -> set[str]:
    data = path.read_bytes()
    if data[:2] != b"MZ":
        raise AuditError("not a DOS/PE binary")
    pe = u32(data, 0x3C)
    if pe + 24 > len(data) or data[pe:pe + 4] != b"PE\0\0":
        raise AuditError("invalid PE header")
    machine = u16(data, pe + 4)
    if machine != 0x14C:
        raise AuditError(f"ASI is not x86 Win32 (machine={machine:#x})")
    nsections = u16(data, pe + 6)
    optional_size = u16(data, pe + 20)
    optional = pe + 24
    if u16(data, optional) != 0x10B:
        raise AuditError("not PE32")
    if optional_size < 112 or optional + optional_size > len(data):
        raise AuditError("optional PE header missing import directory")
    n_dirs = u32(data, optional + 92)
    if n_dirs < 2:
        raise AuditError("PE has no declared import directory")
    import_rva = u32(data, optional + 104)
    import_size = u32(data, optional + 108)
    if not import_rva or import_size < 20:
        raise AuditError("unexpected empty import directory")
    sections = []
    section_start = optional + optional_size
    if not 1 <= nsections <= 96:
        raise AuditError("invalid section count")
    for n in range(nsections):
        i = section_start + n * 40
        virtual_size = u32(data, i + 8)
        virtual_address = u32(data, i + 12)
        raw_size = u32(data, i + 16)
        raw_ptr = u32(data, i + 20)
        sections.append((virtual_address, max(virtual_size, raw_size), raw_ptr))

    def at_rva(rva: int) -> int:
        for start, size, raw in sections:
            if start <= rva < start + size:
                pos = raw + rva - start
                if pos < len(data):
                    return pos
        raise AuditError(f"RVA outside file: {rva:#x}")

    output: set[str] = set()
    directory = at_rva(import_rva)
    for n in range(min(import_size // 20, 1024)):
        off = directory + 20 * n
        entry = [u32(data, off + i * 4) for i in range(5)]
        if not any(entry):
            break
        name = at_rva(entry[3])
        zero = data.find(b"\0", name, min(len(data), name + 256))
        if zero < 0:
            raise AuditError("malformed DLL import name")
        output.add(data[name:zero].decode("ascii").lower())
    else:
        raise AuditError("unterminated PE import descriptors")
    return output


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("asi", type=pathlib.Path)
    args = parser.parse_args()
    dlls = imports(args.asi)
    forbidden = {
        "msvcp140.dll", "msvcp140_1.dll", "msvcp140_2.dll",
        "vcruntime140.dll", "vcruntime140_1.dll",
        "ucrtbase.dll", "msvcr120.dll", "msvcp120.dll",
    }
    bad = sorted(dlls & forbidden)
    if bad:
        raise AuditError("dynamic MSVC runtime dependencies remain: " + ", ".join(bad))
    print("PASS: x86 PE32, no direct dynamic MSVC CRT DLL imports")
    print("DLL imports: " + ", ".join(sorted(dlls)))


if __name__ == "__main__":
    main()
