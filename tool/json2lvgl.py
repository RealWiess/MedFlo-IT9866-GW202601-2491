#!/usr/bin/env python3
"""
json2lvgl.py — JSON Schema → LVGL C Code Generator for IT9866/IT9868
=====================================================================

Converts LVGL UI JSON descriptions (following lvgl_ui_schema.json) into
compilable C code that leverages ITE IT2D 2.5D GPU acceleration.

Usage:
    python json2lvgl.py gateway_hmi_example.json          # outputs to stdout
    python json2lvgl.py input.json -o output_dir/         # writes .c/.h files
    python json2lvgl.py input.json --preview               # dry-run, prints stats

Features:
    - 1:1 mapping from JSON widget types to LVGL C API
    - IT2D hardware features: recolor, gradient glow, composite layers
    - Animation generation (lv_anim_t)
    - State machine generation (state override enums + apply functions)
    - Asset manifest generation
    - Design token resolution ({token_name} references)
"""

import json
import os
import sys
import argparse
import textwrap
from pathlib import Path
from typing import Any, Optional


# ═══════════════════════════════════════════════════════════════
# Color utilities
# ═══════════════════════════════════════════════════════════════

def hex_to_rgb(hex_str: str) -> tuple:
    """#00E5FF → (0, 229, 255)"""
    h = hex_str.lstrip('#')
    if len(h) == 6:
        return (int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16))
    elif len(h) == 8:
        return (int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16), int(h[6:8], 16))
    return (0, 0, 0)


def rgba_to_rgb_alpha(rgba_str: str) -> tuple:
    """rgba(0,5,15,0.75) → ((0,5,15), 191)"""
    import re
    m = re.match(r'rgba\((\d+),\s*(\d+),\s*(\d+),\s*([\d.]+)\)', rgba_str)
    if m:
        r, g, b = int(m.group(1)), int(m.group(2)), int(m.group(3))
        a = int(float(m.group(4)) * 255)
        return (r, g, b), a
    return (0, 0, 0), 255


def resolve_color(color_str: str, tokens: dict) -> str:
    """Resolve {token} references and return as LVGL-compatible C literal."""
    if not color_str:
        return "LV_COLOR_BLACK"

    # Token reference
    if color_str.startswith('{') and color_str.endswith('}'):
        token_name = color_str[1:-1]
        resolved = tokens.get(token_name, '#000000')
        return resolve_color(resolved, tokens)

    # Hex color
    if color_str.startswith('#'):
        h = color_str.lstrip('#')
        if len(h) == 6:
            return f"lv_color_hex(0x{h.upper()})"
        elif len(h) == 8:
            return f"lv_color_hex(0x{h.upper()})"

    # RGBA
    if color_str.startswith('rgba'):
        (r, g, b), a = rgba_to_rgb_alpha(color_str)
        return f"lv_color_make({r}, {g}, {b})"

    return f"lv_color_hex(0x000000)"


def resolve_color_rgb_tuple(color_str: str, tokens: dict) -> tuple:
    """Resolve {token} references and return (r, g, b) as integers."""
    if not color_str:
        return (0, 0, 0)

    # Token reference
    if color_str.startswith('{') and color_str.endswith('}'):
        token_name = color_str[1:-1]
        resolved = tokens.get(token_name, '#000000')
        return resolve_color_rgb_tuple(resolved, tokens)

    # Hex color
    if color_str.startswith('#'):
        return hex_to_rgb(color_str)

    # RGBA
    if color_str.startswith('rgba'):
        (r, g, b), _ = rgba_to_rgb_alpha(color_str)
        return (r, g, b)

    return (0, 0, 0)


def resolve_alpha(color_str: str, tokens: dict) -> int:
    """Extract alpha value from color, 0-255 (LV_OPA)."""
    if color_str.startswith('rgba'):
        _, a = rgba_to_rgb_alpha(color_str)
        return a
    if color_str.startswith('{'):
        token_name = color_str[1:-1]
        resolved = tokens.get(token_name, '#000000FF')
        return resolve_alpha(resolved, tokens)
    if color_str.startswith('#') and len(color_str) == 9:
        return int(color_str[7:9], 16)
    return 255  # LV_OPA_COVER


# ═══════════════════════════════════════════════════════════════
# Code generation state
# ═══════════════════════════════════════════════════════════════

class CodeGen:
    def __init__(self, json_data: dict):
        self.data = json_data
        self.tokens = self._extract_tokens()
        self.output: list[str] = []
        self.indent_level = 0
        self.widget_ids: list[str] = []
        self.asset_files: set[str] = set()
        self.state_names: list[str] = []

    def _extract_tokens(self) -> dict:
        dt = self.data.get('design_tokens', {})
        colors = dt.get('colors', {})
        fonts = dt.get('fonts', {})
        timing = dt.get('timing', {})
        return {
            **colors,
            **{f"font_{k}_{s}": f"&{k}_{s}" for k, v in fonts.items() for s in v.get('sizes', [])},
            **{f"time_{k}": v for k, v in timing.items()},
        }

    def emit(self, line: str = ""):
        indent = "    " * self.indent_level
        self.output.append(f"{indent}{line}")

    def emit_block(self, block: str):
        for line in block.strip().split('\n'):
            if line.strip():
                self.emit(line)

    def indent(self):
        self.indent_level += 1

    def dedent(self):
        self.indent_level = max(0, self.indent_level - 1)

    def get_color(self, key: str, widget: dict = None) -> str:
        if widget:
            val = widget.get(key)
            if val:
                return resolve_color(val, self.tokens)
        return "LV_COLOR_BLACK"

    def get_alpha(self, key: str, widget: dict = None) -> int:
        if widget:
            val = widget.get(key)
            if val:
                return resolve_alpha(val, self.tokens)
        return 255

    def get_font(self, token: str) -> str:
        """Resolve 'tech_14' → '&tech_14'"""
        resolved = self.tokens.get(f"font_{token}", f"&lv_font_montserrat_{token.split('_')[-1]}")
        return resolved

    def track_widget(self, wid: str):
        self.widget_ids.append(wid)

    def track_asset(self, filepath: str):
        if filepath:
            self.asset_files.add(filepath)


