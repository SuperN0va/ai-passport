#include "dex_model.h"
#include <stdlib.h>
#include <string.h>

const dex_entry_t *dex_entry(unsigned id) {
    return id >= 1 && id <= DEX_COUNT ? &dex_entries[id] : NULL;
}

const dex_move_t *dex_move(unsigned species, unsigned index) {
    const dex_entry_t *e = dex_entry(species);
    if (!e || index >= e->move_count) return NULL;
    unsigned n = dex_move_indices[e->move_offset + index];
    return n < dex_move_variant_count ? &dex_moves[n] : NULL;
}

bool dex_favorite(const dex_model_t *m, unsigned id) {
    return id >= 1 && id <= DEX_COUNT &&
           (m->favorites[(id - 1) / 8] & (1u << ((id - 1) % 8))) != 0;
}

bool dex_matches(unsigned id, unsigned gen, unsigned type, const char *query) {
    const dex_entry_t *e = dex_entry(id);
    if (!e || (gen && e->gen != gen) || (type && e->type1 != type && e->type2 != type)) return false;
    if (!query || !*query) return true;
    char q[DEX_QUERY_MAX + 1];
    size_t n = strlen(query);
    if (n > DEX_QUERY_MAX) return false;
    bool numeric = true;
    for (size_t i = 0; i < n; ++i) {
        char c = query[i];
        q[i] = c >= 'a' && c <= 'z' ? (char)(c - 'a' + 'A') : c;
        if (c < '0' || c > '9') numeric = false;
    }
    q[n] = 0;
    if (numeric) return strtoul(q, NULL, 10) == id;
    return strstr(e->initials, q) != NULL;
}

void dex_refresh_results(dex_model_t *m, unsigned preferred_id) {
    unsigned old = m->cursor;
    m->count = 0;
    for (unsigned id = 1; id <= DEX_COUNT; ++id) {
        if (dex_matches(id, m->generation, m->type, m->query) &&
            (!m->favorites_only || dex_favorite(m, id))) m->results[m->count++] = (uint16_t)id;
    }
    m->cursor = m->count ? (uint16_t)(old < m->count ? old : (unsigned)m->count - 1u) : 0;
    for (unsigned i = 0; i < m->count; ++i) {
        if (m->results[i] == preferred_id) { m->cursor = (uint16_t)i; break; }
    }
}

void dex_model_init(dex_model_t *m) {
    memset(m, 0, sizeof(*m));
    m->page = DEX_SPLASH;
    m->last_id = 1;
    dex_refresh_results(m, 1);
}

bool dex_model_boot_tick(dex_model_t *m, uint32_t elapsed_ms) {
    if (m->page != DEX_SPLASH || elapsed_ms < DEX_SPLASH_MS) return false;
    m->page = DEX_HOME;
    return true;
}

static unsigned cycle(unsigned value, unsigned count, int delta) {
    if (!count) return 0;
    int next = (int)value + delta;
    if (next < 0) return count - 1;
    if ((unsigned)next >= count) return 0;
    return (unsigned)next;
}

static void list_open(dex_model_t *m) {
    m->page = DEX_LIST;
    dex_refresh_results(m, m->last_id);
}

