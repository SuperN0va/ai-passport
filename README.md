[简体中文](README.zh_CN.md) · **English**

# Pocket Dex for AI Passport

An offline Chinese Pokédex for the ESP32-C3 AI Passport with 8 MB Flash and no
PSRAM. Browse all 1,025 species, read Chinese descriptions, search by number or
name initials, save favorites and explore versioned move lists using three buttons.

![Actual LVGL UI preview](docs/assets/pokedex-ui.png)

This feature branch contains the application. It reuses the
[FoloToy AI Passport](docs/README.md) BSP and supplies its own interface: a
Rotom-inspired pixel-art startup logo, red/cyan/navy colors and all 18 type symbols.
The preview above is rendered by LVGL, not photographed from a device.

## Features

- National numbers 0001–1025, one default form and static pixel sprite each.
- Chinese names and descriptions, types, first-appearance generation, height,
  weight and six base stats.
- Number order, combined generation/type filtering, number or name-initial search.
- Persistent favorites and last viewed species.
- Learnable move names, types, power, accuracy and base PP. Each species uses
  one selected game version, displayed separately from its debut generation.
- Fully offline; no cries, Wi-Fi, account or cloud service required.

Move data prefers Gen 9 Scarlet/Violet for 733 species; the others use Gen 8
(283 species) or Gen 7 (9 species), retaining exact game labels without merging
versions. There are 53,189 deduplicated relationships and 1,832 move/version variants.

## Controls

| Input | Action |
| --- | --- |
| Any button during the 2.4-second intro | Skip to Home without also selecting an item |
| UP / DOWN | Select or scroll; hold to repeat |
| Short OK | Open or confirm |
| Hold OK | Return |
| First gesture after the backlight turns off | Wake only |

Enter `25`, `0025` or `PKQ` to find Pikachu. Search uses Chinese-name initials,
not a full Chinese keyboard. Choose generation and type from Home to combine
filters. All entries clears filters; search and favorites operate independently.
Add or remove favorites on a species detail page. Backlight dims after 30 seconds
and turns off after 90 seconds of inactivity; this is not deep sleep.

## Build and flashing

Activate ESP-IDF 5.5.3, clone this branch, and run:

```sh
./tools/validate.sh
```

On Windows, activate `export.ps1` and run `bash tools/validate.sh` with Git Bash
and a native host C compiler available. Generated data, fonts and icons are
included; image-generation tools are not needed for a normal firmware build.

The verified merged output is `build/FoloToy-AI-Passport-full.bin`, flashed at
`0x0`. Matching ELF/MAP and segmented images are archived under
`build/firmware/<full-bin-sha256>/`. A merged write can reset favorites stored in
NVS; compatible segmented flashing preserves the padded gaps. Do not erase the
whole chip. This application retains the provisioned identity and Recovery areas
and the five-second UP bootloader entry.

## Validation and data notes

Build and host tests passed: 13 C programs, 111 Python tests, both Chinese fonts
covering 2,374 characters, and 4,000 actual LVGL page changes. The delivered app
uses 56.75% of its 3 MiB partition. On 2026-10-04 the user confirmed testing the
Rotom UI revision on their device; this is user-reported basic use, with detailed
measurements and individual acceptance checks still unverified.

See [application documentation](docs/pokedex.md) for exact firmware/ELF hashes,
memory figures, data sources, reproduction and remaining checks. Four Chinese
description source versions are unknown and explicitly labeled. Game data has
not been individually verified against every original game.

Pokémon artwork, names and game text retain their respective rights; the generated
fan-app logo is not an official mark. Third-party notices and source metadata are
retained in [assets](assets/README.md). The base project code uses the
[MIT license](LICENSE); that does not relicense third-party game material.