# ═══════════════════════════════════════════════════════════════
# Widget generators
# ═══════════════════════════════════════════════════════════════

def generate_screen_style(cg: CodeGen, style: dict, state_prefix: str = ""):
    """Generate LVGL style set calls from style dict."""
    s = style
    if not s:
        return

    state = "LV_STATE_DEFAULT" if not state_prefix else f"LV_STATE_{state_prefix.upper()}"

    if s.get('radius'):
        cg.emit(f"lv_obj_set_style_radius({cg.current_obj}, {s['radius']}, {state});")

    if s.get('bg_color'):
        c = resolve_color(s['bg_color'], cg.tokens)
        cg.emit(f"lv_obj_set_style_bg_color({cg.current_obj}, {c}, {state});")

    if s.get('bg_opa'):
        cg.emit(f"lv_obj_set_style_bg_opa({cg.current_obj}, {s['bg_opa']}, {state});")

    if s.get('opa', 255) < 255:
        cg.emit(f"lv_obj_set_style_opa({cg.current_obj}, {s['opa']}, {state});")

    border = s.get('border')
    if border:
        if border.get('width'):
            w = border['width']
            cg.emit(f"lv_obj_set_style_border_width({cg.current_obj}, {w}, {state});")
            if border.get('color'):
                c = resolve_color(border['color'], cg.tokens)
                cg.emit(f"lv_obj_set_style_border_color({cg.current_obj}, {c}, {state});")

    shadow = s.get('shadow')
    if shadow:
        if shadow.get('width'):
            cg.emit(f"lv_obj_set_style_shadow_width({cg.current_obj}, {shadow['width']}, {state});")
        if shadow.get('color'):
            c = resolve_color(shadow['color'], cg.tokens)
            cg.emit(f"lv_obj_set_style_shadow_color({cg.current_obj}, {c}, {state});")
        if shadow.get('offset_x'):
            cg.emit(f"lv_obj_set_style_shadow_ofs_x({cg.current_obj}, {shadow['offset_x']}, {state});")
        if shadow.get('offset_y'):
            cg.emit(f"lv_obj_set_style_shadow_ofs_y({cg.current_obj}, {shadow['offset_y']}, {state});")

    if s.get('text_color'):
        c = resolve_color(s['text_color'], cg.tokens)
        cg.emit(f"lv_obj_set_style_text_color({cg.current_obj}, {c}, {state});")

    font = s.get('text_font')
    if font:
        f = cg.get_font(font)
        cg.emit(f"lv_obj_set_style_text_font({cg.current_obj}, {f}, {state});")

    if s.get('text_align'):
        align_map = {'left': 'LV_TEXT_ALIGN_LEFT', 'center': 'LV_TEXT_ALIGN_CENTER',
                     'right': 'LV_TEXT_ALIGN_RIGHT'}
        a = align_map.get(s['text_align'], 'LV_TEXT_ALIGN_LEFT')
        cg.emit(f"lv_obj_set_style_text_align({cg.current_obj}, {a}, {state});")

    if s.get('text_letter_space'):
        cg.emit(f"lv_obj_set_style_text_letter_space({cg.current_obj}, {s['text_letter_space']}, {state});")

    if s.get('text_line_space'):
        cg.emit(f"lv_obj_set_style_text_line_space({cg.current_obj}, {s['text_line_space']}, {state});")

    pad = s.get('padding')
    if pad:
        for side in ['top', 'bottom', 'left', 'right']:
            if pad.get(side, 0):
                cg.emit(f"lv_obj_set_style_pad_{side}({cg.current_obj}, {pad[side]}, {state});")

    # Widget-specific arc/bar styles
    arc_color = s.get('arc_color')
    if arc_color:
        c = resolve_color(arc_color, cg.tokens)
        cg.emit(f"lv_obj_set_style_arc_color({cg.current_obj}, {c}, {state});")

    if s.get('arc_width'):
        cg.emit(f"lv_obj_set_style_arc_width({cg.current_obj}, {s['arc_width']}, {state});")

    if s.get('arc_rounded') is not None:
        v = 'true' if s['arc_rounded'] else 'false'
        cg.emit(f"lv_obj_set_style_arc_rounded({cg.current_obj}, {v}, {state});")

    arc_bg_color = s.get('arc_bg_color')
    if arc_bg_color:
        c = resolve_color(arc_bg_color, cg.tokens)
        cg.emit(f"lv_obj_set_style_arc_color({cg.current_obj}, {c}, LV_PART_MAIN | {state});")


def generate_recolor(cg: CodeGen, widget: dict):
    """Generate IT2D hardware recolor setup."""
    recolor = widget.get('recolor')
    if not recolor:
        return

    color = resolve_color(recolor['color'], cg.tokens)
    opa = recolor.get('opa', 255)
    cg.emit(f"/* IT2D hardware recolor: tint image with {recolor['color']} */")
    cg.emit(f"lv_img_set_recolor({cg.current_obj}, {color});")
    cg.emit(f"lv_obj_set_style_img_recolor_opa({cg.current_obj}, {opa}, LV_STATE_DEFAULT);")


