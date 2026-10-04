#include "dex_sprite.h"
#include "dex_data.h"
#include "vendor/lodepng.h"
#include <stdlib.h>

static uint32_t u32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

bool dex_sprite_decode(const uint8_t *blob, size_t length, unsigned id,
                       uint16_t pixels[DEX_SPRITE_SIDE * DEX_SPRITE_SIDE]) {
    const size_t toc = DEX_COUNT * 12u;
    if (!blob || !pixels || id < 1 || id > DEX_COUNT || length < toc) return false;
    const uint8_t *entry = blob + (id - 1) * 12;
    uint32_t off = u32(entry), len = u32(entry + 4);
    if (entry[8] != 48 || entry[9] || entry[10] != 48 || entry[11] ||
        off > length - toc || len > length - toc - off || len == 0) return false;
    unsigned char *raw = NULL;
    size_t size = 0;
    LodePNGDecompressSettings settings;
    lodepng_decompress_settings_init(&settings);
    settings.max_output_size = 48 * 48 * 2;
    unsigned error = lodepng_inflate(&raw, &size, blob + toc + off, len, &settings);
    if (error || size != 48 * 48 * 2) { free(raw); return false; }
    for (unsigned y = 0; y < DEX_SPRITE_SIDE; ++y) {
        for (unsigned x = 0; x < DEX_SPRITE_SIDE; ++x) {
            unsigned p = ((y / 2) * 48 + x / 2) * 2;
            pixels[y * DEX_SPRITE_SIDE + x] = (uint16_t)(raw[p] | ((uint16_t)raw[p + 1] << 8));
        }
    }
    free(raw);
    return true;
}
