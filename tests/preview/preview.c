#include "dex_ui.h"
#include "dex_visual_assets.h"
#include "bsp_display_rounding.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint16_t frame[240 * 320];
static uint16_t draw_buffer[240 * 40];

static void flush(lv_display_t *display, const lv_area_t *area, uint8_t *pixels) {
    const uint16_t *src = (uint16_t *)pixels;
    for (int y = area->y1; y <= area->y2; ++y) {
        for (int x = area->x1; x <= area->x2; ++x) frame[y * 240 + x] = *src++;
    }
    lv_display_flush_ready(display);
}

static void snapshot(const char *path, lv_display_t *display) {
    lv_tick_inc(30);
    lv_refr_now(display);
    FILE *f = fopen(path, "wb");
    assert(f);
    fputs("P6\n240 320\n255\n", f);
    for (int y = 0; y < 320; ++y) {
        int32_t x1, x2;
        assert(bsp_display_rounded_row_span(y, 240, 320, 30, &x1, &x2));
        for (int x = 0; x < 240; ++x) {
            unsigned c = x < x1 || x > x2 ? 0 : frame[y * 240 + x];
            uint8_t rgb[] = {(uint8_t)(((c >> 11) & 31) * 255 / 31),
                             (uint8_t)(((c >> 5) & 63) * 255 / 63), (uint8_t)((c & 31) * 255 / 31)};
            fwrite(rgb, 1, 3, f);
        }
    }
    fclose(f);
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    assert(argc == 3); /* sprite blob, output directory */
    FILE *f = fopen(argv[1], "rb");
    assert(f); fseek(f, 0, SEEK_END); long size = ftell(f); rewind(f);
    uint8_t *blob = malloc((size_t)size);
    assert(blob && fread(blob, 1, (size_t)size, f) == (size_t)size); fclose(f);
    lv_init();
    lv_display_t *display = lv_display_create(240, 320);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, draw_buffer, NULL, sizeof(draw_buffer), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush);
    dex_ui_init(blob, (size_t)size);
    assert(dex_ui_check_fonts());
    for (unsigned type = 1; type <= 18; ++type) {
        assert(dex_type_icons_20[type].header.w == 20 && dex_type_icons_20[type].header.h == 20);
        assert(dex_type_icons_24[type].header.w == 24 && dex_type_icons_24[type].header.h == 24);
        assert(dex_type_icons_20[type].data_size == 20 * 20 * 3);
        assert(dex_type_icons_24[type].data_size == 24 * 24 * 3);
        assert(dex_type_colors[type] && dex_type_foregrounds[type]);
    }
    assert(dex_logo.header.w <= 208 && dex_logo.header.h <= 180);
    puts("Compiled LVGL 9.5.0 fonts: all inventoried glyphs + negative control PASS");
    dex_model_t model;
    dex_model_init(&model);
    dex_ui_render(&model, 100, true);
    char path[1024];
    snprintf(path, sizeof(path), "%s/splash.ppm", argv[2]); snapshot(path, display);
    const dex_page_t pages[] = {DEX_HOME, DEX_LIST, DEX_DETAIL, DEX_INFO, DEX_MOVES,
                               DEX_MOVE_DETAIL, DEX_GENERATION, DEX_TYPE, DEX_SEARCH};
    for (unsigned i = 0; i < sizeof(pages) / sizeof(pages[0]); ++i) {
        model.page = pages[i]; model.detail_id = 25;
        if (model.page == DEX_SEARCH) strcpy(model.query, "PKQ");
        dex_ui_render(&model, 86, true);
        snprintf(path, sizeof(path), "%s/%02u.ppm", argv[2], i);
        snapshot(path, display);
    }
    model.page = DEX_DETAIL; model.detail_id = 1025;
    dex_ui_render(&model, -1, true);
    snprintf(path, sizeof(path), "%s/09.ppm", argv[2]); snapshot(path, display);
    model.page = DEX_INFO; model.detail_id = 906; model.scroll = 0;
    dex_ui_render(&model, 86, true);
    snprintf(path, sizeof(path), "%s/10.ppm", argv[2]); snapshot(path, display);
    for (unsigned option = 6; option <= 18; option += 6) {
        model.page = DEX_TYPE; model.option = (uint8_t)option;
        dex_ui_render(&model, 100, true);
        snprintf(path, sizeof(path), "%s/types-%u.ppm", argv[2], option); snapshot(path, display);
    }
    model.option = 0; model.page = DEX_DETAIL; model.detail_id = 1;
    dex_ui_render(&model, 100, true);
    snprintf(path, sizeof(path), "%s/dual-type.ppm", argv[2]); snapshot(path, display);
    model.page = DEX_LIST; model.favorites_only = true; model.count = 0;
    dex_ui_render(&model, 100, true);
    snprintf(path, sizeof(path), "%s/empty-favorites.ppm", argv[2]); snapshot(path, display);
    dex_model_init(&model);
    model.page = DEX_HOME;
    dex_ui_render(&model, 100, false);
    snprintf(path, sizeof(path), "%s/storage-error.ppm", argv[2]); snapshot(path, display);
    model.page = DEX_SPLASH;
    dex_ui_render(&model, 100, true); lv_refr_now(display);
    dex_ui_error("按键初始化失败，请重启设备。");
    snprintf(path, sizeof(path), "%s/startup-error.ppm", argv[2]); snapshot(path, display);
    model.page = DEX_HOME;
    dex_ui_render(&model, 86, true); lv_refr_now(display);
    lv_mem_monitor_t before, after;
    lv_mem_monitor(&before);
    size_t minimum_free = before.free_size;
    size_t minimum_largest = before.free_biggest_size;
    for (unsigned i = 0; i < 4000; ++i) {
        unsigned step = i % 2000;
        model.page = step % 10 == 9 ? DEX_SPLASH : pages[step % 10];
        model.detail_id = (uint16_t)(step % 1025 + 1);
        dex_ui_render(&model, 86, true);
        lv_tick_inc(30); lv_refr_now(display);
        lv_mem_monitor(&after);
        if (after.free_size < minimum_free) minimum_free = after.free_size;
        if (after.free_biggest_size < minimum_largest) minimum_largest = after.free_biggest_size;
        if (i == 1999) {
            model.page = DEX_HOME;
            dex_ui_render(&model, 86, true); lv_refr_now(display);
            lv_mem_monitor(&after);
            printf("Initial render warmup: free=%zu -> %zu\n", before.free_size, after.free_size);
            before = after;
        }
    }
    model.page = DEX_HOME;
    dex_ui_render(&model, 86, true); lv_refr_now(display);
    lv_mem_monitor(&after);
    printf("2000 further rendered page changes: free=%zu -> %zu; largest=%zu\n",
           before.free_size, after.free_size, after.free_biggest_size);
    printf("48 KiB pool: minimum free=%zu; minimum largest block=%zu\n", minimum_free, minimum_largest);
    assert(after.free_size >= before.free_size);
    assert(minimum_largest >= 4096);
    free(blob);
    puts("Headless UI stress: PASS (not a physical-device result)");
    return 0;
}