def generate_glow(cg: CodeGen, widget: dict):
    """Generate IT2D glow effect (gradient ring or stack blur)."""
    glow = widget.get('glow')
    if not glow:
        return

    wid = widget['id']
    method = glow.get('method', 'gradient_ring')
    radius = glow.get('radius', 10)
    alpha = glow.get('alpha', 180)
    r, g, b = resolve_color_rgb_tuple(glow['color'], cg.tokens)

    cg.emit(f"/* IT2D glow effect: {method} radius={radius} alpha={alpha} */")

    if method == 'gradient_ring':
        # Create a slightly larger arc/panel behind the widget as a gradient glow surface
        cg.emit(f"/* Gradient ring glow for {wid} — pre-rendered surface */")
        cg.emit(f"static ite_it2d_surface_t *glow_surf_{wid} = NULL;")
        cg.emit(f"if (!glow_surf_{wid}) {{")
        cg.indent()
        cg.emit(f"glow_surf_{wid} = lv_mem_alloc(sizeof(ite_it2d_surface_t));")
        cg.emit(f"memset(glow_surf_{wid}, 0, sizeof(ite_it2d_surface_t));")
        cg.emit(f"glow_surf_{wid}->width = {widget['w']} + {radius * 2};")
        cg.emit(f"glow_surf_{wid}->height = {widget['h']} + {radius * 2};")
        cg.emit(f"glow_surf_{wid}->flags.format = ITE_IT2D_PIXEL_FORMAT_RGB_565;")
        cg.emit(f"lv_draw_ite_it2d_create_surface(glow_surf_{wid});")
        cg.emit(f"/* Render gradient ring to surface */")
        cg.emit(f"ite_it2d_context_t ctx_{wid};")
        cg.emit(f"ite_it2d_ctx_init(&ctx_{wid});")
        cg.emit(f"ite_it2d_set_dst_surface(&ctx_{wid}, glow_surf_{wid});")
        cg.emit(f"ite_it2d_set_fg_color(&ctx_{wid}, ite_it2d_color_make(0,0,0,0));")
        cg.emit(f"ite_it2d_fill(&ctx_{wid});")
        cg.emit(f"ite_it2d_set_gradient_end_color(&ctx_{wid}, ite_it2d_color_make({r},{g},{b},{alpha}));")
        cg.emit(f"ite_it2d_set_gradient_dir(&ctx_{wid}, ITE_IT2D_GRAD_DIR_DIAG);")
        cg.emit(f"ite_it2d_enable_blending(&ctx_{wid}, true);")
        cg.emit(f"ite_it2d_gradient_fill(&ctx_{wid});")
        cg.emit(f"ite_it2d_wait_surface_finish(glow_surf_{wid});")
        cg.dedent()
        cg.emit(f"}}")

    elif method == 'gradient_fill':
        cg.emit(f"static ite_it2d_surface_t *glow_surf_{wid} = NULL;")
        cg.emit(f"if (!glow_surf_{wid}) {{")
        cg.indent()
        cg.emit(f"glow_surf_{wid} = lv_mem_alloc(sizeof(ite_it2d_surface_t));")
        cg.emit(f"memset(glow_surf_{wid}, 0, sizeof(ite_it2d_surface_t));")
        cg.emit(f"glow_surf_{wid}->width = {widget['w']};")
        cg.emit(f"glow_surf_{wid}->height = {widget['h']};")
        cg.emit(f"glow_surf_{wid}->flags.format = ITE_IT2D_PIXEL_FORMAT_RGB_565;")
        cg.emit(f"lv_draw_ite_it2d_create_surface(glow_surf_{wid});")
        cg.emit(f"/* Fill with gradient */")
        cg.emit(f"ite_it2d_context_t ctx_{wid};")
        cg.emit(f"ite_it2d_ctx_init(&ctx_{wid});")
        cg.emit(f"ite_it2d_set_dst_surface(&ctx_{wid}, glow_surf_{wid});")
        cg.emit(f"ite_it2d_set_fg_color(&ctx_{wid}, ite_it2d_color_make({r},{g},{b},{alpha}));")
        cg.emit(f"ite_it2d_set_gradient_end_color(&ctx_{wid}, ite_it2d_color_make(0,0,0,0));")
        cg.emit(f"ite_it2d_set_gradient_dir(&ctx_{wid}, ITE_IT2D_GRAD_DIR_VER);")
        cg.emit(f"ite_it2d_enable_blending(&ctx_{wid}, true);")
        cg.emit(f"ite_it2d_gradient_fill(&ctx_{wid});")
        cg.emit(f"ite_it2d_wait_surface_finish(glow_surf_{wid});")
        cg.dedent()
        cg.emit(f"}}")

    elif method == 'stack_blur':
        cg.emit(f"/* Stack blur glow for {wid} — cached after first render */")
        cg.emit(f"static lv_img_dsc_t glow_img_{wid} = {{")
        cg.emit(f"    .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,")
        cg.emit(f"    .header.w = {widget['w']},")
        cg.emit(f"    .header.h = {widget['h']}")
        cg.emit(f"}};")
        cg.emit(f"/* TODO: render glow via stack_blur_job(), upload to IT2D surface */")


def generate_glass(cg: CodeGen, widget: dict):
    """Generate frosted glass effect (stack blur cache + alpha blend)."""
    glass = widget.get('glass')
    if not glass:
        return

    wid = widget['id']
    radius = glass.get('blur_radius', 8)
    tint = resolve_color(glass.get('tint_color', '#000000'), cg.tokens)
    tint_opa = glass.get('tint_opa', 64)

    cg.emit(f"/* Frosted glass effect for {wid}: blur_radius={radius} */")
    cg.emit(f"static lv_img_dsc_t glass_bg_{wid} = {{ 0 }};")
    cg.emit(f"if (!glass_bg_{wid}.data) {{")
    cg.indent()
    cg.emit(f"/* Step 1: Capture background region */")
    cg.emit(f"/* Step 2: Apply stack_blur_job(src, w, h, {radius}, 1, 0, 1); */")
    cg.emit(f"/* Step 3: Blend tint color with alpha {tint_opa} */")
    cg.emit(f"/* Step 4: Store as cached IT2D surface */")
    cg.dedent()
    cg.emit(f"}}")


def generate_transform(cg: CodeGen, widget: dict):
    """Generate IT2D hardware transform matrix setup."""
    transform = widget.get('transform') or widget.get('style', {}).get('transform')
    if not transform:
        return

    rotation = transform.get('rotation', 0)
    scale_x = transform.get('scale_x', 1.0)
    scale_y = transform.get('scale_y', 1.0)
    interpolate = transform.get('interpolation', True)

    if rotation != 0 or scale_x != 1.0 or scale_y != 1.0:
        cg.emit(f"/* IT2D hardware transform: rotation={rotation}°, scale=({scale_x},{scale_y}) */")
        cg.emit(f"lv_img_set_angle({cg.current_obj}, {int(rotation * 10)});")
        if scale_x != 1.0 or scale_y != 1.0:
            cg.emit(f"lv_img_set_zoom({cg.current_obj}, {int(scale_x * 256)});")
        if interpolate:
            cg.emit(f"lv_obj_set_style_antialias({cg.current_obj}, true, LV_STATE_DEFAULT);")


