#pragma once
#include "dex_model.h"
#include "lvgl.h"

/* All functions must run in the LVGL task or under the BSP LVGL lock.
 * Static screen/pixel ownership lasts for the whole application.
 * No background timers or callbacks hold pointers into rebuilt pages. */
void dex_ui_init(const uint8_t *sprites, size_t length);
void dex_ui_render(dex_model_t *model, int battery, bool storage_ok);
void dex_ui_error(const char *message);
bool dex_ui_check_fonts(void);
