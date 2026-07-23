#!/usr/bin/env python3
"""
workflow.py — ITE LVGL Auto Pipeline Orchestrator
===================================================

One-command pipeline from JSON UI design → IT9866 ROM.

Usage:
    python workflow.py examples/gateway_hmi_example.json
    python workflow.py my_ui.json --preview              # MCP validation only
    python workflow.py my_ui.json --skip-build           # generate code, don't compile
    python workflow.py my_ui.json --flash                # also flash after build
    python workflow.py my_ui.json --project-dir D:/myproject/

Pipeline stages:
    Stage 1: assets_pipeline.py — PNG images → LVGL C arrays
    Stage 2: json2lvgl.py       — JSON design → LVGL C code
    Stage 3: CMake + make       — Compile firmware ROM
    Stage 4: (optional) Flash   — USB burn to IT9866

Requirements:
    Python 3.8+ with Pillow (pip install Pillow)
    ITEGCC toolchain at C:\ITEGCC_98x
    ITE SDK project at configured path
    LVGL MCP server (optional, for --preview visual validation)
"""

import argparse
import json
import os
import shutil
import subprocess
import sys
import time
from datetime import datetime
from pathlib import Path


# ═══════════════════════════════════════════════════════════════
# Configuration — adjust these paths to match your environment
# ═══════════════════════════════════════════════════════════════

DEFAULT_SDK_ROOT = r"C:\SW code\source code\ITE9868_GWBuild_2491_20260718"
DEFAULT_PROJECT  = "GW202601_LVGL"
DEFAULT_TOOLCHAIN = r"C:\ITEGCC_98x"
DEFAULT_BUILD_DIR = "build/openrtos/GW202601_LVGL"

# Tool paths (relative to this script — tools are in the same directory)
SCRIPT_DIR = Path(__file__).parent.absolute()
JSON2LVGL = SCRIPT_DIR / "json2lvgl.py"
ASSETS_PIPELINE = SCRIPT_DIR / "assets_pipeline.py"


# ═══════════════════════════════════════════════════════════════
# Stage runners
# ═══════════════════════════════════════════════════════════════

def stage_assets(ui_json_path: str, output_dir: str, sdk_root: str, verbose: bool = False):
    """Stage 1: Convert image assets to LVGL C arrays."""
    print_header("STAGE 1/4: Asset Pipeline (PNG -> C arrays)")

    # Load JSON to extract asset list
    with open(ui_json_path, 'r', encoding='utf-8') as f:
        ui_data = json.load(f)

    # Collect assets from the UI JSON
    assets = collect_assets(ui_data)

    if not assets:
        print("  No image assets found in UI JSON. Skipping.")
        return True

    print(f"  Found {len(assets)} image assets to convert")

    # Create asset output directory
    asset_output = os.path.join(output_dir, "assets")
    os.makedirs(asset_output, exist_ok=True)

    success_count = 0
    for asset in assets:
        filepath = asset.get('file', '')
        if not filepath:
            continue

        # Resolve relative path: relative to the JSON file's directory
        json_dir = os.path.dirname(os.path.abspath(ui_json_path))
        candidate = os.path.join(json_dir, filepath)
        if os.path.exists(candidate):
            filepath = candidate
        elif not os.path.isabs(filepath) and not os.path.exists(filepath):
            print(f"  WARNING: Asset not found: {filepath} (tried {candidate})")
            continue

        fmt = asset.get('format', 'rgb565')
        resize = asset.get('resize', None)
        var_name = asset.get('var_name', None)
        chroma = asset.get('chroma_keyed', False)
        grayscale = asset.get('recolorable', False)
        chroma_color = asset.get('chroma_color', '#FF00FF')

        args = ['python', str(ASSETS_PIPELINE), filepath, '-o', asset_output]
        if fmt == 'argb8888':
            args.append('--argb8888')
        elif fmt == 'alpha8':
            args.append('--alpha8')
        elif grayscale:
            args.append('--grayscale')
        else:
            args.append('--rgb565')
        if resize:
            args.extend(['--resize', resize])
        if chroma:
            args.append('--chroma-key')
            args.extend(['--chroma-color', chroma_color])
        if var_name:
            args.extend(['--var-name', var_name])
        if verbose:
            args.append('-v')

        try:
            result = subprocess.run(args, capture_output=not verbose, text=True)
            if result.returncode == 0:
                success_count += 1
                if verbose:
                    print(result.stdout)
            else:
                print(f"  ERROR converting {asset['file']}: {result.stderr}")
        except Exception as e:
            print(f"  ERROR: {e}")

    print(f"  Converted {success_count}/{len(assets)} assets")
    return success_count == len(assets)