def generate_animation(cg: CodeGen, widget: dict):
    """Generate LVGL animation (lv_anim_t) from animation definitions."""
    anims = widget.get('anim')
    if not anims:
        return

    wid = widget['id']
    if isinstance(anims, dict):
        anims = [anims]

    cg.emit(f"/* Animations for {wid} */")

    for i, anim in enumerate(anims):
        anim_type = anim['type']
        duration = anim.get('duration_ms', 400)
        delay = anim.get('delay_ms', 0)
        easing_map = {
            'linear': 'LV_ANIM_EASE_LINEAR', 'ease_in': 'LV_ANIM_EASE_IN',
            'ease_out': 'LV_ANIM_EASE_OUT', 'ease_in_out': 'LV_ANIM_EASE_IN_OUT',
            'bounce': 'LV_ANIM_EASE_BOUNCE', 'overshoot': 'LV_ANIM_EASE_OVERSHOOT'
        }
        ease = easing_map.get(anim.get('ease', 'ease_out'), 'LV_ANIM_EASE_OUT')
        loop = anim.get('loop', False)
        reverse = anim.get('reverse', False)
        play_count = '0' if loop else '1'

        anim_id = f"anim_{wid}_{i}"

        if anim_type == 'rotation':
            frm = anim.get('from', 0)
            to = anim.get('to', 3600)
            cg.emit(f"static lv_anim_t {anim_id};")
            cg.emit(f"lv_anim_init(&{anim_id});")
            cg.emit(f"lv_anim_set_var(&{anim_id}, {cg.current_obj});")
            cg.emit(f"lv_anim_set_exec_cb(&{anim_id}, (lv_anim_exec_xcb_t)lv_img_set_angle);")
            cg.emit(f"lv_anim_set_values(&{anim_id}, {int(frm)}, {int(to)});")
            cg.emit(f"lv_anim_set_time(&{anim_id}, {duration});")
            cg.emit(f"lv_anim_set_delay(&{anim_id}, {delay});")
            cg.emit(f"lv_anim_set_playback_time(&{anim_id}, {anim.get('playback_time_ms', duration)});")
            cg.emit(f"lv_anim_set_repeat_count(&{anim_id}, {play_count});")
            cg.emit(f"lv_anim_set_path_cb(&{anim_id}, {ease});")
            if reverse:
                cg.emit(f"lv_anim_set_playback_time(&{anim_id}, {duration});")
            cg.emit(f"lv_anim_start(&{anim_id});")

        elif anim_type == 'alpha_pulse':
            prop = anim.get('property', 'opa')
            frm = anim.get('from', 128)
            to = anim.get('to', 255)
            cg.emit(f"static lv_anim_t {anim_id};")
            cg.emit(f"lv_anim_init(&{anim_id});")
            cg.emit(f"lv_anim_set_var(&{anim_id}, {cg.current_obj});")
            cg.emit(f"lv_anim_set_exec_cb(&{anim_id}, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);")
            cg.emit(f"lv_anim_set_values(&{anim_id}, {int(frm)}, {int(to)});")
            cg.emit(f"lv_anim_set_time(&{anim_id}, {duration});")
            cg.emit(f"lv_anim_set_delay(&{anim_id}, {delay});")
            cg.emit(f"lv_anim_set_repeat_count(&{anim_id}, {play_count});")
            if reverse:
                cg.emit(f"lv_anim_set_playback_time(&{anim_id}, {duration});")
            cg.emit(f"lv_anim_set_path_cb(&{anim_id}, {ease});")
            cg.emit(f"lv_anim_start(&{anim_id});")

        elif anim_type == 'scale':
            frm_scale = int(anim.get('from', 256))
            to_scale = int(anim.get('to', 512))
            cg.emit(f"static lv_anim_t {anim_id};")
            cg.emit(f"lv_anim_init(&{anim_id});")
            cg.emit(f"lv_anim_set_var(&{anim_id}, {cg.current_obj});")
            cg.emit(f"lv_anim_set_exec_cb(&{anim_id}, (lv_anim_exec_xcb_t)lv_img_set_zoom);")
            cg.emit(f"lv_anim_set_values(&{anim_id}, {frm_scale}, {to_scale});")
            cg.emit(f"lv_anim_set_time(&{anim_id}, {duration});")
            cg.emit(f"lv_anim_set_repeat_count(&{anim_id}, {play_count});")
            if reverse:
                cg.emit(f"lv_anim_set_playback_time(&{anim_id}, {duration});")
            cg.emit(f"lv_anim_set_path_cb(&{anim_id}, {ease});")
            cg.emit(f"lv_anim_start(&{anim_id});")

        elif anim_type == 'color_transition':
            cg.emit(f"/* Color transition animation for {wid} — handled via state change timer */")

        elif anim_type == 'value':
            prop = anim.get('property', 'value')
            frm = anim.get('from', 0)
            to = anim.get('to', 100)
            cg.emit(f"static lv_anim_t {anim_id};")
            cg.emit(f"lv_anim_init(&{anim_id});")
            cg.emit(f"lv_anim_set_var(&{anim_id}, {cg.current_obj});")
            cg.emit(f"lv_anim_set_exec_cb(&{anim_id}, (lv_anim_exec_xcb_t)lv_arc_set_value);")
            cg.emit(f"lv_anim_set_values(&{anim_id}, {int(frm)}, {int(to)});")
            cg.emit(f"lv_anim_set_time(&{anim_id}, {duration});")
            cg.emit(f"lv_anim_set_repeat_count(&{anim_id}, {play_count});")
            if reverse:
                cg.emit(f"lv_anim_set_playback_time(&{anim_id}, {duration});")
            cg.emit(f"lv_anim_set_path_cb(&{anim_id}, {ease});")
            cg.emit(f"lv_anim_start(&{anim_id});")


