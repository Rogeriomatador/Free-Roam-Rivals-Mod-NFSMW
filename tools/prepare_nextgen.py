#!/usr/bin/env python3
"""Build an isolated, attribution-preserving NEXT *alternative*, never an in-place FRR update.

The native-racer runtime comes UNMODIFIED from Zakkey250's public MIT release.
This script does not copy/link two vehicle managers into one game or install
anything into the user's game directory. Standard library only.
"""
from __future__ import annotations

import argparse
import hashlib
import pathlib
import re
import shutil
import stat
import sys
import zipfile

UPSTREAM_REPO = "Zakkey250/MW-NativeFreeRoamRacer"
UPSTREAM_VERSION = "v0.1.2"
UPSTREAM_ASSET = "MW-NativeFreeRoamRacer-v0.1.2.zip"
# From upstream docs. NOT inferred from matching executable byte length/MD5.
SUPPORTED_EXE_SHA256 = "b248271bf8eac8c9b283b8c95e3add672b713bf529b05f1780e58268493b9d06"
REQUIRED = {
    "nfsmwnativefreeroamracers.asi",
    "nfsmwnativefreeroamracers.ini",
    "nfsmwnativefreeroamracers.tpk",
}
PRESET = {
    ("Encounter", "Language"): "en",
    ("Encounter", "AIMode"): "Custom AI",
    ("Encounter", "WinStreakWager"): "0",
    ("Population", "Enabled"): "1",
    ("Population", "MaximumRacers"): "4",
    ("Vehicles", "Variety"): "3",
    ("LimitBreaker", "LBEngineAudioExpansion"): "0",
    ("LimitBreaker", "LBVehicleVarietyExpansion"): "0",
    ("Diagnostics", "Logging"): "0",
    ("Diagnostics", "FrameTiming"): "0",
    ("Updates", "Enabled"): "0",
}


class NextError(ValueError):
    pass


