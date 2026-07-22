/*
 * ui_screen.c — Gateway HMI with boot splash + dual-ring dashboard
 *
 * Phase 1 (0-3s): Splash — MedFlo logo + NMGW2601 branding
 * Phase 2:        Dual-ring HMI — WiFi/BLE status rings + info panels
 */

#include "ui_screen.h"
#include "img_bg_480.h"
#include "logo_nmgw.h"
#include <stdio.h>

/* ── Colors (RGB565) ── */
#define CYAN        lv_color_hex(0x00E5FF)
#define BLUE        lv_color_hex(0x2979FF)
#define ORANGE      lv_color_hex(0xFF9100)
#define WHITE       lv_color_hex(0xFFFFFF)
#define DARK_BG     lv_color_hex(0x050A10)
#define PANEL_BG    lv_color_hex(0x0A1520)
#define DIM_WHITE   lv_color_hex(0x8899AA)

/* ── Widgets ── */
static lv_obj_t *splash_scr = NULL;
static lv_obj_t *main_scr    = NULL;
static lv_obj_t *arc_wifi    = NULL;
static lv_obj_t *arc_ble     = NULL;
static lv_obj_t *label_status = NULL;
static lv_obj_t *label_wifi  = NULL;
static lv_obj_t *label_ble   = NULL;

static lv_anim_t anim_w, anim_b;

/* ── Helpers ── */
static void arc_rotate_cb(void *var, int32_t v)
{
    lv_arc_set_rotation((lv_obj_t *)var, v % 3600);
}

static lv_obj_t *mk_label(lv_obj_t *p, const char *txt, lv_color_t c,
                           const lv_font_t *f, int x, int y)
{
    lv_obj_t *l = lv_label_create(p);
    lv_label_set_text(l, txt);
    lv_obj_set_style_text_color(l, c, 0);
    if (f) lv_obj_set_style_text_font(l, f, 0);
    lv_obj_set_pos(l, x, y);
    return l;
}

/* ═══════════════════════════════════════════════════
 * SPLASH SCREEN
 * ═══════════════════════════════════════════════════ */
