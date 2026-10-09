#!/usr/bin/env python3
"""Collect bounded FRR evidence locally. Never includes game binaries or saves."""
import argparse
from collections import Counter
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import zipfile

MAX_LOG = 8 * 1024 * 1024
MAX_CONFIG = 256 * 1024
MAX_EXCEPTION_TRACE = 128 * 1024
MAX_HASH_BYTES = 64 * 1024 * 1024
MAX_PLUGINS = 256
CONFIGS = ('FreeRoamRivals.ini', 'Rivals.ini', 'VehiclePools.ini', 'UndergroundBlacklist.ini')
SUPPORTED_MD5 = 'c0516b485065fabdd69579816b5df763'


def summarize(text):
    mismatches = []
    for line in text.splitlines():
        if 'compatibility audit address=' in line and 'match=0' in line:
            mismatches.append(line.split('NativeFactory ', 1)[-1])
    blocks = Counter(re.findall(r'NativePrototype blocked=([^\s]+)', text))
    versions = re.findall(r'NFSMW Free Roam Rivals v([^\s]+) loaded', text)
    advice = []
    if mismatches or 'compatibility blocked address=' in text:
        advice.append('Criação bloqueada por compatibilidade: analisar janelas/destinos e inventário antes de alterar assinaturas.')
    if 'DiagnosticBundle F9 begin' not in text:
        advice.append('Ativar DiagnosticBundleEnabled=1 e obter uma captura F9 para módulos/código/contexto.')
    if 'NativeFactory constructor invoke' not in text:
        advice.append('Não há chamada do construtor registrada neste trecho; solicitações não provam criação.')
    if 'NativePrototype stage=active' in text:
        advice.append('Ativação nativa registrada; presença visual, direção pela IA e duração ainda exigem observação no jogo.')
    if 'drive_to_obtain_metric_calibration' in text:
        advice.append('Obter calibração dirigindo de forma estável; não assumir escala de distância.')
    return dict(versions=versions[-3:], compatibility_mismatches=mismatches,
                blockers=dict(blocks), constructor_invocations=text.count('NativeFactory constructor invoke'),
                activation_records=text.count('NativePrototype stage=active'),
                f9_records=text.count('DiagnosticBundle F9 begin'), next_checks=advice,
                scope='Somente trecho coletado; ausência no trecho não prova ausência na sessão completa.')


def inside(path, root):
    return not path.is_symlink() and path.resolve().is_relative_to(root)


def fingerprint(path, limit):
    sha, md5 = hashlib.sha256(), hashlib.md5()
    with path.open('rb') as stream:
        remaining = limit
        while remaining:
            chunk = stream.read(min(1024 * 1024, remaining))
            if not chunk:
                break
            sha.update(chunk); md5.update(chunk); remaining -= len(chunk)
        changed = remaining != 0 or bool(stream.read(1))
    return dict(bytes_hashed=limit - remaining, changed_during_read=changed,
                sha256=sha.hexdigest(), md5=md5.hexdigest())


