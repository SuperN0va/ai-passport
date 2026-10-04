#include "dex_model.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_data(void) {
    unsigned gens[10] = {0}, move_gens[10] = {0};
    assert(!dex_entry(0) && !dex_entry(1026));
    for (unsigned id = 1; id <= DEX_COUNT; ++id) {
        const dex_entry_t *e = dex_entry(id);
        assert(e && *e->name && *e->description && *e->description_version);
        assert(e->gen >= 1 && e->gen <= 9);
        assert(e->type1 >= 1 && e->type1 <= 18 && e->type2 <= 18);
        ++gens[e->gen]; ++move_gens[e->move_gen];
        assert(e->move_count > 0);
        for (unsigned j = 0; j < e->move_count; ++j) {
            const dex_move_t *mv = dex_move(id, j);
            assert(mv && *mv->name && mv->group == e->move_group);
            assert(mv->type >= 1 && mv->type <= 18);
            assert(mv->pp > 0 && mv->accuracy <= 100 && mv->accuracy >= -1);
            if (j) assert(dex_move(id, j - 1)->id < mv->id);
        }
        assert(!dex_move(id, e->move_count));
    }
    assert(gens[1] == 151 && gens[9] == 120);
    assert(move_gens[9] == 733 && move_gens[8] == 283 && move_gens[7] == 9);
    assert(dex_entries[25].gen == 1 && dex_entries[25].move_gen == 9);
    bool pla = false, sv = false;
    for (size_t i = 0; i < dex_move_variant_count; ++i) {
        if (dex_moves[i].id == 85 && dex_moves[i].group == 24) {
            assert(dex_moves[i].power == 80 && dex_moves[i].pp == 10); pla = true;
        }
        if (dex_moves[i].id == 85 && dex_moves[i].group == 25) {
            assert(dex_moves[i].power == 90 && dex_moves[i].pp == 15); sv = true;
        }
    }
    assert(pla && sv);
}

static void test_search(void) {
    assert(dex_matches(25, 0, 0, "25"));
    assert(dex_matches(25, 0, 0, "0025"));
    assert(dex_matches(25, 1, 13, "pkq"));
    assert(dex_matches(190, 0, 0, "CWGS"));
    assert(dex_matches(428, 0, 0, "CET"));
    assert(!dex_matches(25, 9, 13, "PKQ"));
    assert(!dex_matches(25, 0, 12, "PKQ"));
    assert(!dex_matches(25, 0, 0, "0"));
    assert(!dex_matches(25, 0, 0, "000000025"));
    assert(dex_matches(1, 1, 4, "")); /* second type matches */
    assert(dex_matches(1, 1, 12, ""));
    assert(!dex_matches(0, 0, 0, ""));
}

