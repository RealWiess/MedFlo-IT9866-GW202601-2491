#!/usr/bin/env python3
"""pkgtool wrapper — translates new-style options to V3.2.5-compatible syntax"""
import subprocess, sys, os

PKGTOOL = os.path.join(os.path.dirname(__file__), "pkgtool.exe")
args = sys.argv[1:]

# Map: new option → (old option template, value processor)
# Unformatted device 1 = NOR
device = None
unformatted_size = None
unformatted_data = {}  # index → (pos, file)
partitions = {}  # index → (size, dir)
version = "1.0"
key = "0"
output_file = None
cmd = None
extra_args = []

i = 0
while i < len(args):
    a = args[i]
    if a == "--unformatted-device":
        device = args[i+1]; i += 2; continue
    if a == "--unformatted-size":
        unformatted_size = args[i+1]; i += 2; continue
    if a == "--unformatted-index":
        idx = args[i+1]
        data = {}
        while i+2 < len(args) and args[i+2].startswith("--unformatted-"):
            if args[i+2] == "--unformatted-data":
                data['file'] = args[i+3]; i += 2
            elif args[i+2] == "--unformatted-pos":
                data['pos'] = args[i+3]; i += 2
            else:
                break
        unformatted_data[idx] = data
        i += 2; continue
    if a == "--partition-device":
        idx = args[i+1]
        part = {}
        while i+2 < len(args) and args[i+2].startswith("--partition-"):
            if args[i+2] == "--partition-index":
                part['idx'] = args[i+3]; i += 2
            elif args[i+2] == "--partition-dir":
                part['dir'] = args[i+3]; i += 2
            elif args[i+2] == "--partition-size":
                part['size'] = args[i+3]; i += 2
            else:
                break
        if 'idx' in part:
            partitions[part['idx']] = part
        i += 2; continue
    if a == "--version":
        version = args[i+1]; i += 2; continue
    if a == "--key":
        key = args[i+1]; i += 2; continue
    if a == "-o":
        output_file = args[i+1]; i += 2; continue
    if a == "-l":
        cmd = 'list'
        output_file = args[i+1]; i += 2; continue
    if a == "-s":
        extra_args += [a, args[i+1]]; i += 2; continue
    if a == "-n" or a == "-m":
        cmd = 'make_image'
        extra_args += [a, args[i+1]]; i += 2; continue
    if a == "--partition":
        i += 1; continue
    i += 1

# Build old-style command
if cmd == 'list':
    new_args = [PKGTOOL, '-l', output_file, '--key', key]
else:
    new_args = [PKGTOOL]
    if output_file:
        new_args += ['-o', output_file]

    # Unformatted data (NOR)
    if device == '1':
        if unformatted_size and unformatted_size != '0':
            new_args += ['--nor-unformatted-size', unformatted_size]
        for idx, d in sorted(unformatted_data.items()):
            n = int(idx)
            if n == 0:
                new_args += ['--nor-unformatted-data0', d.get('file', '')]
            elif n == 1:
                if d.get('pos'):
                    new_args += ['--nor-unformatted-data1-pos', d['pos']]
                new_args += ['--nor-unformatted-data1', d.get('file', '')]
            elif n == 2:
                if d.get('pos'):
                    new_args += ['--nor-unformatted-data2-pos', d['pos']]
                new_args += ['--nor-unformatted-data2', d.get('file', '')]

    # Partitions (NOR)
    for idx in ['0', '1', '2', '3']:
        p = partitions.get(idx, {})
        size = p.get('size', '0')
        dr = p.get('dir', '')
        n = int(idx)
        if n == 0:
            new_args += ['--nor-partiton0-size', size]
            if dr: new_args += ['--nor-partiton0-dir', dr]
        elif n == 1:
            new_args += ['--nor-partiton1-size', size]
            if dr: new_args += ['--nor-partiton1-dir', dr]
        elif n == 2:
            new_args += ['--nor-partiton2-size', size]
            if dr: new_args += ['--nor-partiton2-dir', dr]
        elif n == 3:
            new_args += ['--nor-partiton3-size', size]
            if dr: new_args += ['--nor-partiton3-dir', dr]

    new_args += ['--key', key, '--version', version]
    new_args += extra_args

# Run
print(f"[pkgtool_wrapper] {' '.join(new_args)}", file=sys.stderr)
sys.exit(subprocess.call(new_args))
