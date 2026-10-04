#include "dex_ui.h"
#include "bsp_display.h"
#include "bsp_button.h"
#include "bsp_battery.h"
#include "bsp_pins.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "pocket_dex";
extern const uint8_t dex_sprites_start[] asm("_binary_sprites_bin_start");
extern const uint8_t dex_sprites_end[] asm("_binary_sprites_bin_end");

typedef struct { bsp_btn_t button; bsp_btn_ev_t event; } input_event_t;
static QueueHandle_t s_queue;
static dex_model_t s_model;
static nvs_handle_t s_storage;
static bool s_storage_open;
static bool s_storage_ok;
static const int s_windows[BSP_BTN_COUNT][2] = BSP_BTN_MV_TABLE;

static void button_event(bsp_btn_t button, bsp_btn_ev_t event, void *user) {
    (void)user;
    input_event_t input = {button, event};
    if (s_queue) (void)xQueueSend(s_queue, &input, 0);
}

static void storage_load(void) {
    /* Never erase someone else's NVS to recover our own application. */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_OK) err = nvs_open("pocket_dex", NVS_READWRITE, &s_storage);
    s_storage_open = err == ESP_OK;
    s_storage_ok = s_storage_open;
    if (!s_storage_open) { ESP_LOGW(TAG, "Persistence unavailable: %s", esp_err_to_name(err)); return; }
    uint8_t bytes[DEX_SAVE_BYTES];
    size_t size = sizeof(bytes);
    err = nvs_get_blob(s_storage, "state_v1", bytes, &size);
    if (err == ESP_OK && !dex_save_decode(&s_model, bytes, size))
        ESP_LOGW(TAG, "Invalid saved record: using clean application defaults");
    else if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        s_storage_ok = false;
        ESP_LOGW(TAG, "Cannot read state: %s", esp_err_to_name(err));
    }
}

static void storage_save(void) {
    if (!s_model.storage_dirty) return;
    if (!s_storage_open) { s_storage_ok = false; return; }
    uint8_t bytes[DEX_SAVE_BYTES];
    dex_save_encode(&s_model, bytes);
    esp_err_t err = nvs_set_blob(s_storage, "state_v1", bytes, sizeof(bytes));
    if (err == ESP_OK) err = nvs_commit(s_storage);
    s_storage_ok = err == ESP_OK;
    if (s_storage_ok) s_model.storage_dirty = false;
    else ESP_LOGW(TAG, "State save failed: %s", esp_err_to_name(err));
}

static void app_task(void *arg) {
    (void)arg;
    dex_model_init(&s_model);
    storage_load();
    bool battery_ready = bsp_battery_init() == ESP_OK;
    int battery = battery_ready ? bsp_battery_soc() : -1;
    if (!bsp_lvgl_lock(1000)) { ESP_LOGE(TAG, "Initial LVGL lock failed"); vTaskDelete(NULL); return; }
    dex_ui_init(dex_sprites_start, (size_t)(dex_sprites_end - dex_sprites_start));
    bool fonts_ok = dex_ui_check_fonts();
    if (fonts_ok) dex_ui_render(&s_model, battery, s_storage_ok);
    else dex_ui_error("字库检查失败，请重新生成固件。");
    bsp_lvgl_unlock();
    bsp_display_backlight(70);
    if (!fonts_ok) { ESP_LOGE(TAG, "Font coverage failed"); vTaskDelete(NULL); return; }
    esp_err_t buttons = bsp_button_init(button_event, NULL);
    if (buttons != ESP_OK) {
        if (bsp_lvgl_lock(1000)) { dex_ui_error("按键初始化失败，请重启设备。"); bsp_lvgl_unlock(); }
        ESP_LOGE(TAG, "Buttons unavailable: %s", esp_err_to_name(buttons));
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "READY species=%u fonts=PASS heap=%u largest=%u", DEX_COUNT,
             (unsigned)esp_get_free_heap_size(),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    int64_t last_activity = esp_timer_get_time();
    const int64_t intro_started = last_activity;
    int64_t last_battery = last_activity;
    int64_t repeat_at = 0;
    int repeat_button = -1;
    int brightness = 70;
    bool refresh = false;
    bool wake_gesture = false;
    for (;;) {
        input_event_t event;
        bool received = xQueueReceive(s_queue, &event, pdMS_TO_TICKS(60)) == pdTRUE;
        int64_t now = esp_timer_get_time();
        if (s_model.page == DEX_SPLASH) {
            if (received && event.event == BSP_BTN_PRESS) {
                dex_model_input(&s_model, DEX_OK);
                wake_gesture = true; /* Consume the rest of this skip gesture. */
                refresh = true;
            } else refresh = dex_model_boot_tick(&s_model, (uint32_t)((now - intro_started) / 1000));
            received = false;
            if (!refresh) continue;
            last_activity = now;
        }
        if (received) {
            bool was_dark = brightness == 0;
            last_activity = now;
            if (brightness != 70) { brightness = 70; bsp_display_backlight(70); }
            if (event.event == BSP_BTN_PRESS) wake_gesture = was_dark;
            if (wake_gesture) {
                repeat_button = -1; /* First gesture only wakes the display. */
            } else if (event.button == BSP_BTN_OK) {
                if (event.event == BSP_BTN_CLICK || event.event == BSP_BTN_DOUBLE || event.event == BSP_BTN_LONG) {
                    dex_model_input(&s_model, event.event == BSP_BTN_LONG ? DEX_BACK : DEX_OK);
                    refresh = true;
                }
            } else if (event.event == BSP_BTN_PRESS) {
                dex_model_input(&s_model, event.button == BSP_BTN_UP ? DEX_UP : DEX_DOWN);
                repeat_button = event.button;
                repeat_at = now + 500000;
                refresh = true;
            }
        }
        /* Poll only the BSP-owned calibrated reader. The callbacks still just
         * enqueue events; repeating never blocks the shared timer task. */
        if (repeat_button >= 0 && now >= repeat_at) {
            int mv = bsp_button_read_mv();
            if (mv >= s_windows[repeat_button][0] && mv < s_windows[repeat_button][1]) {
                dex_model_input(&s_model, repeat_button == BSP_BTN_UP ? DEX_UP : DEX_DOWN);
                repeat_at = now + 110000;
                last_activity = now;
                refresh = true;
            } else repeat_button = -1;
        }
        if (now - last_battery >= 30000000) {
            battery = battery_ready ? bsp_battery_soc() : -1;
            last_battery = now;
            refresh = true;
        }
        int target_brightness = now - last_activity > 90000000 ? 0 : now - last_activity > 30000000 ? 12 : 70;
        if (target_brightness != brightness) { brightness = target_brightness; bsp_display_backlight((uint8_t)brightness); }
        if (!refresh) continue;
        /* Serialize/commit in our worker outside the LVGL lock. The next frame
         * shows the actual save result, never an uncommitted success message. */
        storage_save();
        if (bsp_lvgl_lock(500)) {
            dex_ui_render(&s_model, battery, s_storage_ok);
            bsp_lvgl_unlock();
            refresh = false;
        }
    }
}

void app_main(void) {
    ESP_ERROR_CHECK(bsp_display_init());
    if (!bsp_lvgl_init()) { ESP_LOGE(TAG, "Display/LVGL unavailable"); return; }
    s_queue = xQueueCreate(16, sizeof(input_event_t));
    if (!s_queue || xTaskCreate(app_task, "pocket_dex", 8192, NULL, 4, NULL) != pdPASS)
        ESP_LOGE(TAG, "Cannot allocate application task");
}
