#!/usr/bin/env python3
"""
assets_pipeline.py — PNG/SVG/Image → LVGL C Array Converter for IT9866/IT9868
==============================================================================

Converts image assets to LVGL-compatible C arrays, optimized for IT2D GPU.
Supports batch processing from a manifest file or direct single-file conversion.

Usage:
    python assets_pipeline.py icon.png                          # single file → stdout
    python assets_pipeline.py icon.png -o data/                 # single file → .c/.h
    python assets_pipeline.py --manifest assets.json            # batch from manifest
    python assets_pipeline.py --all --input-dir UI/ --output-dir project/data/
    python assets_pipeline.py bg.png --rgb565 --resize 480x480  # resize + format

Features:
    - PNG/JPEG/BMP → LVGL C array (lv_img_dsc_t)
    - Multiple output formats: RGB565, ARGB8888, ALPHA8, grayscale (recolorable)
    - Automatic resizing, dithering
    - Batch manifest (JSON list of files + options)
    - Integration with json2lvgl.py asset manifest
    - IT2D-optimized: marks GPU-manageable images, chroma key support

Dependencies:
    pip install Pillow
"""

import argparse
import json
import os
import struct
import sys
from pathlib import Path
from typing import Optional

try:
    from PIL import Image, ImageFilter, ImageOps
except ImportError:
    print("ERROR: Pillow is required. Install with: pip install Pillow", file=sys.stderr)
    sys.exit(1)


# ═══════════════════════════════════════════════════════════════
# Color conversion utilities
# ═══════════════════════════════════════════════════════════════

def rgb_to_565(r: int, g: int, b: int) -> int:
    """24-bit RGB → 16-bit RGB565"""
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def rgb_to_8888(r: int, g: int, b: int, a: int) -> int:
    """RGBA → 32-bit ARGB8888 (LVGL format: 0xAARRGGBB in memory)"""
    return (a << 24) | (r << 16) | (g << 8) | b


def rgb_to_grayscale(r: int, g: int, b: int) -> int:
    """RGB → grayscale (for recolorable images — IT2D hardware recolor)"""
    return int(0.299 * r + 0.587 * g + 0.114 * b)


# ═══════════════════════════════════════════════════════════════
# Image loader / preprocessor
# ═══════════════════════════════════════════════════════════════

def load_image(filepath: str, resize: Optional[str] = None,
               grayscale: bool = False, dither: bool = False) -> Image.Image:
    """Load and preprocess an image."""
    img = Image.open(filepath)

    # Convert to RGBA for consistent processing
    if img.mode == 'P':
        img = img.convert('RGBA')
    elif img.mode == 'RGB':
        img = img.convert('RGBA')
    elif img.mode == 'L':
        img = img.convert('RGBA')
    elif img.mode != 'RGBA':
        img = img.convert('RGBA')

    # Resize if requested
    if resize:
        try:
            w, h = map(int, resize.split('x'))
            # Use LANCZOS for high-quality downscaling
            img = img.resize((w, h), Image.LANCZOS)
        except ValueError:
            print(f"WARNING: Invalid resize format '{resize}', expected 'WxH'", file=sys.stderr)

    # Convert to grayscale for recolorable images
    if grayscale:
        # Keep alpha channel, convert RGB to grayscale
        r, g, b, a = img.split()
        gray = ImageOps.grayscale(Image.merge('RGB', (r, g, b)))
        img = Image.merge('RGBA', (gray, gray, gray, a))

    return img


# ═══════════════════════════════════════════════════════════════
# C array generators
# ═══════════════════════════════════════════════════════════════

def generate_rgb565_array(img: Image.Image) -> tuple[bytes, str, int, int]:
    """Convert image to RGB565 byte array. Returns (data, cf_str, w, h)."""
    w, h = img.size
    pixels = img.load()
    data = bytearray()

    for y in range(h):
        for x in range(w):
            r, g, b, a = pixels[x, y]
            if a < 128:
                # Transparent pixels → magenta chroma key (or black)
                r, g, b = 0, 0, 0
            val = rgb_to_565(r, g, b)
            data.extend(struct.pack('<H', val))

    return bytes(data), 'LV_COLOR_FORMAT_RGB565', w, h


