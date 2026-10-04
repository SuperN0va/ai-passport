#pragma once
#include <stddef.h>
#include <stdint.h>

#define DEX_COUNT 1025

typedef struct {
    const char *name;
    const char *initials;
    const char *description;
    const char *description_version;
    uint32_t move_offset;
    uint16_t move_count;
    uint16_t height;
    uint16_t weight;
    uint16_t stats[6];
    uint8_t gen;
    uint8_t type1;
    uint8_t type2;
    uint8_t move_gen;
    uint8_t move_group;
} dex_entry_t;

typedef struct {
    const char *name;
    uint16_t id;
    int16_t power;
    int16_t accuracy;
    int16_t pp;
    uint8_t type;
    uint8_t group;
} dex_move_t;

extern const dex_entry_t dex_entries[DEX_COUNT + 1];
extern const dex_move_t dex_moves[];
extern const size_t dex_move_variant_count;
extern const uint16_t dex_move_indices[];
extern const char *const dex_types[19];
extern const char *const dex_groups[26];

const dex_entry_t *dex_entry(unsigned id);
const dex_move_t *dex_move(unsigned species, unsigned index);