def sha256(path: pathlib.Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as reader:
        for block in iter(lambda: reader.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def read_upstream_checksum(path: pathlib.Path) -> str:
    """Read the release's SHA256SUMS file, not merely a user-provided ZIP name."""
    for raw in path.read_text(encoding="utf-8-sig").splitlines():
        line = raw.strip()
        simple = re.fullmatch(r"([a-fA-F0-9]{64})\s+\*?(.+)", line)
        bsd = re.fullmatch(r"SHA256\s*\((.+)\)\s*=\s*([a-fA-F0-9]{64})", line, re.I)
        if simple and pathlib.PurePosixPath(simple.group(2).replace("\\", "/")).name == UPSTREAM_ASSET:
            return simple.group(1).lower()
        if bsd and pathlib.PurePosixPath(bsd.group(1).replace("\\", "/")).name == UPSTREAM_ASSET:
            return bsd.group(2).lower()
    raise NextError("Upstream SHA256SUMS does not identify the pinned v0.1.2 ZIP")


def safe_archive_members(archive: zipfile.ZipFile) -> dict[str, zipfile.ZipInfo]:
    """Whitelist exactly the three upstream runtime pieces; fail on ambiguities."""
    found: dict[str, zipfile.ZipInfo] = {}
    total = 0
    for item in archive.infolist():
        if item.is_dir():
            continue
        raw = item.filename
        normalized = raw.replace("\\", "/")
        parts = pathlib.PurePosixPath(normalized).parts
        if not parts or normalized.startswith("/") or ".." in parts or ":" in normalized:
            raise NextError("unsafe archive entry path")
        mode = (item.external_attr >> 16) & 0o170000
        if mode == stat.S_IFLNK:
            raise NextError("archive contains a symlink")
        name = parts[-1].lower()
        if name not in REQUIRED:
            continue  # Do not bundle LimitBreaker, unrelated ASIs, or third-party assets.
        if name in found:
            raise NextError(f"duplicate upstream runtime entry: {name}")
        if item.file_size < 32 or item.file_size > 16 * 1024 * 1024:
            raise NextError(f"unexpected {name} size")
        total += item.file_size
        if total > 24 * 1024 * 1024:
            raise NextError("whitelisted runtime exceeds size budget")
        found[name] = item
    missing = REQUIRED.difference(found)
    if missing:
        raise NextError("official ZIP lacks required runtime entries: " + ", ".join(sorted(missing)))
    return found


def customize_ini(text: str) -> str:
    section: str | None = None
    changed: set[tuple[str, str]] = set()
    output = []
    for line in text.splitlines(keepends=True):
        header = re.match(r"^\s*\[([^\]]+)\]\s*(?:[;#].*)?$", line.strip())
        if header:
            section = header.group(1)
            output.append(line)
            continue
        match = re.match(r"^(\s*)([A-Za-z_][A-Za-z_0-9]*)(\s*=\s*)([^;#\r\n]*)(.*)$", line)
        key = (section, match.group(2)) if match else None
        if match and key in PRESET:
            if key in changed:
                raise NextError(f"ambiguous duplicate INI setting {key}")
            changed.add(key)
            line = match.group(1) + match.group(2) + match.group(3) + PRESET[key] + match.group(5)
        output.append(line)
    missing = set(PRESET) - changed
    if missing:
        raise NextError("upstream INI missing required preset fields: " + repr(sorted(missing)))
    return "".join(output)


def create_candidate(
    upstream_zip: pathlib.Path,
    checksums: pathlib.Path,
    output: pathlib.Path,
    upstream_license: pathlib.Path,
    optional_exe: pathlib.Path | None = None,
) -> pathlib.Path:
    if upstream_zip.name != UPSTREAM_ASSET:
        raise NextError("pin the official upstream release asset name, never an arbitrary ZIP")
    expected = read_upstream_checksum(checksums)
    actual = sha256(upstream_zip)
    if actual != expected:
        raise NextError(f"upstream ZIP SHA-256 mismatch: expected {expected}, got {actual}")
    if optional_exe is not None and sha256(optional_exe) != SUPPORTED_EXE_SHA256:
        raise NextError("speed.exe SHA-256 unsupported by the upstream NFRR v0.1.2 runtime")
    # Don't delete/rewrite an existing stage; no chance of damaging the game.
    if output.exists():
        raise NextError("output directory already exists; refusing to overwrite")
    if not upstream_license.exists() or "MIT License" not in upstream_license.read_text(encoding="utf-8"):
        raise NextError("upstream MIT license missing")
    with zipfile.ZipFile(upstream_zip) as archive:
        members = safe_archive_members(archive)
        ini = customize_ini(archive.read(members["nfsmwnativefreeroamracers.ini"]).decode("utf-8-sig"))
        root = output / "NEXT-Standalone"
        scripts = root / "scripts"
        scripts.mkdir(parents=True)
        for ext in ("asi", "tpk"):
            item = members[f"nfsmwnativefreeroamracers.{ext}"]
            (scripts / f"NFSMWNativeFreeRoamRacers.{ext}").write_bytes(archive.read(item))
        (scripts / "NFSMWNativeFreeRoamRacers.ini").write_text(ini, encoding="utf-8")
        shutil.copyfile(upstream_license, root / "LICENCA-NFRR-MIT.txt")
        guide = (
            "FREE ROAM RIVALS NEXT: PROPOSTA DE MOTOR DE RIVAIS ALTERNATIVO\n"
            "==========================================================\n"
            "CANDIDATO ISOLADO, NAO E UMA ATUALIZACAO DO ASI FreeRoamRivals.\n"
            "Motor independente de Zakkey250, MIT; versao publica v0.1.2.\n"
            "Atribuicao e direitos: ver LICENCA-NFRR-MIT.txt.\n\n"
            "ANTES DE INSTALAR: faca backup do jogo, INIs e saves.\n"
            "NUNCA rode FreeRoamRivals.asi e NFSMWNativeFreeRoamRacers.asi juntos:\n"
            "ambos controlam veiculos nativos e podem disputar capacidade/hooks.\n"
            "Desative/remova apenas o FreeRoamRivals.asi da pasta do ASI loader\n"
            "(guarde uma copia FORA de scripts). Depois use a pasta scripts\n"
            "deste candidato, sem substituir WidescreenFix e outros ASIs.\n"
            "Nao modifica speed.exe, saves, INIs antigos ou DLC.\n\n"
            "COMPATIBILIDADE: somente NFSPatcher English 1.3 LAA speed.exe,\n"
            f"SHA256 {SUPPORTED_EXE_SHA256.upper()}.\n"
            "O MD5 C0516B... registrado nos testes FRR NAO prova esse SHA-256.\n"
            "Confira o SHA-256 no Windows: Get-FileHash .\\speed.exe -Algorithm SHA256\n"
            "Se diferente, NAO tente usar o candidato no jogo.\n\n"
            "Configuracao inicial: 4 corredores, Custom AI, jogo sem LimitBreaker,\n"
            "efeitos de LB desativados, mensagens em ingles, update/log OFF.\n"
            "Ao contrario de nosso prototipo: o NFRR implementa populacao de\n"
            "corredores e corridas de 300m. NAO inclui Sprint nativo com largada.\n"
            "Nao sao garantidas ausencia de CTD ou compatibilidade com todo mod.\n"
            "NAO instale ambos ASIs para comparar; faca testes isolados.\n\n"
            "Fonte: https://github.com/Zakkey250/MW-NativeFreeRoamRacer\n"
            "Release: https://github.com/Zakkey250/MW-NativeFreeRoamRacer/releases/tag/v0.1.2\n"
            f"ZIP original SHA-256: {actual}\n"
            f"Checagem do speed.exe incluida na geracao: {'SIM' if optional_exe else 'NAO'}\n"
        )
        (root / "LEIA-ME-ANTES-PTBR.txt").write_text(guide, encoding="utf-8")
        (root / "HASHES-SHA256.txt").write_text(
            "".join(f"{sha256(p)}  {p.relative_to(root).as_posix()}\n"
                    for p in sorted(root.rglob("*")) if p.is_file()), encoding="ascii"
        )
    return root


def main() -> None:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--upstream-zip", type=pathlib.Path, required=True)
    p.add_argument("--checksums", type=pathlib.Path, required=True)
    p.add_argument("--license", type=pathlib.Path, required=True)
    p.add_argument("--output-dir", type=pathlib.Path, required=True)
    p.add_argument("--speed-exe", type=pathlib.Path)
    args = p.parse_args()
    try:
        stage = create_candidate(args.upstream_zip, args.checksums, args.output_dir,
                                 args.license, args.speed_exe)
    except (NextError, OSError, zipfile.BadZipFile, UnicodeDecodeError) as exc:
        print("NEXT candidate rejected: " + str(exc), file=sys.stderr)
        sys.exit(2)
    print(f"SAFE STAGING ONLY: {stage}; NOT installed into any game; 3 upstream runtime files + notices")


if __name__ == "__main__":
    main()