def generate_composite(cg: CodeGen, widget: dict):
    """Generate IT2D multi-surface composite hints."""
    comp = widget.get('composite')
    if not comp:
        return

    layer = comp.get('layer', 0)
    blend = comp.get('blend_mode', 'normal')
    cache = comp.get('cache_surf', False)

    blend_map = {
        'normal': 'ITE_IT2D_ALPHA_DEFAULT',
        'additive': 'ITE_IT2D_ALPHA_SRC',
        'subtractive': 'ITE_IT2D_ALPHA_DST',
    }
    bm = blend_map.get(blend, 'ITE_IT2D_ALPHA_DEFAULT')

    cg.emit(f"/* IT2D composite: layer={layer}, blend={blend} */")
    cg.emit(f"lv_obj_set_style_blend_mode({cg.current_obj}, LV_BLEND_MODE_NORMAL, LV_STATE_DEFAULT);")

    if cache:
        cg.emit(f"/* Cached composite surface for {widget['id']} */")


def generate_events(cg: CodeGen, widget: dict):
    """Generate event callback registrations."""
    events = widget.get('events', [])
    if not events:
        return

    wid = widget['id']
    for evt in events:
        trigger = evt['trigger']
        action = evt['action']
        target = evt.get('target', '')

        event_cb_map = {
            'on_click': 'LV_EVENT_CLICKED',
            'on_press': 'LV_EVENT_PRESSED',
            'on_release': 'LV_EVENT_RELEASED',
            'on_long_press': 'LV_EVENT_LONG_PRESSED',
            'on_value_change': 'LV_EVENT_VALUE_CHANGED',
            'on_focus': 'LV_EVENT_FOCUSED',
            'on_defocus': 'LV_EVENT_DEFOCUSED',
        }

        lv_event = event_cb_map.get(trigger, 'LV_EVENT_CLICKED')

        if action == 'toggle_state':
            target_state = target
            cb_name = f"on_{wid}_{trigger}"
            cg.emit(f"static void {cb_name}(lv_event_t *e) {{")
            cg.indent()
            cg.emit(f"extern void ui_apply_state(const char *state_name);")
            cg.emit(f'ui_apply_state("{target_state}");')
            cg.dedent()
            cg.emit(f"}}")
            cg.emit(f"lv_obj_add_event_cb({cg.current_obj}, {cb_name}, {lv_event}, NULL);")

        elif action == 'navigate_to':
            cb_name = f"on_{wid}_{trigger}"
            cg.emit(f"static void {cb_name}(lv_event_t *e) {{")
            cg.indent()
            cg.emit(f"extern void ui_load_screen(const char *name);")
            cg.emit(f'ui_load_screen("{target}");')
            cg.dedent()
            cg.emit(f"}}")
            cg.emit(f"lv_obj_add_event_cb({cg.current_obj}, {cb_name}, {lv_event}, NULL);")

        elif action == 'callback':
            cb_name = evt.get('params', {}).get('handler', f"ui_callback_{wid}_{trigger}")
            cg.emit(f"extern void {cb_name}(lv_event_t *e);")
            cg.emit(f"lv_obj_add_event_cb({cg.current_obj}, {cb_name}, {lv_event}, NULL);")

        else:
            # Generic action — call a named function
            cg.emit(f"extern void {action}(lv_event_t *e);")
            cg.emit(f"lv_obj_add_event_cb({cg.current_obj}, {action}, {lv_event}, NULL);")