def collect(game_dir, output):
    root = Path(game_dir).resolve()
    if not root.is_dir():
        raise ValueError('Pasta do jogo inexistente')
    exe = root / 'speed.exe'
    if not exe.is_file() or not inside(exe, root):
        raise ValueError('Informe a pasta que contém speed.exe (sem link simbólico)')
    manifest = dict(schema='FRR_DIAGNOSTIC_V1', generated_utc=datetime.now(timezone.utc).isoformat(),
                    files=[], warnings=[], binaries_included=False, saves_included=False)
    total = 0
    candidates = [exe]
    directories = [root]
    scripts = root / 'scripts'
    if scripts.is_dir() and inside(scripts, root):
        directories.append(scripts)
        directories.extend(p for p in scripts.iterdir() if p.is_dir() and inside(p, root))
    plugins = sorted({p for d in directories for p in d.iterdir()
                      if p.is_file() and p.suffix.lower() in ('.dll', '.asi') and inside(p, root)})
    if len(plugins) > MAX_PLUGINS:
        manifest['warnings'].append('Inventário de plugins limitado a 256 arquivos.')
    candidates.extend(plugins[:MAX_PLUGINS])
    for path in candidates:
        item = dict(path=path.relative_to(root).as_posix(), size=path.stat().st_size, included=False)
        if total + item['size'] <= MAX_HASH_BYTES:
            item.update(fingerprint(path, item['size'])); total += item['size']
        else:
            item['hash_skipped'] = 'Limite total de leitura de binários: 64 MiB'
        manifest['files'].append(item)
    exe_info = manifest['files'][0]
    manifest['supported_executable'] = exe_info.get('md5') == SUPPORTED_MD5 and exe_info['size'] == 6029312 and not exe_info.get('changed_during_read', True)
    folder = scripts / 'FreeRoamRivals'
    log = folder / 'FreeRoamRivals.log'
    entries = {}
    log_text = ''
    if log.is_file() and inside(log, root):
        size = log.stat().st_size
        with log.open('rb') as stream:
            start = max(0, size - MAX_LOG); stream.seek(start); data = stream.read(MAX_LOG)
        if start and b'\n' in data:
            data = data.split(b'\n', 1)[1]
        entries['FreeRoamRivals.log.tail'] = data
        log_text = data.decode('utf-8', errors='replace')
        manifest['log'] = dict(observed_size=size, collected_bytes=len(data), truncated=start > 0,
                               collected_sha256=hashlib.sha256(data).hexdigest())
    else:
        manifest['warnings'].append('FreeRoamRivals.log não encontrado; captura F9 ainda necessária.')
    exception_trace = folder / 'NativeExceptions.log'
    if exception_trace.is_file() and inside(exception_trace, root):
        size = exception_trace.stat().st_size
        if size <= MAX_EXCEPTION_TRACE:
            with exception_trace.open('rb') as stream:
                data = stream.read(MAX_EXCEPTION_TRACE)
            entries['NativeExceptions.log'] = data
            manifest['exception_trace'] = dict(observed_size=size, collected_bytes=len(data),
                collected_sha256=hashlib.sha256(data).hexdigest(),
                scope='First-chance observations; not proof of a fatal crash. Latest launch replaces this trace.')
        else:
            manifest['warnings'].append('NativeExceptions.log excede 128 KiB; não incluído.')
    for name in CONFIGS:
        path = folder / name
        if path.is_file() and inside(path, root):
            if path.stat().st_size <= MAX_CONFIG:
                with path.open('rb') as stream:
                    data = stream.read(MAX_CONFIG)
                entries['config/' + name] = data
            else:
                manifest['warnings'].append(name + ' excede 256 KiB; não incluído.')
    manifest['summary'] = summarize(log_text)
    manifest['summary']['scope'] += ' Log limitado a 8 MiB; módulos presentes no disco não provam carregamento.'
    entries['diagnostic_report.json'] = json.dumps(manifest, ensure_ascii=False, indent=2).encode('utf-8')
    with zipfile.ZipFile(output, 'x', compression=zipfile.ZIP_DEFLATED) as archive:
        for name, data in entries.items():
            archive.writestr(name, data)
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('game_dir', help='Pasta que contém speed.exe')
    parser.add_argument('-o', '--output', help='ZIP novo; nunca sobrescreve arquivo existente')
    args = parser.parse_args()
    output = Path(args.output) if args.output else Path(args.game_dir) / ('FreeRoamRivals-Diagnostic-' + datetime.now().strftime('%Y%m%d-%H%M%S') + '.zip')
    try:
        report = collect(args.game_dir, output)
    except (OSError, ValueError) as error:
        parser.error(str(error))
    print('Criado:', output)
    print('Executável suportado:', report['supported_executable'])
    print('Inclui relatório, INIs do FRR, até 8 MiB finais do log e NativeExceptions.log quando disponível (até 128 KiB). Não inclui exe, plugins ou saves.')
    for note in report['summary']['next_checks']:
        print('-', note)


if __name__ == '__main__':
    main()
