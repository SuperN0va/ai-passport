#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DEX_SPRITE_SIDE 96
/* Output lifetime belongs to caller; raw RGB565, no byte swapping. */
bool dex_sprite_decode(const uint8_t *blob, size_t length, unsigned id,
                       uint16_t pixels[DEX_SPRITE_SIDE * DEX_SPRITE_SIDE]);