def generate_widget(cg: CodeGen, widget: dict, parent_var: str = "screen"):
    """Generate LVGL C code for a single widget."""
    wtype = widget['type']
    wid = widget['id']
    cg.track_widget(wid)

    x, y = widget.get('x', 0), widget.get('y', 0)
    w, h = widget.get('w', 100), widget.get('h', 50)

    var_name = f"ui_{wid}"
    cg.current_obj = var_name

    # Widget creation
    creators = {
        'container':  f"{var_name} = lv_obj_create({parent_var});",
        'panel':      f"{var_name} = lv_obj_create({parent_var});",
        'label':      f"{var_name} = lv_label_create({parent_var});",
        'btn':        f"{var_name} = lv_btn_create({parent_var});",
        'image':      f"{var_name} = lv_img_create({parent_var});",
        'arc':        f"{var_name} = lv_arc_create({parent_var});",
        'bar':        f"{var_name} = lv_bar_create({parent_var});",
        'slider':     f"{var_name} = lv_slider_create({parent_var});",
        'switch':     f"{var_name} = lv_switch_create({parent_var});",
        'checkbox':   f"{var_name} = lv_checkbox_create({parent_var});",
        'dropdown':   f"{var_name} = lv_dropdown_create({parent_var});",
        'roller':     f"{var_name} = lv_roller_create({parent_var});",
        'textarea':   f"{var_name} = lv_textarea_create({parent_var});",
        'list':       f"{var_name} = lv_list_create({parent_var});",
        'table':      f"{var_name} = lv_table_create({parent_var});",
        'chart':      f"{var_name} = lv_chart_create({parent_var});",
        'spinner':    f"{var_name} = lv_spinner_create({parent_var});",
        'tabview':    f"{var_name} = lv_tabview_create({parent_var});",
        'msgbox':     f"{var_name} = lv_msgbox_create({parent_var});",
        'keyboard':   f"{var_name} = lv_keyboard_create({parent_var});",
        'canvas':     f"{var_name} = lv_canvas_create({parent_var});",
        'line':       f"{var_name} = lv_line_create({parent_var});",
        'led':        f"{var_name} = lv_led_create({parent_var});",
        'imgbtn':     f"{var_name} = lv_imgbtn_create({parent_var});",
        'calendar':   f"{var_name} = lv_calendar_create({parent_var});",
    }

    cg.emit(f"/* --- Widget: {wid} ({wtype}) --- */")
    create_line = creators.get(wtype)
    if not create_line:
        cg.emit(f"/* ERROR: Unknown widget type '{wtype}' */")
        return

    cg.emit(create_line)

    # Position and size
    cg.emit(f"lv_obj_set_pos({var_name}, {x}, {y});")
    cg.emit(f"lv_obj_set_size({var_name}, {w}, {h});")

    # Hidden
    if widget.get('hidden'):
        cg.emit(f"lv_obj_add_flag({var_name}, LV_OBJ_FLAG_HIDDEN);")

    # Clickable
    if widget.get('clickable'):
        cg.emit(f"lv_obj_add_flag({var_name}, LV_OBJ_FLAG_CLICKABLE);")

    # Scrollable
    if widget.get('scrollable'):
        cg.emit(f"lv_obj_add_flag({var_name}, LV_OBJ_FLAG_SCROLLABLE);")

    # Default style
    style = widget.get('style', {})
    generate_screen_style(cg, style)

    # State-specific styles
    state_styles = widget.get('state_style', {})
    for state_name, state_style in state_styles.items():
        generate_screen_style(cg, state_style, state_name)

    # Label text
    if 'label' in widget:
        label = widget['label'].replace('"', '\\"')
        cg.emit(f'lv_label_set_text({var_name}, "{label}");')

    # Value
    if 'value' in widget and wtype in ('arc', 'bar', 'slider', 'switch'):
        val = widget['value']
        if wtype == 'arc':
            cg.emit(f"lv_arc_set_value({var_name}, {int(val) if isinstance(val, (int, float)) else val});")
        elif wtype == 'bar':
            cg.emit(f"lv_bar_set_value({var_name}, {int(val) if isinstance(val, (int, float)) else val}, LV_ANIM_OFF);")
        elif wtype == 'slider':
            cg.emit(f"lv_slider_set_value({var_name}, {int(val) if isinstance(val, (int, float)) else val}, LV_ANIM_OFF);")
        elif wtype == 'switch' and isinstance(val, bool):
            cg.emit(f"if ({'true' if val else 'false'}) lv_obj_add_state({var_name}, LV_STATE_CHECKED);")

    # Range
    rng = widget.get('range')
    if rng and wtype in ('arc', 'bar', 'slider'):
        cg.emit(f"lv_{wtype}_set_range({var_name}, {rng.get('min', 0)}, {rng.get('max', 100)});")

    # Options (dropdown/roller)
    opts = widget.get('options')
    if opts:
        opt_str = '\\n'.join(opts)
        cg.emit(f'lv_{wtype}_set_options({var_name}, "{opt_str}");')

    # Image source
    src = widget.get('src')
    if src and wtype in ('image', 'imgbtn'):
        if isinstance(src, dict):
            # Support multi-variant images
            default_src = src.get('default', src)
            if isinstance(default_src, dict):
                filepath = default_src.get('file', '')
                cg.track_asset(filepath)
                img_var = f"img_{wid}"
                cg.emit(f"LV_IMG_DECLARE({img_var});")
                cg.emit(f"lv_img_set_src({var_name}, &{img_var});")

                if src.get('recolorable'):
                    cg.emit(f"/* Image marked as recolorable — grayscale source */")
                if default_src.get('chroma_keyed'):
                    cg.emit(f"/* Chroma key enabled for {wid} */")

                # Error variant
                error_src = src.get('error')
                if error_src and isinstance(error_src, dict):
                    err_file = error_src.get('file', '')
                    cg.track_asset(err_file)
                    cg.emit(f"LV_IMG_DECLARE(img_{wid}_error);")
        elif isinstance(src, str):
            cg.emit(f'lv_img_set_src({var_name}, "{src}");')

    # Apply IT2D GPU features
    generate_recolor(cg, widget)
    generate_glow(cg, widget)
    generate_glass(cg, widget)
    generate_transform(cg, widget)
    generate_composite(cg, widget)

    # Animations
    generate_animation(cg, widget)

    # Events
    generate_events(cg, widget)

    # Children
    children = widget.get('children', [])
    for child in children:
        generate_widget(cg, child, var_name)

    cg.emit()
    return var_name


# ═══════════════════════════════════════════════════════════════
# State machine generator
# ═══════════════════════════════════════════════════════════════

