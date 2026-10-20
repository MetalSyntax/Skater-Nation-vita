/**
 * @file patch.c
 * @brief Game-specific hooks, memory patches, physical controls mapping, and camera orbit.
 */
#include <psp2/ctrl.h>
#include <psp2/io/fcntl.h>
#include <psp2/kernel/processmgr.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vitaGL.h>

#include "imports.h"
#include "input.h"
#include "log.h"
#include "macho.h"
#include "patch.h"

// Game-specific import tables for the port being developed.
const import_entry patch_imports[] = {
};
const int patch_imports_count = sizeof(patch_imports) / sizeof(patch_imports[0]);

void patch_init(void) {
    l_info("Initializing game-specific patches...");
}

// ---------------------------------------------------------------- HUD controls opacity (1% default)

static float hud_alpha = 1.0f; // 1% opacity for joystick and buttons A/B

static void set_character_alpha(uintptr_t ch, double alpha_pct) {
    if (!ch || ch < 0x81000000 || ch >= 0x99000000)
        return;

    // Guest (iOS) code: must go through call_guest, which preserves r9 (scratch on iOS).
    static void *tu_ctor, *tu_dtor, *as_ctor, *as_dtor, *set_mem;

    if (!tu_ctor) {
        tu_ctor = (void *)macho_sym("__ZN7gameswf9tu_stringC1EPKc");
        tu_dtor = (void *)macho_sym("__ZN7gameswf9tu_stringD1Ev");
        as_ctor = (void *)macho_sym("__ZN7gameswf8as_valueC1Ed");
        as_dtor = (void *)macho_sym("__ZN7gameswf8as_valueD1Ev");
        set_mem = (void *)macho_sym("__ZN7gameswf9character10set_memberERKNS_10tu_stringiERKNS_8as_valueE");
    }

    if (!tu_ctor || !tu_dtor || !as_ctor || !as_dtor || !set_mem)
        return;

    char str_buf[64] = {0};
    char val_buf[64] = {0};

    call_guest(tu_ctor, (uint32_t)str_buf, (uint32_t)"_alpha", 0, 0);
    union {
        double d;
        struct { uint32_t lo, hi; } u;
    } dv;
    dv.d = alpha_pct;
    // iOS softfp: as_value(double) takes the double in r1:r2 (verified in the binary)
    call_guest(as_ctor, (uint32_t)val_buf, dv.u.lo, dv.u.hi, 0);

    call_guest(set_mem, (uint32_t)ch, (uint32_t)str_buf, (uint32_t)val_buf, 0);

    call_guest(as_dtor, (uint32_t)val_buf, 0, 0, 0);
    call_guest(tu_dtor, (uint32_t)str_buf, 0, 0, 0);
}

static void update_hud_opacity(void) {
    static uint32_t *m_hudMenu_ptr = NULL;
    if (!m_hudMenu_ptr) {
        m_hudMenu_ptr = (uint32_t *)macho_sym("__ZN7HudMenu9m_hudMenuE");
    }
    if (!m_hudMenu_ptr || !*m_hudMenu_ptr)
        return;

    uintptr_t hud = *m_hudMenu_ptr;
    if (hud < 0x81000000 || hud >= 0x99000000)
        return;

    // Check if characters have been created by HudMenu::Init()
    uintptr_t btn_a = *(uintptr_t *)(hud + 0x128c);
    uintptr_t btn_b = *(uintptr_t *)(hud + 0x1294);
    uintptr_t dpad  = *(uintptr_t *)(hud + 0x1260);

    if (!btn_a || !btn_b || !dpad)
        return;
    if (btn_a < 0x81000000 || btn_a >= 0x99000000)
        return;
    if (btn_b < 0x81000000 || btn_b >= 0x99000000)
        return;
    if (dpad < 0x81000000 || dpad >= 0x99000000)
        return;

    // Re-apply when the HUD is rebuilt (new level) or the L+R toggle changes the value.
    static float last_applied_alpha = -1.0f;
    static uintptr_t last_btn_a;
    if (last_applied_alpha == hud_alpha && last_btn_a == btn_a)
        return;
    last_btn_a = btn_a;

    // Set 1% opacity only on joystick and buttons A/B. Names from HudMenu::Init (RenderFX::Find):
    // +0x1258 is mc_map (the minimap, left visible), +0x125c mc_thumb, +0x1260 btn_dpad, +0x1264 mc_pad.
    set_character_alpha(dpad, (double)hud_alpha);
    set_character_alpha(*(uintptr_t *)(hud + 0x125c), (double)hud_alpha); // mc_thumb
    set_character_alpha(*(uintptr_t *)(hud + 0x1264), (double)hud_alpha); // mc_pad (ring)
    // mc_btn_a (+0x128c)
    set_character_alpha(btn_a, (double)hud_alpha);
    // mc_btn_b (+0x1294)
    set_character_alpha(btn_b, (double)hud_alpha);

    last_applied_alpha = hud_alpha;
    l_info("HUD controls opacity set to %.0f%%", hud_alpha);
}