static void splash_timer_cb(lv_timer_t *t)
{
    /* Build main HMI on a NEW screen, then switch */
    main_scr = lv_obj_create(NULL);

    /* Background image */
    lv_obj_t *bg2 = lv_image_create(main_scr);
    lv_image_set_src(bg2, &img_bg_480);
    lv_obj_set_size(bg2, 480, 480);
    lv_obj_center(bg2);

    /* ── WiFi arc ring (outer, cyan) ── */
    arc_wifi = lv_arc_create(main_scr);
    lv_obj_set_size(arc_wifi, 432, 432);
    lv_obj_set_pos(arc_wifi, 24, 24);
    lv_obj_remove_style(arc_wifi, NULL, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(arc_wifi, LV_OPA_0, 0);
    /* Track (background circle) */
    lv_obj_set_style_arc_width(arc_wifi, 8, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_wifi, lv_color_hex(0x0D2030), LV_PART_MAIN);
    lv_obj_set_style_arc_opa(arc_wifi, LV_OPA_60, LV_PART_MAIN);
    /* Indicator (sweep) */
    lv_obj_set_style_arc_width(arc_wifi, 8, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc_wifi, CYAN, LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(arc_wifi, LV_OPA_90, LV_PART_INDICATOR);
    lv_arc_set_bg_angles(arc_wifi, 0, 360);
    lv_arc_set_angles(arc_wifi, 0, 100);

    lv_anim_init(&anim_w);
    lv_anim_set_var(&anim_w, arc_wifi);
    lv_anim_set_exec_cb(&anim_w, arc_rotate_cb);
    lv_anim_set_values(&anim_w, 0, 3600);
    lv_anim_set_time(&anim_w, 40000);
    lv_anim_set_repeat_count(&anim_w, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&anim_w);

    /* ── BLE arc ring (inner, blue) ── */
    arc_ble = lv_arc_create(main_scr);
    lv_obj_set_size(arc_ble, 336, 336);
    lv_obj_set_pos(arc_ble, 72, 72);
    lv_obj_remove_style(arc_ble, NULL, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(arc_ble, LV_OPA_0, 0);
    lv_obj_set_style_arc_width(arc_ble, 8, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_ble, lv_color_hex(0x0D2030), LV_PART_MAIN);
    lv_obj_set_style_arc_opa(arc_ble, LV_OPA_60, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_ble, 8, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc_ble, BLUE, LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(arc_ble, LV_OPA_90, LV_PART_INDICATOR);
    lv_arc_set_bg_angles(arc_ble, 0, 360);
    lv_arc_set_angles(arc_ble, 0, 130);

    lv_anim_init(&anim_b);
    lv_anim_set_var(&anim_b, arc_ble);
    lv_anim_set_exec_cb(&anim_b, arc_rotate_cb);
    lv_anim_set_values(&anim_b, 3600, 0);
    lv_anim_set_time(&anim_b, 30000);
    lv_anim_set_repeat_count(&anim_b, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&anim_b);

    /* ── Center icon + status ── */
    label_status = lv_label_create(main_scr);
    lv_label_set_text(label_status, LV_SYMBOL_WIFI "\n" LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_text_color(label_status, CYAN, 0);
    lv_obj_set_style_text_font(label_status, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_align(label_status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label_status, LV_ALIGN_CENTER, 0, -15);

    /* ── Model label ── */
    mk_label(main_scr, "N M G W 2 6 0 1", DIM_WHITE,
             &lv_font_montserrat_16, 0, 250);
    lv_obj_t *ml = lv_obj_get_child(main_scr, -1);
    lv_obj_set_style_text_letter_space(ml, 4, 0);

    /* ── WiFi panel ── */
    lv_obj_t *pw = lv_obj_create(main_scr);
    lv_obj_set_size(pw, 404, 40);
    lv_obj_set_pos(pw, 38, 336);
    lv_obj_set_style_bg_color(pw, PANEL_BG, 0);
    lv_obj_set_style_bg_opa(pw, LV_OPA_80, 0);
    lv_obj_set_style_border_width(pw, 1, 0);
    lv_obj_set_style_border_color(pw, CYAN, 0);
    lv_obj_set_style_border_opa(pw, LV_OPA_30, 0);
    lv_obj_set_style_radius(pw, 6, 0);
    lv_obj_set_style_pad_all(pw, 6, 0);

    mk_label(pw, LV_SYMBOL_WIFI, CYAN, &lv_font_montserrat_16, 8, 10);
    mk_label(pw, "WiFi", CYAN, &lv_font_montserrat_14, 30, 11);
    label_wifi = mk_label(pw, "--", WHITE, &lv_font_montserrat_14, 80, 11);

    /* ── BLE panel ── */
    lv_obj_t *pb = lv_obj_create(main_scr);
    lv_obj_set_size(pb, 404, 40);
    lv_obj_set_pos(pb, 38, 386);
    lv_obj_set_style_bg_color(pb, PANEL_BG, 0);
    lv_obj_set_style_bg_opa(pb, LV_OPA_80, 0);
    lv_obj_set_style_border_width(pb, 1, 0);
    lv_obj_set_style_border_color(pb, BLUE, 0);
    lv_obj_set_style_border_opa(pb, LV_OPA_30, 0);
    lv_obj_set_style_radius(pb, 6, 0);
    lv_obj_set_style_pad_all(pb, 6, 0);

    mk_label(pb, LV_SYMBOL_BLUETOOTH, BLUE, &lv_font_montserrat_16, 8, 10);
    mk_label(pb, "BLE", BLUE, &lv_font_montserrat_14, 30, 11);
    label_ble = mk_label(pb, "--", WHITE, &lv_font_montserrat_14, 80, 11);

    /* ── Version ── */
    /* Version (uses CMake FW_BUILD_TIME) */
    lv_obj_t *ver = lv_label_create(main_scr);
    lv_label_set_text_fmt(ver, "GW202601  LVGL9.3  IT2D  %s", FW_BUILD_TIME);
    lv_obj_set_style_text_color(ver, DIM_WHITE, 0);
    lv_obj_set_style_text_font(ver, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(ver, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(ver, 480);
    lv_obj_set_pos(ver, 0, 460);

    /* Switch to main screen, then cleanup splash */
    lv_scr_load(main_scr);
    if (splash_scr) {
        lv_obj_del(splash_scr);
        splash_scr = NULL;
    }
}

/* ═══════════════════════════════════════════════════
 * ui_screen_init — splash → HMI
 * ═══════════════════════════════════════════════════ */
void ui_screen_init(void)
{
    /* ── Splash screen ── */
    splash_scr = lv_obj_create(NULL);
    lv_scr_load(splash_scr);

    /* Dark background */
    lv_obj_set_style_bg_color(splash_scr, DARK_BG, 0);
    lv_obj_set_style_bg_opa(splash_scr, LV_OPA_COVER, 0);

    /* NMGW2601 Logo image */
    lv_obj_t *logo_img = lv_image_create(splash_scr);
    lv_image_set_src(logo_img, &logo_nmgw);
    lv_obj_set_size(logo_img, 480, 480);
    lv_obj_center(logo_img);

    /* Auto-transition after 3 seconds */
    lv_timer_create(splash_timer_cb, 3000, NULL);
}

/* ═══════════════════════════════════════════════════
 * Status updaters
 * ═══════════════════════════════════════════════════ */
void ui_screen_update_wifi(const char *ssid, const char *ip, bool ok)
{
    if (!label_wifi) return;
    char b[80];
    if (ok && ip && ip[0] && !(ip[0]=='0'&&ip[1]=='0'&&ip[2]=='0'&&ip[3]=='0')) {
        snprintf(b, sizeof(b), "%s  |  %s", ssid, ip);
        lv_obj_set_style_arc_color(arc_wifi, CYAN, LV_PART_INDICATOR);
    } else if (ok) {
        snprintf(b, sizeof(b), "%s  |  waiting DHCP...", ssid);
    } else {
        snprintf(b, sizeof(b), "%s  |  OFFLINE", ssid);
        lv_obj_set_style_arc_color(arc_wifi, ORANGE, LV_PART_INDICATOR);
    }
    lv_label_set_text(label_wifi, b);
}

void ui_screen_update_ble(int cnt, bool ok)
{
    if (!label_ble) return;
    char b[32];
    snprintf(b, sizeof(b), ok ? "%d devices" : "FAULT", cnt);
    lv_label_set_text(label_ble, b);
    if (!ok) {
        lv_obj_set_style_arc_color(arc_ble, ORANGE, LV_PART_INDICATOR);
    }
}

void ui_screen_set_error(bool err)
{
    if (!arc_wifi) return;
    lv_color_t c = err ? ORANGE : CYAN;
    lv_obj_set_style_arc_color(arc_wifi, c, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc_ble,  c, LV_PART_INDICATOR);
    if (label_status) {
        lv_label_set_text(label_status, err ? LV_SYMBOL_WARNING : LV_SYMBOL_WIFI "\n" LV_SYMBOL_BLUETOOTH);
        lv_obj_set_style_text_color(label_status, c, 0);
    }
}
