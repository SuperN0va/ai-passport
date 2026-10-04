#include "dex_ui.h"
#include "src/misc/cache/instance/lv_image_cache.h"
#include "dex_sprite.h"
#include "dex_glyphs.h"
#include "dex_visual_assets.h"
#include <stdio.h>
#include <string.h>

LV_FONT_DECLARE(dex_font_16);
LV_FONT_DECLARE(dex_font_20);

#define PAPER 0xedf5fc
#define INK 0x122a42
#define MUTED 0x426077
#define RED 0xc9353b
#define SELECTED 0xdce9f4
#define CYAN 0x37d5e8
#define GOLD 0xffd35b
#define SPRITE_BG 0x244238

static lv_obj_t *s_screen;
static const uint8_t *s_blob;
static size_t s_blob_length;
static uint16_t s_pixels[96 * 96];
static unsigned s_sprite_id;
static bool s_sprite_ok;
static lv_image_dsc_t s_image;

static lv_obj_t *box(lv_obj_t *parent, int x, int y, int w, int h, uint32_t color) {
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    return obj;
}

static lv_obj_t *label(lv_obj_t *parent, const char *text, int x, int y,
                       int width, bool large, uint32_t color) {
    lv_obj_t *obj = lv_label_create(parent);
    lv_obj_set_style_text_font(obj, large ? &dex_font_20 : &dex_font_16, 0);
    lv_obj_set_style_text_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_text_line_space(obj, 4, 0);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_width(obj, width);
    lv_label_set_text(obj, text);
    return obj;
}

static void one_line(lv_obj_t *parent, const char *text, int x, int y,
                     int width, bool large, uint32_t color) {
    lv_obj_t *obj = label(parent, text, x, y, width, large, color);
    lv_label_set_long_mode(obj, LV_LABEL_LONG_MODE_DOTS);
}

static void header(const char *title, int battery) {
    lv_obj_t *band = box(s_screen, 0, 0, 240, 43, RED);
    box(s_screen, 0, 41, 240, 2, CYAN);
    one_line(band, title, 22, 13, 156, false, PAPER);
    char buf[12];
    if (battery < 0) snprintf(buf, sizeof(buf), "--%%");
    else snprintf(buf, sizeof(buf), "%d%%", battery);
    one_line(band, buf, 181, 14, 44, false, PAPER);
}

static void footer(const char *text, bool storage_ok) {
    lv_obj_t *line = box(s_screen, 12, 292, 216, 1, 0xb0c6d8);
    (void)line;
    label(s_screen, storage_ok ? text : "存储失败 · 请重试", 23, 297, 194, false,
          storage_ok ? MUTED : RED);
}

static void type_icon(lv_obj_t *parent, unsigned type, int x, int y, bool large) {
    if (type < 1 || type > 18) return;
    lv_obj_t *image = lv_image_create(parent);
    lv_image_set_src(image, large ? &dex_type_icons_24[type] : &dex_type_icons_20[type]);
    lv_obj_set_pos(image, x, y);
}

static void type_badge(lv_obj_t *parent, unsigned type, int x, int y, int width) {
    if (type < 1 || type > 18) return;
    lv_obj_t *badge = box(parent, x, y, width, 26, dex_type_colors[type]);
    type_icon(badge, type, 3, 3, false);
    one_line(badge, dex_types[type], 28, 3, width - 30, false, dex_type_foregrounds[type]);
}

static lv_obj_t *row(unsigned slot, const char *text, bool selected, bool marked,
                     unsigned icon, bool pokemon) {
    lv_obj_t *card = box(s_screen, 12, 69 + (int)slot * 36, 216, 33,
                         selected ? INK : 0xffffff);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(selected ? CYAN : 0xb5cbdd), 0);
    if (selected) box(card, 0, 6, 3, 21, GOLD);
    if (icon) type_icon(card, icon, 9, 6, false);
    int x = icon ? 37 : 9;
    one_line(card, text, x, 6, pokemon ? 147 : 207 - x, false, selected ? PAPER : INK);
    if (marked) box(card, 4, 3, 3, 3, GOLD);
    return card;
}

void dex_ui_init(const uint8_t *sprites, size_t length) {
    s_blob = sprites;
    s_blob_length = length;
    s_screen = lv_screen_active();
    lv_obj_remove_style_all(s_screen);
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(PAPER), 0);
    lv_obj_set_style_bg_opa(s_screen, LV_OPA_COVER, 0);
    lv_obj_remove_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);
    s_image = (lv_image_dsc_t){
        .header = {.magic = LV_IMAGE_HEADER_MAGIC, .cf = LV_COLOR_FORMAT_RGB565,
                   .w = 96, .h = 96, .stride = 192},
        .data_size = sizeof(s_pixels), .data = (const uint8_t *)s_pixels
    };
}