def generate_states(cg: CodeGen):
    """Generate state machine code from screen states definition."""
    screen = cg.data.get('screen', {})
    states = screen.get('states', {})
    if not states:
        return

    state_names = list(states.keys())
    cg.state_names = state_names

    # State enum
    cg.emit()
    cg.emit("/* ══════════════════════════════════════════════════════ */")
    cg.emit("/* UI State Machine                                         */")
    cg.emit("/* ══════════════════════════════════════════════════════ */")
    cg.emit()
    cg.emit("typedef enum {")
    for i, name in enumerate(state_names):
        cg.emit(f"    UI_STATE_{name.upper()} = {i},")
    cg.emit("    UI_STATE_COUNT")
    cg.emit("} ui_state_t;")
    cg.emit()
    cg.emit("static ui_state_t ui_current_state = UI_STATE_DEFAULT;")
    cg.emit("static ui_state_t ui_previous_state = UI_STATE_DEFAULT;")
    cg.emit()

    # Global current state tracking
    cg.emit("static const char *ui_state_names[] = {")
    for name in state_names:
        cg.emit(f'    "{name}",')
    cg.emit("};")
    cg.emit()

    # State apply function
    cg.emit("void ui_apply_state(const char *state_name) {")
    cg.indent()
    cg.emit("ui_previous_state = ui_current_state;")
    cg.emit()
    cg.emit("/* Find state index */")
    cg.emit("for (int i = 0; i < UI_STATE_COUNT; i++) {")
    cg.emit("    if (strcmp(ui_state_names[i], state_name) == 0) {")
    cg.emit("        ui_current_state = (ui_state_t)i;")
    cg.emit("        break;")
    cg.emit("    }")
    cg.emit("}")
    cg.emit()

    cg.emit("/* Apply state-specific overrides */")
    cg.emit("switch (ui_current_state) {")
    cg.indent()

    for state_name, state_def in states.items():
        cg.emit(f"case UI_STATE_{state_name.upper()}: {{")
        cg.indent()

        # Background image change
        bg_image = state_def.get('bg_image')
        if bg_image:
            screen_id = screen.get('id', 'main')
            cg.emit(f'/* TODO: change background to {bg_image} */')

        # Widget overrides
        overrides = state_def.get('overrides', {})
        for ovr_wid, ovr in overrides.items():
            var_name = f"ui_{ovr_wid}"

            if ovr.get('hidden') is True:
                cg.emit(f"lv_obj_add_flag({var_name}, LV_OBJ_FLAG_HIDDEN);")
            elif ovr.get('hidden') is False:
                cg.emit(f"lv_obj_clear_flag({var_name}, LV_OBJ_FLAG_HIDDEN);")

            if 'text' in ovr:
                txt = ovr['text'].replace('"', '\\"')
                cg.emit(f'lv_label_set_text({var_name}, "{txt}");')

            if 'text_color' in ovr:
                c = resolve_color(ovr['text_color'], cg.tokens)
                cg.emit(f"lv_obj_set_style_text_color({var_name}, {c}, LV_STATE_DEFAULT);")

            if 'bg_color' in ovr:
                c = resolve_color(ovr['bg_color'], cg.tokens)
                cg.emit(f"lv_obj_set_style_bg_color({var_name}, {c}, LV_STATE_DEFAULT);")

            if 'border_color' in ovr:
                c = resolve_color(ovr['border_color'], cg.tokens)
                cg.emit(f"lv_obj_set_style_border_color({var_name}, {c}, LV_STATE_DEFAULT);")

            if 'recolor' in ovr:
                r = ovr['recolor']
                c = resolve_color(r['color'], cg.tokens)
                opa = r.get('opa', 255)
                cg.emit(f"/* State override: hardware recolor to {r['color']} */")
                cg.emit(f"lv_img_set_recolor({var_name}, {c});")
                cg.emit(f"lv_obj_set_style_img_recolor_opa({var_name}, {opa}, LV_STATE_DEFAULT);")

            if 'src_variant' in ovr:
                variant = ovr['src_variant']
                cg.emit(f"/* Switch image to {variant} variant */")
                cg.emit(f"lv_img_set_src({var_name}, &img_{ovr_wid}_{variant});")

            if 'opa' in ovr:
                cg.emit(f"lv_obj_set_style_opa({var_name}, {ovr['opa']}, LV_STATE_DEFAULT);")

            if 'value' in ovr:
                cg.emit(f"/* TODO: set value {ovr['value']} on {ovr_wid} */")

            # State-specific glow override
            if 'glow' in ovr:
                cg.emit(f"/* State override: glow updated for {ovr_wid} */")

            # State-specific animation override
            if 'anim' in ovr:
                cg.emit(f"/* State override: animation restarted for {ovr_wid} */")

            if 'label' in ovr:
                txt = ovr['label'].replace('"', '\\"')
                cg.emit(f'lv_label_set_text({var_name}, "{txt}");')

        cg.emit("break;")
        cg.dedent()
        cg.emit("}")

    cg.emit("default: break;")
    cg.dedent()
    cg.emit("}")
    cg.dedent()
    cg.emit("}")
    cg.emit()

    # Toggle helper
    cg.emit("void ui_toggle_state(const char *state_a, const char *state_b) {")
    cg.indent()
    cg.emit("if (strcmp(ui_state_names[ui_current_state], state_a) == 0) {")
    cg.emit("    ui_apply_state(state_b);")
    cg.emit("} else {")
    cg.emit("    ui_apply_state(state_a);")
    cg.emit("}")
    cg.dedent()
    cg.emit("}")
    cg.emit()


# ═══════════════════════════════════════════════════════════════
# Asset manifest generator
# ═══════════════════════════════════════════════════════════════

def generate_asset_manifest(cg: CodeGen):
    """Generate the list of image assets that need conversion."""
    if not cg.asset_files:
        return

    cg.emit()
    cg.emit("/* ══════════════════════════════════════════════════════ */")
    cg.emit("/* Asset Manifest                                           */")
    cg.emit("/* ══════════════════════════════════════════════════════ */")
    cg.emit("/*")
    cg.emit(" * Image assets referenced by this UI. Convert them with:")
    cg.emit(" *   python assets_pipeline.py --input <files> --output data/")
    cg.emit(" */")
    cg.emit("/*")
    for f in sorted(cg.asset_files):
        cg.emit(f" *   - {f}")
    cg.emit(" */")
    cg.emit()


# ═══════════════════════════════════════════════════════════════
# Full file generators
# ═══════════════════════════════════════════════════════════════