def generate_rgb565_chroma_keyed(img: Image.Image,
                                  chroma_r: int = 255, chroma_g: int = 0,
                                  chroma_b: int = 255) -> tuple[bytes, str, int, int]:
    """Convert image to RGB565 with chroma key (magenta = transparent).
       IT2D hardware color_key can then punch out the chroma pixels."""
    w, h = img.size
    pixels = img.load()
    data = bytearray()

    for y in range(h):
        for x in range(w):
            r, g, b, a = pixels[x, y]
            if a < 128:
                r, g, b = chroma_r, chroma_g, chroma_b  # magenta = transparent
            val = rgb_to_565(r, g, b)
            data.extend(struct.pack('<H', val))

    return bytes(data), 'LV_COLOR_FORMAT_RGB565', w, h


def generate_argb8888_array(img: Image.Image) -> tuple[bytes, str, int, int]:
    """Convert image to ARGB8888 byte array."""
    w, h = img.size
    pixels = img.load()
    data = bytearray()

    for y in range(h):
        for x in range(w):
            r, g, b, a = pixels[x, y]
            val = rgb_to_8888(r, g, b, a)
            data.extend(struct.pack('<I', val))

    return bytes(data), 'LV_COLOR_FORMAT_ARGB8888', w, h


def generate_alpha8_array(img: Image.Image) -> tuple[bytes, str, int, int]:
    """Extract alpha channel only (used for masks)."""
    w, h = img.size
    pixels = img.load()
    data = bytearray()

    for y in range(h):
        for x in range(w):
            _, _, _, a = pixels[x, y]
            data.append(a)

    return bytes(data), 'LV_COLOR_FORMAT_A8', w, h


def generate_grayscale_array(img: Image.Image) -> tuple[bytes, str, int, int]:
    """Convert to grayscale RGB565 (for IT2D hardware recolor).
       Preserves luminance so recolor works correctly."""
    w, h = img.size
    pixels = img.load()
    data = bytearray()

    for y in range(h):
        for x in range(w):
            r, g, b, a = pixels[x, y]
            if a < 128:
                r, g, b = 0, 0, 0
            gray = rgb_to_grayscale(r, g, b)
            val = rgb_to_565(gray, gray, gray)
            data.extend(struct.pack('<H', val))

    return bytes(data), 'LV_COLOR_FORMAT_RGB565', w, h


# ═══════════════════════════════════════════════════════════════
# C code writers
# ═══════════════════════════════════════════════════════════════

def format_hex_array(data: bytes, bytes_per_line: int = 16) -> str:
    """Format bytes as a C hex array with line breaks."""
    lines = []
    for i in range(0, len(data), bytes_per_line):
        chunk = data[i:i + bytes_per_line]
        hex_str = ', '.join(f'0x{b:02X}' for b in chunk)
        lines.append(f'    {hex_str},')
    return '\n'.join(lines)


def write_lvgl_image_c(var_name: str, data: bytes, cf_str: str,
                       w: int, h: int, comment: str = "",
                       color_format_name: str = "RGB565") -> str:
    """Generate a complete LVGL image descriptor + data array as C source."""
    lines = []
    lines.append(f'/* {comment} */' if comment else '/* Auto-generated by assets_pipeline.py */')
    lines.append(f'/* Format: {color_format_name}, {w}x{h}, {len(data)} bytes */')
    lines.append(f'')
    lines.append(f'#include <lvgl.h>')
    lines.append(f'')
    lines.append(f'#ifndef LV_ATTRIBUTE_MEM_ALIGN')
    lines.append(f'#define LV_ATTRIBUTE_MEM_ALIGN')
    lines.append(f'#endif')
    lines.append(f'')
    lines.append(f'#ifndef LV_ATTRIBUTE_IMG_{var_name.upper()}')
    lines.append(f'#define LV_ATTRIBUTE_IMG_{var_name.upper()}')
    lines.append(f'#endif')
    lines.append(f'')
    lines.append(f'const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST')
    lines.append(f'LV_ATTRIBUTE_IMG_{var_name.upper()}')
    lines.append(f'uint8_t {var_name}_map[] = {{')
    lines.append(format_hex_array(data))
    lines.append(f'}};')
    lines.append(f'')
    stride = w * 2  # default RGB565 stride
    if 'ARGB8888' in cf_str or 'XRGB8888' in cf_str:
        stride = w * 4
    elif 'A8' in cf_str:
        stride = w
    lines.append(f'const lv_image_dsc_t {var_name} = {{')
    lines.append(f'  .header.magic = LV_IMAGE_HEADER_MAGIC,')
    lines.append(f'  .header.cf = {cf_str},')
    lines.append(f'  .header.flags = 0,')
    lines.append(f'  .header.w = {w},')
    lines.append(f'  .header.h = {h},')
    lines.append(f'  .header.stride = {stride},')
    lines.append(f'  .data_size = {len(data)},')
    lines.append(f'  .data = {var_name}_map,')
    lines.append(f'}};')
    return '\n'.join(lines)


