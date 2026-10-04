#pragma once
#include "dex_data.h"
#include <stdbool.h>

#define DEX_FAVORITE_BYTES ((DEX_COUNT + 7) / 8)
#define DEX_SAVE_BYTES (8 + DEX_FAVORITE_BYTES + 4)
#define DEX_QUERY_MAX 8
#define DEX_SPLASH_MS 2400

typedef enum {
    DEX_HOME, DEX_LIST, DEX_GENERATION, DEX_TYPE, DEX_SEARCH,
    DEX_DETAIL, DEX_INFO, DEX_MOVES, DEX_MOVE_DETAIL, DEX_SPLASH
} dex_page_t;
typedef enum { DEX_UP, DEX_DOWN, DEX_OK, DEX_BACK, DEX_PAGE_UP, DEX_PAGE_DOWN } dex_input_t;

typedef struct {
    dex_page_t page;
    uint8_t favorites[DEX_FAVORITE_BYTES];
    uint16_t last_id;
    uint8_t generation;
    uint8_t type;
    bool favorites_only;
    bool from_search;
    char query[DEX_QUERY_MAX + 1];
    uint16_t results[DEX_COUNT];
    uint16_t count;
    uint16_t cursor;
    uint16_t detail_id;
    uint16_t move_cursor;
    uint16_t scroll;
    uint8_t home_cursor;
    uint8_t option;
    uint8_t detail_action;
    uint8_t character;
    bool storage_dirty;
} dex_model_t;

void dex_model_init(dex_model_t *m);
bool dex_model_boot_tick(dex_model_t *m, uint32_t elapsed_ms);
bool dex_favorite(const dex_model_t *m, unsigned id);
bool dex_matches(unsigned id, unsigned gen, unsigned type, const char *query);
void dex_refresh_results(dex_model_t *m, unsigned preferred_id);
void dex_model_input(dex_model_t *m, dex_input_t input);
void dex_save_encode(const dex_model_t *m, uint8_t out[DEX_SAVE_BYTES]);
bool dex_save_decode(dex_model_t *m, const uint8_t *data, size_t size);
