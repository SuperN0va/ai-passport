#include "dex_sprite.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    FILE *f = fopen("assets/pokedex/sprites.bin", "rb");
    assert(f);
    assert(fseek(f, 0, SEEK_END) == 0);
    long len = ftell(f);
    assert(len > 0 && fseek(f, 0, SEEK_SET) == 0);
    uint8_t *blob = malloc((size_t)len);
    assert(blob && fread(blob, 1, (size_t)len, f) == (size_t)len);
    fclose(f);
    uint16_t pixels[96 * 96];
    uint32_t hashes[1025];
    for (unsigned id = 1; id <= 1025; ++id) {
        assert(dex_sprite_decode(blob, (size_t)len, id, pixels));
        uint32_t hash = 2166136261u;
        for (unsigned y = 0; y < 96; y += 2) {
            for (unsigned x = 0; x < 96; x += 2) {
                unsigned p = y * 96 + x;
                assert(pixels[p] == pixels[p + 1] && pixels[p] == pixels[p + 96]);
                hash = (hash ^ pixels[p]) * 16777619u;
            }
        }
        hashes[id - 1] = hash;
        for (unsigned prev = 0; prev + 1 < id; ++prev) assert(hash != hashes[prev]);
    }
    assert(!dex_sprite_decode(blob, (size_t)len, 0, pixels));
    assert(!dex_sprite_decode(blob, (size_t)len, 1026, pixels));
    assert(!dex_sprite_decode(blob, 12, 1, pixels));
    uint8_t old = blob[8]; blob[8] = 255;
    assert(!dex_sprite_decode(blob, (size_t)len, 1, pixels));
    blob[8] = old;
    blob[0] = blob[1] = blob[2] = blob[3] = 255;
    assert(!dex_sprite_decode(blob, (size_t)len, 1, pixels));
    free(blob);
    puts("1025 sprites: bounded decode, nearest-neighbor scale, uniqueness: PASS");
    return 0;
}