static void test_navigation(void) {
    dex_model_t m;
    dex_model_init(&m);
    assert(m.page == DEX_SPLASH);
    dex_model_input(&m, DEX_OK);
    assert(m.page == DEX_HOME);
    dex_model_input(&m, DEX_OK);
    assert(m.page == DEX_LIST && m.count == 1025);
    dex_model_input(&m, DEX_UP);
    assert(m.cursor == 1024);
    dex_model_input(&m, DEX_OK);
    assert(m.page == DEX_DETAIL && m.detail_id == 1025);
    dex_model_input(&m, DEX_DOWN);
    dex_model_input(&m, DEX_OK);
    assert(m.page == DEX_MOVES);
    dex_model_input(&m, DEX_OK);
    assert(m.page == DEX_MOVE_DETAIL);
    dex_model_input(&m, DEX_BACK);
    assert(m.page == DEX_MOVES);
    dex_model_input(&m, DEX_BACK);
    dex_model_input(&m, DEX_DOWN);
    dex_model_input(&m, DEX_OK);
    assert(dex_favorite(&m, 1025));
    dex_model_input(&m, DEX_BACK);
    assert(m.page == DEX_LIST && m.cursor == 1024);
    dex_model_input(&m, DEX_BACK);
    m.home_cursor = 4;
    dex_model_input(&m, DEX_OK);
    assert(m.count == 1 && m.results[0] == 1025);
    dex_model_input(&m, DEX_OK);
    m.detail_action = 2;
    dex_model_input(&m, DEX_OK);
    dex_model_input(&m, DEX_BACK);
    assert(m.count == 0 && m.cursor == 0);
    dex_model_input(&m, DEX_OK);
    assert(m.page == DEX_LIST);
    dex_model_input(&m, DEX_BACK);
    m.home_cursor = 1;
    dex_model_input(&m, DEX_OK); m.option = 1; dex_model_input(&m, DEX_OK);
    assert(m.count == 151);
    dex_model_input(&m, DEX_BACK); m.home_cursor = 2;
    dex_model_input(&m, DEX_OK); m.option = 12; dex_model_input(&m, DEX_OK);
    assert(m.count == 14);
    for (unsigned i = 0; i < m.count; ++i) assert(dex_matches(m.results[i], 1, 12, ""));
    dex_model_input(&m, DEX_BACK); m.home_cursor = 3;
    dex_model_input(&m, DEX_OK);
    assert(m.page == DEX_SEARCH && !m.type && !m.generation);
    m.character = 2; dex_model_input(&m, DEX_OK);
    m.character = 5; dex_model_input(&m, DEX_OK);
    m.character = 37; dex_model_input(&m, DEX_OK);
    assert(m.count == 1 && m.results[0] == 25);
    dex_model_input(&m, DEX_BACK);
    assert(m.page == DEX_SEARCH && !strcmp(m.query, "25"));
    m.character = 36; dex_model_input(&m, DEX_OK);
    assert(!strcmp(m.query, "2"));
    for (unsigned i = 0; i < 20000; ++i) dex_model_input(&m, (dex_input_t)(i % 6));
    assert(m.cursor < (m.count ? m.count : 1));
}

static void test_intro(void) {
    dex_model_t m;
    dex_model_init(&m);
    assert(!dex_model_boot_tick(&m, DEX_SPLASH_MS - 1));
    assert(m.page == DEX_SPLASH);
    assert(dex_model_boot_tick(&m, DEX_SPLASH_MS));
    assert(m.page == DEX_HOME && !m.storage_dirty);
    dex_model_input(&m, DEX_OK);
    assert(m.page == DEX_LIST);
    assert(!dex_model_boot_tick(&m, UINT32_MAX));
    assert(m.page == DEX_LIST);
    for (unsigned key = 0; key < 6; ++key) {
        dex_model_init(&m);
        dex_model_input(&m, (dex_input_t)key);
        assert(m.page == DEX_HOME && m.home_cursor == 0 && !m.storage_dirty);
    }
}

static void test_persistence(void) {
    dex_model_t a, b;
    dex_model_init(&a); dex_model_init(&b);
    a.last_id = 1025; a.favorites[0] = 1; a.favorites[128] = 1;
    uint8_t bytes[DEX_SAVE_BYTES];
    dex_save_encode(&a, bytes);
    assert(dex_save_decode(&b, bytes, sizeof(bytes)));
    assert(b.last_id == 1025 && dex_favorite(&b, 1) && dex_favorite(&b, 1025));
    assert(!dex_favorite(&b, 0) && !dex_favorite(&b, 1026));
    assert(!dex_save_decode(&b, bytes, sizeof(bytes) - 1));
    for (size_t i = 0; i < sizeof(bytes); ++i) {
        bytes[i] ^= 1;
        assert(!dex_save_decode(&b, bytes, sizeof(bytes)));
        bytes[i] ^= 1;
    }
    assert(!dex_save_decode(&b, NULL, sizeof(bytes)));
}

int main(void) {
    test_data(); test_search(); test_intro(); test_navigation(); test_persistence();
    puts("Pokédex model, versioned data, filters, navigation and CRC: PASS");
    return 0;
}