def write_lvgl_header(var_name: str, comment: str = "") -> str:
    """Generate the header declaration for an LVGL image."""
    lines = []
    lines.append(f'/* {comment} */' if comment else f'/* {var_name} — Auto-generated */')
    lines.append(f'#ifndef IMG_{var_name.upper()}_H')
    lines.append(f'#define IMG_{var_name.upper()}_H')
    lines.append(f'')
    lines.append(f'#include <lvgl.h>')
    lines.append(f'')
    lines.append(f'LV_IMG_DECLARE({var_name});')
    lines.append(f'')
    lines.append(f'#endif /* IMG_{var_name.upper()}_H */')
    return '\n'.join(lines)


# ═══════════════════════════════════════════════════════════════
# Single file converter
# ═══════════════════════════════════════════════════════════════

def convert_single(filepath: str, output_dir: str,
                   var_name: str = None,
                   fmt: str = "rgb565",
                   resize: str = None,
                   chroma_keyed: bool = False,
                   grayscale: bool = False,
                   chroma_color: str = "#FF00FF",
                   verbose: bool = False) -> tuple[str, str]:
    """Convert a single image file to LVGL C array. Returns (c_path, h_path)."""

    if not os.path.exists(filepath):
        print(f"ERROR: File not found: {filepath}", file=sys.stderr)
        sys.exit(1)

    # Derive variable name from filename
    if var_name is None:
        base = os.path.splitext(os.path.basename(filepath))[0]
        # Sanitize: replace non-C-identifier chars with underscore
        var_name = "img_" + ''.join(c if c.isalnum() else '_' for c in base)

    # Load and preprocess
    img = load_image(filepath, resize=resize, grayscale=grayscale)
    w, h = img.size

    if verbose:
        print(f"  {os.path.basename(filepath)}: {w}x{h} → {fmt}", file=sys.stderr)

    # Convert to target format
    format_map = {
        "rgb565":        (generate_rgb565_array, "RGB565"),
        "rgb565_chroma": (generate_rgb565_chroma_keyed, "RGB565+ChromaKey"),
        "argb8888":      (generate_argb8888_array, "ARGB8888"),
        "alpha8":        (generate_alpha8_array, "ALPHA8"),
        "grayscale":     (generate_grayscale_array, "Grayscale for Recolor"),
    }

    if chroma_keyed:
        fmt = "rgb565_chroma"

    gen_func, fmt_label = format_map.get(fmt, format_map["rgb565"])

    if chroma_keyed:
        try:
            cr, cg, cb = hex_to_rgb_tuple(chroma_color)
        except:
            cr, cg, cb = 255, 0, 255
        data, cf_str, w_out, h_out = generate_rgb565_chroma_keyed(img, cr, cg, cb)
    elif fmt == "grayscale":
        data, cf_str, w_out, h_out = generate_grayscale_array(img)
    else:
        data, cf_str, w_out, h_out = gen_func(img)

    # Generate .c file
    comment = f"{os.path.basename(filepath)} — {w}x{h} px, {fmt_label}"
    c_code = write_lvgl_image_c(var_name, data, cf_str, w_out, h_out,
                                comment=comment, color_format_name=fmt_label)
    h_code = write_lvgl_header(var_name, comment=comment)

    # Write files
    os.makedirs(output_dir, exist_ok=True)
    c_path = os.path.join(output_dir, f"{var_name}.c")
    h_path = os.path.join(output_dir, f"{var_name}.h")

    with open(c_path, 'w', encoding='utf-8') as f:
        f.write(c_code)
    with open(h_path, 'w', encoding='utf-8') as f:
        f.write(h_code)

    file_size_kb = len(data) / 1024
    if verbose:
        print(f"    → {os.path.basename(c_path)} ({file_size_kb:.1f} KB)", file=sys.stderr)

    return c_path, h_path


# ═══════════════════════════════════════════════════════════════
# Batch / manifest processor
# ═══════════════════════════════════════════════════════════════

def hex_to_rgb_tuple(hex_str: str) -> tuple:
    """#FF00FF → (255, 0, 255)"""
    h = hex_str.lstrip('#')
    return tuple(int(h[i:i+2], 16) for i in (0, 2, 4))