void dex_model_input(dex_model_t *m, dex_input_t input) {
    if (m->page == DEX_SPLASH) { m->page = DEX_HOME; return; }
    int delta = (input == DEX_UP || input == DEX_PAGE_UP) ? -1 : 1;
    bool move = input == DEX_UP || input == DEX_DOWN || input == DEX_PAGE_UP || input == DEX_PAGE_DOWN;
    if (input == DEX_BACK) {
        switch (m->page) {
        case DEX_INFO: case DEX_MOVES: m->page = DEX_DETAIL; break;
        case DEX_MOVE_DETAIL: m->page = DEX_MOVES; break;
        case DEX_DETAIL:
            m->page = DEX_LIST;
            dex_refresh_results(m, m->detail_id);
            break;
        case DEX_LIST: m->page = m->from_search ? DEX_SEARCH : DEX_HOME; break;
        default: m->page = DEX_HOME; break;
        }
        return;
    }
    switch (m->page) {
    case DEX_SPLASH: break; /* Consumed above; never navigate through the intro. */
    case DEX_HOME:
        if (move) m->home_cursor = (uint8_t)cycle(m->home_cursor, 5, delta);
        if (input != DEX_OK) break;
        m->favorites_only = false;
        m->from_search = false;
        m->query[0] = 0;
        switch (m->home_cursor) {
        case 0: m->generation = m->type = 0; list_open(m); break;
        case 1: m->page = DEX_GENERATION; m->option = m->generation; break;
        case 2: m->page = DEX_TYPE; m->option = m->type; break;
        case 3:
            m->generation = m->type = 0;
            m->page = DEX_SEARCH; m->character = 0; break;
        case 4:
            m->generation = m->type = 0;
            m->favorites_only = true; list_open(m); break;
        }
        break;
    case DEX_GENERATION: case DEX_TYPE:
        if (move) m->option = (uint8_t)cycle(m->option, m->page == DEX_GENERATION ? 10 : 19, delta);
        if (input == DEX_OK) {
            if (m->page == DEX_GENERATION) m->generation = m->option;
            else m->type = m->option;
            list_open(m);
        }
        break;
    case DEX_SEARCH:
        if (move) m->character = (uint8_t)cycle(m->character, 38, delta);
        if (input == DEX_OK) {
            size_t len = strlen(m->query);
            if (m->character == 37) { m->from_search = true; list_open(m); }
            else if (m->character == 36) { if (len) m->query[len - 1] = 0; }
            else if (len < DEX_QUERY_MAX) {
                m->query[len] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[m->character];
                m->query[len + 1] = 0;
            }
        }
        break;
    case DEX_LIST:
        if (move) {
            if (input == DEX_PAGE_UP || input == DEX_PAGE_DOWN) delta *= 6;
            m->cursor = (uint16_t)cycle(m->cursor, m->count, delta);
        }
        if (input == DEX_OK && m->count) {
            m->detail_id = m->results[m->cursor];
            m->last_id = m->detail_id;
            m->storage_dirty = true;
            m->detail_action = 0;
            m->page = DEX_DETAIL;
        }
        break;
    case DEX_DETAIL:
        if (move) m->detail_action = (uint8_t)cycle(m->detail_action, 3, delta);
        if (input == DEX_OK) {
            if (m->detail_action == 0) { m->page = DEX_INFO; m->scroll = 0; }
            else if (m->detail_action == 1) { m->page = DEX_MOVES; m->move_cursor = 0; }
            else if (dex_entry(m->detail_id)) {
                unsigned bit = m->detail_id - 1;
                m->favorites[bit / 8] ^= (uint8_t)(1u << (bit % 8));
                m->storage_dirty = true;
            }
        }
        break;
    case DEX_INFO:
        if (move) {
            int next = (int)m->scroll + delta * 36;
            m->scroll = (uint16_t)(next < 0 ? 0 : next > 600 ? 600 : next);
        }
        break;
    case DEX_MOVES: {
        const dex_entry_t *e = dex_entry(m->detail_id);
        if (!e) break;
        if (move) {
            if (input == DEX_PAGE_UP || input == DEX_PAGE_DOWN) delta *= 6;
            m->move_cursor = (uint16_t)cycle(m->move_cursor, e->move_count, delta);
        }
        if (input == DEX_OK && e->move_count) m->page = DEX_MOVE_DETAIL;
        break;
    }
    case DEX_MOVE_DETAIL: break;
    }
}

static uint32_t crc32(const uint8_t *p, size_t len) {
    uint32_t crc = 0xffffffff;
    for (size_t i = 0; i < len; ++i) {
        crc ^= p[i];
        for (int bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1)));
    }
    return ~crc;
}

void dex_save_encode(const dex_model_t *m, uint8_t out[DEX_SAVE_BYTES]) {
    memset(out, 0, DEX_SAVE_BYTES);
    memcpy(out, "PDX1", 4);
    out[4] = (uint8_t)m->last_id;
    out[5] = (uint8_t)(m->last_id >> 8);
    memcpy(out + 8, m->favorites, DEX_FAVORITE_BYTES);
    uint32_t crc = crc32(out, DEX_SAVE_BYTES - 4);
    for (unsigned i = 0; i < 4; ++i) out[DEX_SAVE_BYTES - 4 + i] = (uint8_t)(crc >> (8 * i));
}

bool dex_save_decode(dex_model_t *m, const uint8_t *data, size_t size) {
    if (!data || size != DEX_SAVE_BYTES || memcmp(data, "PDX1", 4)) return false;
    uint32_t expected = 0;
    for (unsigned i = 0; i < 4; ++i) expected |= (uint32_t)data[size - 4 + i] << (8 * i);
    unsigned id = data[4] | ((unsigned)data[5] << 8);
    if (!dex_entry(id) || expected != crc32(data, size - 4)) return false;
    m->last_id = (uint16_t)id;
    memcpy(m->favorites, data + 8, DEX_FAVORITE_BYTES);
    m->favorites[DEX_FAVORITE_BYTES - 1] &= 1; /* Only #1025 uses the last byte. */
    dex_refresh_results(m, id);
    return true;
}