bool dex_ui_check_fonts(void) {
    const lv_font_t *fonts[] = {&dex_font_16, &dex_font_20};
    for (unsigned f = 0; f < 2; ++f) {
        for (size_t i = 0; i < sizeof(dex_glyphs) / sizeof(dex_glyphs[0]); ++i) {
            lv_font_glyph_dsc_t d = {0};
            if (!lv_font_get_glyph_dsc(fonts[f], &d, dex_glyphs[i], 0) || d.is_placeholder) return false;
        }
        lv_font_glyph_dsc_t missing = {0};
        if (lv_font_get_glyph_dsc(fonts[f], &missing, 0x10ffff, 0) && !missing.is_placeholder) return false;
    }
    return true;
}

void dex_ui_error(const char *message) {
    lv_obj_clean(s_screen);
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(PAPER), 0);
    header("图鉴启动检查", -1);
    label(s_screen, message, 20, 83, 200, false, RED);
}

void dex_ui_render(dex_model_t *m, int battery, bool storage_ok) {
    lv_obj_clean(s_screen);
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(m->page == DEX_SPLASH ? INK : PAPER), 0);
    char buf[256];
    const dex_entry_t *e = dex_entry(m->detail_id);
    switch (m->page) {
    case DEX_SPLASH: {
        one_line(s_screen, "全国图鉴 · 0001–1025", 33, 24, 190, false, CYAN);
        const int stars[][2] = {{20, 53}, {213, 72}, {17, 200}, {216, 218}, {39, 239}, {191, 38}};
        for (unsigned i = 0; i < sizeof(stars) / sizeof(stars[0]); ++i)
            box(s_screen, stars[i][0], stars[i][1], 3, 3, i % 2 ? CYAN : GOLD);
        lv_obj_t *logo = lv_image_create(s_screen);
        lv_image_set_src(logo, &dex_logo);
        lv_obj_set_pos(logo, (240 - (int)dex_logo.header.w) / 2, 54 + (180 - (int)dex_logo.header.h) / 2);
        box(s_screen, 57, 246, 126, 2, CYAN);
        one_line(s_screen, "你的掌上宝可梦世界", 48, 257, 156, false, PAPER);
        one_line(s_screen, "按任意键开始", 64, 289, 120, false, GOLD);
        break;
    }
    case DEX_HOME: {
        header("口袋图鉴", battery);
        label(s_screen, "探索，从这里开始", 18, 52, 205, true, INK);
        snprintf(buf, sizeof(buf), "离线资料 · %s", m->generation || m->type ? "筛选已保留" : "全国编号");
        label(s_screen, buf, 18, 81, 210, false, MUTED);
        const char *items[] = {"全部图鉴", "按世代浏览", "按属性浏览", "编号 / 简拼检索", "我的收藏"};
        const char *marks[] = {"01", "02", "03", "04", "05"};
        for (unsigned i = 0; i < 5; ++i) {
            bool selected = i == m->home_cursor;
            lv_obj_t *card = box(s_screen, 14, 112 + (int)i * 34, 212, 30, selected ? INK : 0xffffff);
            lv_obj_set_style_border_width(card, 1, 0);
            lv_obj_set_style_border_color(card, lv_color_hex(selected ? CYAN : 0xb5cbdd), 0);
            label(card, marks[i], 10, 4, 30, false, selected ? GOLD : RED);
            label(card, items[i], 45, 4, 162, false, selected ? PAPER : INK);
        }
        footer("上下选择  OK打开", storage_ok);
        break;
    }
    case DEX_LIST: {
        header(m->favorites_only ? "我的收藏" : m->from_search ? "检索结果" : "全国图鉴", battery);
        if (m->from_search) snprintf(buf, sizeof(buf), "%s · %u 个结果", m->query, m->count);
        else {
            char generation[24];
            if (m->generation) snprintf(generation, sizeof(generation), "Gen %u", m->generation);
            else snprintf(generation, sizeof(generation), "全部世代");
            snprintf(buf, sizeof(buf), "%s %s · %u/%u", generation,
                     m->type ? dex_types[m->type] : "全部属性", m->count ? m->cursor + 1 : 0, m->count);
        }
        one_line(s_screen, buf, 16, 47, 211, false, MUTED);
        if (!m->count) {
            label(s_screen, m->favorites_only ? "还没有收藏\n在详情页添加喜欢的宝可梦" : "没有匹配结果\n长按 OK 返回调整", 22, 118, 196, false, MUTED);
        }
        unsigned start = m->cursor / 6 * 6;
        for (unsigned j = start; j < m->count && j < start + 6; ++j) {
            unsigned id = m->results[j];
            snprintf(buf, sizeof(buf), "%04u  %s", id, dex_entries[id].name);
            lv_obj_t *card = row(j - start, buf, j == m->cursor, dex_favorite(m, id), 0, true);
            const dex_entry_t *item = &dex_entries[id];
            type_icon(card, item->type1, 164, 6, false);
            type_icon(card, item->type2, 188, 6, false);
        }
        footer("OK详情  长按OK返回", storage_ok);
        break;
    }
    case DEX_GENERATION: case DEX_TYPE: {
        bool generation = m->page == DEX_GENERATION;
        header(generation ? "按世代浏览" : "按属性浏览", battery);
        one_line(s_screen, "可组合筛选 · 选择后查看", 16, 47, 212, false, MUTED);
        unsigned start = m->option / 6 * 6;
        unsigned total = generation ? 10 : 19;
        const unsigned first[] = {0, 1, 152, 252, 387, 494, 650, 722, 810, 906};
        for (unsigned j = start; j < total && j < start + 6; ++j) {
            if (!j) snprintf(buf, sizeof(buf), "%s", generation ? "全部世代" : "全部属性");
            else if (generation) snprintf(buf, sizeof(buf), "Gen %u  ·  %04u–%04u", j, first[j], j < 9 ? first[j + 1] - 1 : 1025);
            else snprintf(buf, sizeof(buf), "%s", dex_types[j]);
            row(j - start, buf, j == m->option, false, generation ? 0 : j, false);
        }
        footer("OK筛选  长按OK返回", storage_ok);
        break;
    }
    case DEX_SEARCH: {
        header("编号 / 简拼检索", battery);
        label(s_screen, "25 找皮卡丘 · PKQ 也可以", 17, 51, 206, false, MUTED);
        lv_obj_t *input = box(s_screen, 15, 84, 210, 43, INK);
        snprintf(buf, sizeof(buf), "%s_", m->query);
        label(input, buf, 13, 9, 187, true, PAPER);
        const char *chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        for (unsigned i = 0; i < 36; ++i) {
            bool selected = i == m->character;
            char c[2] = {chars[i], 0};
            /* A styled label avoids a second object for every keyboard key. */
            lv_obj_t *cell = label(s_screen, c, 16 + (int)(i % 9) * 23,
                                  143 + (int)(i / 9) * 26, 21, false, selected ? PAPER : INK);
            lv_obj_set_height(cell, 24);
            lv_obj_set_style_text_align(cell, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_set_style_pad_top(cell, 2, 0);
            lv_obj_set_style_bg_color(cell, lv_color_hex(selected ? RED : SELECTED), 0);
            lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
            lv_obj_set_style_radius(cell, 0, 0);
        }
        lv_obj_t *del = box(s_screen, 16, 251, 97, 30, m->character == 36 ? RED : SELECTED);
        label(del, "删除", 31, 5, 64, false, m->character == 36 ? PAPER : INK);
        lv_obj_t *go = box(s_screen, 122, 251, 102, 30, m->character == 37 ? RED : SELECTED);
        label(go, "查找", 34, 5, 64, false, m->character == 37 ? PAPER : INK);
        footer("上下选字符  OK输入", storage_ok);
        break;
    }
    case DEX_DETAIL: {
        if (!e) break;
        snprintf(buf, sizeof(buf), "NO.%04u   Gen %u", m->detail_id, e->gen);
        header(buf, battery);
        one_line(s_screen, e->name, 18, 49, 209, true, INK);
        box(s_screen, 14, 82, 110, 108, SPRITE_BG);
        if (s_sprite_id != m->detail_id) {
            s_sprite_id = m->detail_id;
            s_sprite_ok = dex_sprite_decode(s_blob, s_blob_length, m->detail_id, s_pixels);
            lv_image_cache_drop(&s_image);
        }
        if (s_sprite_ok) {
            lv_obj_t *img = lv_image_create(s_screen);
            lv_image_set_src(img, &s_image);
            lv_obj_set_pos(img, 21, 88);
        } else label(s_screen, "图片不可用", 22, 123, 97, false, PAPER);
        type_badge(s_screen, e->type1, 132, 83, 96);
        type_badge(s_screen, e->type2, 132, 113, 96);
        snprintf(buf, sizeof(buf), "%u.%u m\n%u.%u kg", e->height / 10, e->height % 10, e->weight / 10, e->weight % 10);
        label(s_screen, buf, 133, 144, 94, false, INK);
        const char *actions[] = {"图鉴资料", "可学招式", dex_favorite(m, m->detail_id) ? "取消收藏  *" : "加入收藏"};
        for (unsigned i = 0; i < 3; ++i) {
            bool selected = i == m->detail_action;
            lv_obj_t *card = box(s_screen, 14, 198 + (int)i * 30, 212, 27, selected ? INK : SELECTED);
            label(card, actions[i], 13, 3, 186, false, selected ? PAPER : INK);
        }
        footer("OK选择  长按OK返回", storage_ok);
        break;
    }
    case DEX_INFO: {
        if (!e) break;
        header("图鉴资料", battery);
        lv_obj_t *body = box(s_screen, 16, 48, 210, 237, PAPER);
        lv_obj_add_flag(body, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(body, LV_SCROLLBAR_MODE_OFF);
        label(body, e->name, 0, 0, 208, true, INK);
        type_badge(body, e->type1, 0, 30, 96);
        type_badge(body, e->type2, 105, 30, 96);
        snprintf(buf, sizeof(buf), "图鉴来源：%s", e->description_version);
        lv_obj_t *source = label(body, buf, 0, 65, 204, false, MUTED);
        lv_obj_update_layout(source);
        int y = 71 + lv_obj_get_height(source);
        lv_obj_t *desc = label(body, e->description, 0, y, 204, false, INK);
        lv_obj_update_layout(desc);
        y += lv_obj_get_height(desc) + 18;
        label(body, "种族值", 0, y, 204, false, RED);
        const char *names[] = {"HP", "攻击", "防御", "特攻", "特防", "速度"};
        for (unsigned j = 0; j < 6; ++j) {
            snprintf(buf, sizeof(buf), "%s  %u", names[j], e->stats[j]);
            label(body, buf, 0, y + 25 + (int)j * 23, 103, false, INK);
            box(body, 107, y + 32 + (int)j * 23, e->stats[j] * 91 / 255 + 1, 6, RED);
        }
        lv_obj_update_layout(body);
        lv_obj_scroll_to_y(body, m->scroll, LV_ANIM_OFF);
        m->scroll = (uint16_t)lv_obj_get_scroll_y(body);
        footer("上下滚动  长按OK返回", storage_ok);
        break;
    }
    case DEX_MOVES: {
        if (!e) break;
        snprintf(buf, sizeof(buf), "招式  Gen %u", e->move_gen);
        header(buf, battery);
        snprintf(buf, sizeof(buf), "%s  %u/%u", dex_groups[e->move_group], m->move_cursor + 1, e->move_count);
        one_line(s_screen, buf, 16, 47, 211, false, RED);
        unsigned start = m->move_cursor / 6 * 6;
        for (unsigned j = start; j < e->move_count && j < start + 6; ++j) {
            const dex_move_t *mv = dex_move(m->detail_id, j);
            if (mv) row(j - start, mv->name, j == m->move_cursor, false, mv->type, false);
        }
        footer("OK参数  长按OK返回", storage_ok);
        break;
    }
    case DEX_MOVE_DETAIL: {
        const dex_move_t *mv = dex_move(m->detail_id, m->move_cursor);
        if (!e || !mv) break;
        header("招式参数", battery);
        label(s_screen, mv->name, 18, 57, 205, true, INK);
        type_badge(s_screen, mv->type, 18, 92, 104);
        snprintf(buf, sizeof(buf), "Gen %u", e->move_gen);
        label(s_screen, buf, 135, 95, 84, false, MUTED);
        const char *names[] = {"威力", "命中", "PP"};
        int values[] = {mv->power, mv->accuracy, mv->pp};
        for (unsigned j = 0; j < 3; ++j) {
            lv_obj_t *card = box(s_screen, 17, 129 + (int)j * 34, 206, 30, SELECTED);
            label(card, names[j], 11, 5, 95, false, INK);
            if (values[j] < 0) snprintf(buf, sizeof(buf), "—");
            else snprintf(buf, sizeof(buf), "%d%s", values[j], j == 1 ? "%" : "");
            label(card, buf, 118, 5, 84, false, INK);
        }
        label(s_screen, dex_groups[e->move_group], 18, 240, 205, false, MUTED);
        label(s_screen, "— 表示无固定数值", 18, 265, 205, false, MUTED);
        footer("长按 OK 返回招式表", storage_ok);
        break;
    }
    }
}