def process_manifest(manifest_path: str, output_dir: str, verbose: bool = False):
    """Process a JSON manifest file listing image assets to convert.

    Manifest format:
    {
      "output_dir": "project/data/",
      "prefix": "img_",
      "assets": [
        {"file": "bg_dark.png", "format": "rgb565"},
        {"file": "icon.png", "format": "argb8888", "var_name": "icon_wifi"},
        {"file": "ring.png", "format": "grayscale", "recolorable": true, "resize": "480x480"},
        {"file": "overlay.png", "format": "rgb565_chroma", "chroma_color": "#FF00FF"}
      ]
    }
    """
    with open(manifest_path, 'r', encoding='utf-8') as f:
        manifest = json.load(f)

    assets = manifest.get('assets', [])
    if not assets:
        print("ERROR: No assets found in manifest.", file=sys.stderr)
        return

    out_dir = manifest.get('output_dir', output_dir)
    prefix = manifest.get('prefix', 'img_')

    print(f"Processing {len(assets)} assets from manifest...")

    results = []
    for i, asset in enumerate(assets):
        filepath = asset['file']
        var_name = asset.get('var_name', None)
        fmt = asset.get('format', 'rgb565')
        resize = asset.get('resize', None)
        chroma_keyed = (fmt in ('rgb565_chroma',))

        c_path, h_path = convert_single(
            filepath=filepath,
            output_dir=out_dir,
            var_name=var_name,
            fmt=fmt if not chroma_keyed else 'rgb565',
            resize=resize,
            chroma_keyed=chroma_keyed,
            grayscale=asset.get('recolorable', False),
            chroma_color=asset.get('chroma_color', '#FF00FF'),
            verbose=verbose,
        )
        results.append({
            'input': filepath,
            'c_file': c_path,
            'h_file': h_path,
            'var_name': var_name or os.path.splitext(os.path.basename(filepath))[0],
        })

    # Generate combined include header for convenience
    if results:
        combined_h = os.path.join(out_dir, 'assets.h')
        with open(combined_h, 'w', encoding='utf-8') as f:
            f.write('/* Auto-generated by assets_pipeline.py — ALL ASSETS */\n')
            f.write('#ifndef ASSETS_H\n#define ASSETS_H\n\n')
            f.write('#include "lvgl/lvgl.h"\n\n')
            for r in results:
                f.write(f'#include "{r["var_name"]}.h"\n')
            f.write('\n#endif /* ASSETS_H */\n')
        print(f"  Generated combined header: {combined_h}")

    print(f"\nDone. {len(results)} assets converted to {out_dir}")


def process_directory(input_dir: str, output_dir: str, fmt: str = "rgb565",
                      verbose: bool = False):
    """Process all PNG/JPG/BMP files in a directory."""
    extensions = ('.png', '.jpg', '.jpeg', '.bmp')
    files = [f for f in os.listdir(input_dir)
             if os.path.splitext(f)[1].lower() in extensions]

    if not files:
        print(f"No image files found in {input_dir}", file=sys.stderr)
        return

    print(f"Found {len(files)} images in {input_dir}")
    for f in sorted(files):
        filepath = os.path.join(input_dir, f)
        try:
            convert_single(filepath, output_dir, fmt=fmt, verbose=verbose)
        except Exception as e:
            print(f"  ERROR converting {f}: {e}", file=sys.stderr)


# ═══════════════════════════════════════════════════════════════
# Manifest generator (from json2lvgl output or directory scan)
# ═══════════════════════════════════════════════════════════════

def generate_manifest_template(input_dir: str, output_file: str):
    """Scan a directory and generate a manifest JSON template."""
    extensions = ('.png', '.jpg', '.jpeg', '.bmp')
    files = sorted([f for f in os.listdir(input_dir)
                    if os.path.splitext(f)[1].lower() in extensions])

    assets = []
    for f in files:
        full_path = os.path.join(input_dir, f)
        try:
            img = Image.open(full_path)
            w, h = img.size
            has_alpha = img.mode in ('RGBA', 'LA') or (img.mode == 'P' and 'transparency' in img.info)

            # Auto-detect best format
            if has_alpha:
                suggested_fmt = 'argb8888'
            else:
                suggested_fmt = 'rgb565'

            assets.append({
                "file": full_path.replace('\\', '/'),
                "format": suggested_fmt,
                "comment": f"{w}x{h} px",
            })
        except Exception:
            assets.append({
                "file": full_path.replace('\\', '/'),
                "format": "rgb565",
                "comment": "unknown size",
            })

    manifest = {
        "output_dir": "project/GW202601_LVGL/data/",
        "prefix": "img_",
        "assets": assets,
    }

    with open(output_file, 'w', encoding='utf-8') as f:
        json.dump(manifest, f, indent=2, ensure_ascii=False)

    print(f"Manifest template written to {output_file}")
    print(f"  {len(assets)} assets detected")
    print(f"  Review formats and add 'recolorable' / 'resize' / 'var_name' as needed.")