def collect_assets(data: dict) -> list:
    """Recursively collect image assets from UI JSON."""
    assets = []

    def _recurse(obj):
        if isinstance(obj, dict):
            # Check for image src
            src = obj.get('src')
            if src:
                if isinstance(src, dict):
                    for variant_key in ('default', 'error', 'active', 'pressed', 'disabled'):
                        variant = src.get(variant_key)
                        if isinstance(variant, dict) and 'file' in variant:
                            a = dict(variant)
                            a['variant'] = variant_key
                            assets.append(a)
                        elif isinstance(variant, str):
                            assets.append({'file': variant, 'variant': variant_key})
                elif isinstance(src, str):
                    assets.append({'file': src})

            # Check bg_image (with optional resize)
            bg_img = obj.get('bg_image')
            if bg_img:
                a = {'file': bg_img, 'format': 'rgb565'}
                bg_resize = obj.get('bg_image_resize')
                if bg_resize:
                    a['resize'] = bg_resize
                assets.append(a)

            # Recurse children
            for child in obj.get('children', []):
                _recurse(child)

        elif isinstance(obj, list):
            for item in obj:
                _recurse(item)

    _recurse(data.get('screen', {}))
    return assets


def stage_json2lvgl(ui_json_path: str, output_dir: str, verbose: bool = False):
    """Stage 2: Convert JSON UI design to LVGL C code."""
    print_header("STAGE 2/4: JSON -> LVGL C Code (json2lvgl)")

    os.makedirs(output_dir, exist_ok=True)

    args = ['python', str(JSON2LVGL), ui_json_path, '-o', output_dir]

    try:
        result = subprocess.run(args, capture_output=True, text=True)
        if result.returncode == 0:
            out = result.stdout.strip()
            if out:
                print(out)
            return True
        else:
            print(f"  ERROR: {result.stderr}")
            return False
    except Exception as e:
        print(f"  ERROR: {e}")
        return False


def stage_build(project_dir: str, flash: bool = False):
    """Stage 3+4: CMake configure → make → (optional) flash."""
    print_header("STAGE 3/4: Build Firmware (CMake + make)")

    build_dir = os.path.join(project_dir, "build", "openrtos", "GW202601_LVGL")
    project_root = os.path.join(project_dir, "project", "GW202601_LVGL")

    # Verify project exists
    if not os.path.exists(project_root):
        print(f"  WARNING: Project directory not found: {project_root}")
        print(f"  Skipping build. Create the LVGL project first.")
        return False

    # Setup environment
    env = os.environ.copy()
    env['PATH'] = f"{project_dir}\\tool\\bin;{DEFAULT_TOOLCHAIN}\\bin;{env.get('PATH', '')}"
    env['CFG_PROJECT'] = 'GW202601_LVGL'
    env['CFG_PLATFORM'] = 'openrtos'
    env['CFG_BUILDPLATFORM'] = 'openrtos'

    # Step 1: CMake
    print("  Running CMake configure...")
    os.makedirs(build_dir, exist_ok=True)

    cmake_args = [
        'cmake',
        '-G', 'Unix Makefiles',
        f'-DCMAKE_TOOLCHAIN_FILE={project_dir}/openrtos/toolchain.cmake',
        project_dir,
    ]

    try:
        result = subprocess.run(cmake_args, cwd=build_dir, env=env,
                                capture_output=True, text=True)
        if result.returncode != 0:
            print(f"  CMake ERROR:")
            print(result.stderr[-2000:])
            return False
        print("  CMake: OK")
    except Exception as e:
        print(f"  CMake ERROR: {e}")
        return False

    # Step 2: Make
    print("  Running make -j8...")
    try:
        result = subprocess.run(['make', '-j8'], cwd=build_dir, env=env,
                                capture_output=True, text=True)
        if result.returncode != 0:
            print(f"  Make ERROR:")
            # Show last lines of output
            lines = result.stderr.split('\n') if result.stderr else result.stdout.split('\n')
            for line in lines[-30:]:
                print(f"    {line}")
            return False
        print("  Make: OK")

        # Find ROM
        rom_candidates = [
            os.path.join(build_dir, 'project', 'GW202601_LVGL', 'ITE_NOR.ROM'),
            os.path.join(build_dir, 'ITE_NOR.ROM'),
        ]
        rom_path = None
        for c in rom_candidates:
            if os.path.exists(c):
                rom_path = c
                break

        if rom_path:
            # Copy ROM with timestamp
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            rom_dest = os.path.join(project_dir, f"GW202601_LVGL_{timestamp}.ROM")
            shutil.copy(rom_path, rom_dest)
            rom_latest = os.path.join(project_dir, "ITE_NOR.ROM")
            shutil.copy(rom_path, rom_latest)
            size_kb = os.path.getsize(rom_path) / 1024
            print(f"  ROM: {rom_dest} ({size_kb:.0f} KB)")
    except Exception as e:
        print(f"  Make ERROR: {e}")
        return False

    # Step 4 (optional): Flash
    if flash:
        print_header("STAGE 4/4: Flash to IT9866")
        burn_script = os.path.join(project_dir, "burn_rom.py")
        if os.path.exists(burn_script):
            try:
                result = subprocess.run(['python', burn_script], cwd=project_dir,
                                        capture_output=True, text=True)
                if result.returncode == 0:
                    print("  Flash: OK")
                else:
                    print(f"  Flash: returned {result.returncode}")
                    print(result.stdout[-1000:])
            except Exception as e:
                print(f"  Flash ERROR: {e}")
        else:
            print(f"  WARNING: burn_rom.py not found. Flash manually.")

    return True