void patch_frame(void) {
    update_hud_opacity();
}

// ---------------------------------------------------------------- synthetic touch helpers

static void add_touch_raw(int id, int x, int y) {
    if (touch_count >= MAX_TOUCHES)
        return;
    touch_point *t = &touches[touch_count++];
    t->id = id;
    t->gx = x;
    t->gy = y;
    t->down = 1;
}

static void add_touch(int id, float x, float y) {
    add_touch_raw(id, x < 0 ? 0 : x > 479 ? 479 : (int)x, y < 0 ? 0 : y > 319 ? 319 : (int)y);
}

static float axis(uint8_t v) {
    float f = (v - 128) / 127.0f;
    return (f > 0.2f || f < -0.2f) ? f : 0.0f;
}

// ---------------------------------------------------------------- right stick camera orbit

#define LOOK_DZ 0.15f
#define LOOK_SPEED_X 9.0f // px/frame at level 10 (480x320 game space)
#define LOOK_SPEED_Y 6.0f
#define CAM_WRAP 3000
#define CAM_LEVEL_DEFAULT 5
#define CAM_LEVEL_MAX 10
#define CAM_CFG DATA_PATH "camera_sens.txt"
#define FID_CAM 3000

static int cam_level = CAM_LEVEL_DEFAULT, cam_active;
static float cam_x, cam_y, cam_ax = 240.0f, cam_ay = 120.0f;
static uint64_t cam_osd_until;

static void cam_load(void) {
    char b[8] = {0};
    SceUID fd = sceIoOpen(CAM_CFG, SCE_O_RDONLY, 0);
    if (fd < 0)
        return;
    sceIoRead(fd, b, sizeof(b) - 1);
    sceIoClose(fd);
    int v = atoi(b);
    if (v >= 1 && v <= CAM_LEVEL_MAX)
        cam_level = v;
}