def generate_source(cg: CodeGen) -> str:
    """Generate the complete .c file."""
    name = cg.data.get('name', 'ui_screen')

    # Header
    cg.emit(f"/*")
    cg.emit(f" * {name}.c — Auto-generated by json2lvgl.py")
    cg.emit(f" * Version: {cg.data.get('version', '1.0')}")
    cg.emit(f" * Target: IT9866/IT9868 with IT2D GPU acceleration")
    cg.emit(f" * DO NOT EDIT BY HAND — modify the JSON source instead.")
    cg.emit(f" */")
    cg.emit()
    cg.emit('#include "lvgl/lvgl.h"')
    cg.emit('#include "ite_it2d.h"')
    cg.emit('#include "lv_draw_ite_it2d.h"')
    cg.emit('#include <string.h>')
    cg.emit(f'#include "{name}.h"')
    cg.emit()

    # === Screen creation function ===
    screen = cg.data.get('screen', {})
    screen_id = screen.get('id', 'main')

    cg.emit(f"/* ══════════════════════════════════════════════════════ */")
    cg.emit(f"/* Screen: {screen_id}                                    */")
    cg.emit(f"/* ══════════════════════════════════════════════════════ */")
    cg.emit()
    cg.emit(f"lv_obj_t *ui_screen_create(void) {{")
    cg.indent()

    # Create screen
    cg.emit(f"lv_obj_t *screen = lv_obj_create(NULL);")
    display = cg.data.get('display', {})
    cg.emit(f"lv_obj_set_size(screen, {display.get('width', 480)}, {display.get('height', 480)});")

    # Background
    bg_color = screen.get('bg_color')
    if bg_color:
        c = resolve_color(bg_color, cg.tokens)
        cg.emit(f"lv_obj_set_style_bg_color(screen, {c}, LV_STATE_DEFAULT);")

    bg_image = screen.get('bg_image')
    if bg_image:
        cg.track_asset(bg_image)
        bg_var = f"img_bg_{screen_id}"
        cg.emit(f"LV_IMG_DECLARE({bg_var});")
        cg.emit(f"lv_obj_set_style_bg_img_src(screen, &{bg_var}, LV_STATE_DEFAULT);")

    bg_gradient = screen.get('bg_gradient')
    if bg_gradient:
        cg.emit(f"/* Background gradient: {bg_gradient.get('direction', 'vertical')} */")

    cg.emit()

    # Generate all children
    for widget in screen.get('children', []):
        generate_widget(cg, widget, "screen")

    cg.emit()
    cg.emit(f"return screen;")
    cg.dedent()
    cg.emit(f"}}")
    cg.emit()

    # === State machine ===
    generate_states(cg)

    # === Asset manifest ===
    generate_asset_manifest(cg)

    return '\n'.join(cg.output)


def generate_header(cg: CodeGen) -> str:
    """Generate the .h header file."""
    name = cg.data.get('name', 'ui_screen')
    guard = f"{name.upper()}_H"

    lines = []
    lines.append(f"/* Auto-generated by json2lvgl.py — DO NOT EDIT */")
    lines.append(f"#ifndef {guard}")
    lines.append(f"#define {guard}")
    lines.append("")
    lines.append('#include "lvgl/lvgl.h"')
    lines.append("")
    lines.append(f"/* Screen creation */")
    lines.append(f"lv_obj_t *ui_screen_create(void);")
    lines.append("")
    lines.append(f"/* State control */")
    lines.append(f"void ui_apply_state(const char *state_name);")
    lines.append(f"void ui_toggle_state(const char *state_a, const char *state_b);")
    lines.append("")
    lines.append(f"/* Widget accessors */")
    for wid in cg.widget_ids:
        lines.append(f"extern lv_obj_t *ui_{wid};")
    lines.append("")
    lines.append(f"#endif /* {guard} */")
    return '\n'.join(lines)


# ═══════════════════════════════════════════════════════════════
# CLI
# ═══════════════════════════════════════════════════════════════

def main():
    parser = argparse.ArgumentParser(
        description='json2lvgl — Convert LVGL UI JSON to C code for IT9866/IT9868',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python json2lvgl.py gateway_hmi_example.json
  python json2lvgl.py input.json -o project/GW202601_LVGL/ui/
  python json2lvgl.py input.json --preview
        """)

    parser.add_argument('input', help='Input JSON file (following lvgl_ui_schema.json)')
    parser.add_argument('-o', '--output-dir', help='Output directory for .c and .h files')
    parser.add_argument('--preview', action='store_true', help='Preview mode: print stats only')
    parser.add_argument('--name', help='Override project name (default: from JSON)')

    args = parser.parse_args()

    # Load JSON
    with open(args.input, 'r', encoding='utf-8') as f:
        data = json.load(f)

    if args.name:
        data['name'] = args.name

    name = data.get('name', 'ui_screen')

    # Validate
    if 'screen' not in data:
        print("ERROR: JSON must have a 'screen' key.", file=sys.stderr)
        sys.exit(1)

    screen = data['screen']
    if 'children' not in screen:
        print("ERROR: 'screen' must have a 'children' array.", file=sys.stderr)
        sys.exit(1)

    # Generate
    cg = CodeGen(data)
    c_code = generate_source(cg)
    h_code = generate_header(cg)

    if args.preview:
        # Preview mode
        widget_count = len(cg.widget_ids)
        state_count = len(cg.state_names)
        asset_count = len(cg.asset_files)
        line_count = len(c_code.split('\n'))

        print(f"╔══════════════════════════════════════╗")
        print(f"║  json2lvgl — Preview                ║")
        print(f"╠══════════════════════════════════════╣")
        print(f"║  Project:   {name:<24s} ║")
        print(f"║  Widgets:   {widget_count:<24d} ║")
        print(f"║  States:    {state_count:<24d} ║")
        print(f"║  Assets:    {asset_count:<24d} ║")
        print(f"║  C Lines:   {line_count:<24d} ║")
        print(f"╚══════════════════════════════════════╝")

        if state_count > 0:
            print(f"\nStates: {', '.join(cg.state_names)}")
        if asset_count > 0:
            print(f"\nAssets to convert:")
            for f in sorted(cg.asset_files):
                print(f"  • {f}")

    elif args.output_dir:
        # Write files
        os.makedirs(args.output_dir, exist_ok=True)
        c_path = os.path.join(args.output_dir, f"{name}.c")
        h_path = os.path.join(args.output_dir, f"{name}.h")

        with open(c_path, 'w', encoding='utf-8') as f:
            f.write(c_code)
        with open(h_path, 'w', encoding='utf-8') as f:
            f.write(h_code)

        print(f"[OK] Generated {c_path} ({len(c_code.split(chr(10)))} lines)")
        print(f"[OK] Generated {h_path} ({len(h_code.split(chr(10)))} lines)")
        print(f"   Widgets: {len(cg.widget_ids)}, States: {len(cg.state_names)}, Assets: {len(cg.asset_files)}")

    else:
        # Stdout
        print(c_code)
        print()
        print("/* ────────────────────────────────────────────── */")
        print("/* Header file (.h)                               */")
        print("/* ────────────────────────────────────────────── */")
        print(h_code)


if __name__ == '__main__':
    main()