def stage_preview(ui_json_path: str):
    """Optional: Visual validation via LVGL MCP or statistics."""
    print_header("PREVIEW: UI Statistics")

    args = ['python', str(JSON2LVGL), ui_json_path, '--preview']
    try:
        result = subprocess.run(args, capture_output=True, text=True)
        print(result.stdout)
    except Exception as e:
        print(f"  Preview ERROR: {e}")


# ═══════════════════════════════════════════════════════════════
# UI
# ═══════════════════════════════════════════════════════════════

def print_header(title: str):
    width = 64
    print()
    print("=" * width)
    print(f"  {title}")
    print("=" * width)


def print_banner():
    print("""
  ___ _____ ___   _    ___     __   ___   ___   ___
 |_ _|_   _| __| | |  / __|   / /  / _ \\ |_ _| / __|
  | |  | | | _|  | |__\\__ \\  / _ \\ | (_) | | | | (_ |
 |___| |_| |___| |____|___/  \\___/  \\___/ |___| \\___|

   ITE LVGL Auto Pipeline v1.0
   Target: IT9866/IT9868 with IT2D GPU
""")
    print("   JSON Design  -->  LVGL C Code  -->  ROM\n")


# ═══════════════════════════════════════════════════════════════
# Main
# ═══════════════════════════════════════════════════════════════

def main():
    parser = argparse.ArgumentParser(
        description='ITE LVGL Auto Pipeline — JSON UI design to IT9866 ROM in one command',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python workflow.py examples/gateway_hmi_example.json
  python workflow.py my_ui.json --preview               # Show UI stats + assets
  python workflow.py my_ui.json --skip-build            # Generate code only
  python workflow.py my_ui.json --flash                 # Build + flash to device
  python workflow.py my_ui.json --project-dir D:/myproject/
        """)

    parser.add_argument('ui_json', help='Path to the LVGL UI JSON design file')
    parser.add_argument('--skip-assets', action='store_true', help='Skip asset conversion stage')
    parser.add_argument('--skip-build', action='store_true', help='Skip firmware build stage')
    parser.add_argument('--preview', action='store_true', help='Preview mode: show UI stats only')
    parser.add_argument('--flash', action='store_true', help='Flash ROM to IT9866 after build')
    parser.add_argument('--project-dir', default=DEFAULT_SDK_ROOT,
                        help=f'ITE SDK project root (default: {DEFAULT_SDK_ROOT})')
    parser.add_argument('--output-dir', default=None,
                        help='Output directory for generated code (default: project-dir/output)')
    parser.add_argument('-v', '--verbose', action='store_true', help='Verbose output')

    args = parser.parse_args()

    # Validate input
    if not os.path.exists(args.ui_json):
        print(f"ERROR: UI JSON file not found: {args.ui_json}", file=sys.stderr)
        sys.exit(1)

    # Determine output directory
    if args.output_dir:
        output_dir = args.output_dir
    else:
        output_dir = os.path.join(args.project_dir, "output")

    os.makedirs(output_dir, exist_ok=True)

    print_banner()
    print(f"  UI Design:  {args.ui_json}")
    print(f"  Output Dir: {output_dir}")
    print(f"  SDK Root:   {args.project_dir}")
    print()

    start_time = time.time()

    # Preview mode
    if args.preview:
        stage_preview(args.ui_json)
        return

    # Stage 1: Assets
    if not args.skip_assets:
        ok = stage_assets(args.ui_json, output_dir, args.project_dir, args.verbose)
        if not ok:
            print("\n  WARNING: Some assets failed to convert. Continuing anyway...")
    else:
        print_header("STAGE 1/4: Asset Pipeline (SKIPPED)")

    # Stage 2: JSON → LVGL C code
    ok = stage_json2lvgl(args.ui_json, output_dir, args.verbose)
    if not ok:
        print("\n  FAILED at Stage 2. Aborting.", file=sys.stderr)
        sys.exit(1)

    # Stage 3+4: Build + optional flash
    if not args.skip_build:
        ok = stage_build(args.project_dir, flash=args.flash)
        if not ok:
            print("\n  FAILED at build stage.", file=sys.stderr)
            sys.exit(1)
    else:
        print_header("STAGE 3/4: Build (SKIPPED)")

    # Summary
    elapsed = time.time() - start_time
    print_header("PIPELINE COMPLETE")
    print(f"  Time: {elapsed:.1f}s")
    print(f"  Generated code: {output_dir}")
    print(f"  ROM: {os.path.join(args.project_dir, 'ITE_NOR.ROM')}")
    print()


if __name__ == '__main__':
    main()