static void cam_adjust(int d) {
    cam_level = cam_level + d < 1 ? 1 : cam_level + d > CAM_LEVEL_MAX ? CAM_LEVEL_MAX : cam_level + d;
    char b[8];
    int n = snprintf(b, sizeof(b), "%d\n", cam_level);
    SceUID fd = sceIoOpen(CAM_CFG, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (fd >= 0) {
        sceIoWrite(fd, b, n);
        sceIoClose(fd);
    }
    cam_osd_until = sceKernelGetProcessTimeWide() + 1500000;
    l_info("camera sensitivity %d/%d", cam_level, CAM_LEVEL_MAX);
}

static float look_curve(float v) {
    float a = v < 0 ? -v : v;
    if (a <= LOOK_DZ)
        return 0.0f;
    float t = (a - LOOK_DZ) / (1.0f - LOOK_DZ);
    t = t > 1.0f ? 1.0f : t;
    t = 0.35f * t + 0.65f * t * t;
    return v < 0 ? -t : t;
}

static void cam_update(void) {
    float k = (float)cam_level / CAM_LEVEL_MAX;
    float vx = look_curve((pad.rx - 128) / 127.0f) * LOOK_SPEED_X * k;
    float vy = look_curve((pad.ry - 128) / 127.0f) * LOOK_SPEED_Y * k;
    if (!vx && !vy) {
        cam_active = 0;
        return;
    }
    if (!cam_active) {
        cam_x = cam_ax;
        cam_y = cam_ay;
        cam_active = 1;
    } else {
        cam_x += vx;
        cam_y += vy;
        if (cam_x < cam_ax - CAM_WRAP || cam_x > cam_ax + CAM_WRAP ||
            cam_y < cam_ay - CAM_WRAP || cam_y > cam_ay + CAM_WRAP) {
            cam_active = 0;
            return;
        }
    }
    add_touch_raw(FID_CAM, (int)cam_x, (int)cam_y);
}

void patch_overlay(void) {
    // Avoid invoking glClear / glScissor on the display framebuffer during presentation
}

// ---------------------------------------------------------------- physical controls mapping

void patch_input(void) {
    static uint32_t prev;
    uint32_t now = pad.buttons, pressed = now & ~prev;
    prev = now;

    static int loaded;
    if (!loaded++) {
        cam_load();
    }

    // L + R: toggle opacity between 1% and 100% for joystick and buttons A/B
    const uint32_t LR = SCE_CTRL_LTRIGGER | SCE_CTRL_RTRIGGER;
    if ((now & LR) == LR) {
        if (pressed & LR) {
            hud_alpha = (hud_alpha <= 1.0f) ? 100.0f : 1.0f;
            l_info("HUD controls opacity toggled to %.0f%%", hud_alpha);
        }
        pad.buttons &= ~LR;
    }

    // SELECT + Up/Down: camera sensitivity adjust
    if (now & SCE_CTRL_SELECT) {
        if (pressed & SCE_CTRL_UP)
            cam_adjust(+1);
        if (pressed & SCE_CTRL_DOWN)
            cam_adjust(-1);
        if ((now & SCE_CTRL_UP) || (now & SCE_CTRL_DOWN))
            pad.buttons &= ~(SCE_CTRL_UP | SCE_CTRL_DOWN);
    }

    // --- Virtual D-Pad (Left Stick / Physical D-Pad) ---
    float dpad_x = 0.0f, dpad_y = 0.0f;
    if (pad.buttons & SCE_CTRL_LEFT) dpad_x -= 1.0f;
    if (pad.buttons & SCE_CTRL_RIGHT) dpad_x += 1.0f;
    if (pad.buttons & SCE_CTRL_UP) dpad_y -= 1.0f;
    if (pad.buttons & SCE_CTRL_DOWN) dpad_y += 1.0f;
    if (dpad_x != 0.0f && dpad_y != 0.0f) {
        dpad_x *= 0.7071f;
        dpad_y *= 0.7071f;
    }

    float lx = axis(pad.lx), ly = axis(pad.ly);
    if (dpad_x != 0.0f || dpad_y != 0.0f) {
        lx = dpad_x;
        ly = dpad_y;
    }

    static int stick_down = 0;
    if (lx != 0.0f || ly != 0.0f) {
        const float cx = 90.0f, cy = 235.0f, rad = 45.0f;
        // First frame: touch at center to register touchBegan on D-Pad
        // Subsequent frames: drag to deflection to steer
        add_touch(2000, stick_down ? (cx + lx * rad) : cx, stick_down ? (cy + ly * rad) : cy);
        stick_down = 1;
    } else {
        stick_down = 0;
    }

    // --- Button A: Cross / R-Trigger (Ollie / Jump / Confirm) ---
    if (pad.buttons & (SCE_CTRL_CROSS | SCE_CTRL_RTRIGGER)) {
        add_touch(1001, 434.0f, 276.0f);
    }

    // --- Button B: Square / Circle / L-Trigger (Grind / Trick) ---
    if (pad.buttons & (SCE_CTRL_SQUARE | SCE_CTRL_CIRCLE | SCE_CTRL_LTRIGGER)) {
        add_touch(1002, 350.0f, 241.0f);
    }

    // --- Triangle: Trick modifier / special trick ---
    if (pad.buttons & SCE_CTRL_TRIANGLE) {
        add_touch(1003, 350.0f, 241.0f);
    }

    // --- Start: Pause / Menu ---
    if (pad.buttons & SCE_CTRL_START) {
        add_touch(1004, 450.0f, 25.0f);
    }

    // --- Select: Map / Objectives (only if not holding Up/Down for camera sensitivity) ---
    if ((pad.buttons & SCE_CTRL_SELECT) && !(pad.buttons & (SCE_CTRL_UP | SCE_CTRL_DOWN))) {
        add_touch(1005, 355.0f, 23.0f);
    }

    // --- Right Stick: Camera Orbit ---
    cam_update();
}