# ═══════════════════════════════════════════════════════════════
# CLI
# ═══════════════════════════════════════════════════════════════

def main():
    parser = argparse.ArgumentParser(
        description='assets_pipeline — Convert images to LVGL C arrays for IT9866/IT9868',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python assets_pipeline.py icon.png                                    # stdout
  python assets_pipeline.py icon.png -o data/                           # write .c/.h
  python assets_pipeline.py bg.png --rgb565 --resize 480x480 -o data/
  python assets_pipeline.py ring.png --grayscale -o data/               # recolorable
  python assets_pipeline.py overlay.png --chroma-key -o data/
  python assets_pipeline.py --manifest assets.json -o project/data/
  python assets_pipeline.py --all --input-dir UI/raw/ -o project/data/
  python assets_pipeline.py --gen-manifest UI/raw/ -o assets.json       # scan dir
        """)

    parser.add_argument('input', nargs='?', help='Input image file')
    parser.add_argument('-o', '--output-dir', default='.', help='Output directory for .c/.h files')

    # Format options
    fmt_group = parser.add_argument_group('Output Format')
    fmt_group.add_argument('--rgb565', dest='fmt', action='store_const', const='rgb565',
                           default='rgb565', help='RGB565 (16-bit, default, no alpha)')
    fmt_group.add_argument('--argb8888', dest='fmt', action='store_const', const='argb8888',
                           help='ARGB8888 (32-bit with alpha)')
    fmt_group.add_argument('--alpha8', dest='fmt', action='store_const', const='alpha8',
                           help='Alpha channel only (8-bit mask)')
    fmt_group.add_argument('--grayscale', dest='fmt', action='store_const', const='grayscale',
                           help='Grayscale RGB565 for IT2D hardware recolor')

    # Image options
    img_group = parser.add_argument_group('Image Processing')
    img_group.add_argument('--resize', metavar='WxH', help='Resize image (e.g. 480x480)')
    img_group.add_argument('--chroma-key', action='store_true', help='Use chroma key for transparency')
    img_group.add_argument('--chroma-color', default='#FF00FF', help='Chroma key color (default: magenta)')
    img_group.add_argument('--var-name', help='Override variable name (default: img_<filename>)')

    # Batch mode
    batch_group = parser.add_argument_group('Batch Processing')
    batch_group.add_argument('--manifest', help='JSON manifest file listing assets to convert')
    batch_group.add_argument('--all', action='store_true', help='Process all images in --input-dir')
    batch_group.add_argument('--input-dir', help='Input directory for batch mode')
    batch_group.add_argument('--gen-manifest', action='store_true', help='Generate manifest template from --input-dir')

    parser.add_argument('-v', '--verbose', action='store_true', help='Verbose output')

    args = parser.parse_args()

    # Manifest template generation
    if args.gen_manifest:
        if not args.input_dir:
            print("ERROR: --gen-manifest requires --input-dir", file=sys.stderr)
            sys.exit(1)
        output_file = args.output_dir
        if os.path.isdir(output_file):
            output_file = os.path.join(output_file, 'assets_manifest.json')
        generate_manifest_template(args.input_dir, output_file)
        return

    # Batch from manifest
    if args.manifest:
        process_manifest(args.manifest, args.output_dir, verbose=args.verbose)
        return

    # Batch from directory
    if args.all:
        if not args.input_dir:
            print("ERROR: --all requires --input-dir", file=sys.stderr)
            sys.exit(1)
        process_directory(args.input_dir, args.output_dir, fmt=args.fmt, verbose=args.verbose)
        return

    # Single file mode
    if not args.input:
        parser.print_help()
        sys.exit(1)

    convert_single(
        filepath=args.input,
        output_dir=args.output_dir,
        var_name=args.var_name,
        fmt=args.fmt,
        resize=args.resize,
        chroma_keyed=args.chroma_key,
        grayscale=(args.fmt == 'grayscale'),
        chroma_color=args.chroma_color,
        verbose=args.verbose,
    )


if __name__ == '__main__':
    main()
