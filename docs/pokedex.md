[简体中文](pokedex.zh_CN.md) · **English**

# Pocket Dex

An offline, Simplified Chinese national Pokédex for AI Passport. The application
starts with a Rotom-inspired pixel-art logo and then its own five-entry navigation screen. The hardware-test menu
and its visual shell are not linked into the application; the existing BSP is reused.

## Scope and controls

The startup screen displays the Chinese title for Pocket Dex for 2.4 seconds. Any button skips
to Home; the rest of that press is consumed so it does not also open a menu item.
The intro runs in the existing worker without blocking the button callbacks.
The red/cyan/navy theme uses rectangular handheld-style panels and high-contrast
selection states. All 18 types have individual symbols and colors in Pokémon
lists, details, descriptions, type filters, move lists and move parameters.

- National numbers 1–1025, one default form each, static pixel sprites.
- Names, types, first-appearance generation, height, weight, description and six base stats.
- Number lookup (including leading zeroes) and case-insensitive name-initial lookup.
- Generation and type filters can be combined. Either type of a dual-type species matches.
- Favorites and the last opened species are saved in the application's own NVS namespace.
- Move names, types, power, accuracy and base PP use the displayed game version.
- No audio, network connection, account, touch input or online service is required.

UP/DOWN select; hold to repeat. OK opens/accepts. Hold OK to return. The search
keyboard has digits, letters, delete and search actions. The first button gesture
after the display has gone dark only wakes it. Backlight dims after 30 seconds and
turns off after 90 seconds; USB and input remain available. This is not deep sleep,
and battery runtime has not been measured.

A fast OK double click is treated as one confirmation so it cannot accidentally
skip two pages. Search uses name initials, not a full Chinese input method.

Choose either filter from Home, return to Home, and choose the other to combine
them. All entries clears both. Favorites and search open with no generation/type
filter. Returning from details restores the list selection. Removing the last
favorite yields a deliberate empty state.

## Data and provenance

The reviewable input is `assets/pokedex/catalog.json`. It records source commits,
per-species description source/version, species generation and move generation
separately. Generated C tables reside in read-only Flash. The game-selection rule
is Scarlet/Violet first (733 species), then the latest recorded game from an
earlier generation (283 Gen 8, 9 Gen 7). Games inside a generation are not merged.

Sources:

- [PokeAPI CSV data](https://github.com/PokeAPI/pokeapi/tree/168b1e89467054cda2e7df43ccebbb69b459497a/data/v2/csv): base data, names, learnsets, move changelog and 722 descriptions.
- [Chinese encyclopedia export](https://github.com/42arch/pokemon-dataset-zh/tree/82ce04e611d19a12556c3955125b048b36187f52): 299 additional game descriptions, originally from [52Poké Wiki](https://wiki.52poke.com/). Form-specific text is selected explicitly; ambiguous combined cells fall back to an earlier unambiguous entry.
- [NightCatSama data](https://github.com/NightCatSama/pokedex): four remaining descriptions (#0417, #0496, #0651, #0652); their original game version is unknown and is shown as such. The cached file's SHA-256 is recorded.
- [Serebii PLA attacks](https://www.serebii.net/legendsarceus/attacks.shtml): normal-style PLA move parameters. PLA values do not inherit ordinary Gen 8/9 values. Agile/Strong Style is outside this release.
- [PKHeX personal table](https://github.com/kwsch/PKHeX): PLA move-shop permissions supplement 492 relationships absent from the PokeAPI snapshot. The exact commit is recorded in the catalog; no PKHeX executable or application code is embedded.
- Sprites and LodePNG were extracted from the reference fork's `feature/ble-artwork` at `6a049e2af6e560b7b92b2e5415a65a6d5a66c1aa`. Sprites are 48×48 RGB565 raw-DEFLATE (408,655 bytes); nearest-neighbor rendering is 96×96. LodePNG retains its upstream license notice.

The catalog contains 53,189 deduplicated relationships and 1,832 move/version
variants. Unknown/non-fixed numeric values display a dash, not zero or a blanket
claim that the move cannot miss. Base stats/types use the structured snapshot;
they do not change with the selected move-data generation. Data-source completeness
and every individual game entry have not been independently checked in-game.

Pokémon names, artwork and game text belong to their respective rights holders.
The export's code license is not a blanket license for third-party game material.
Wiki-origin text retains its applicable attribution/share-alike conditions. This
project records provenance instead of claiming original authorship.

## Reproduction and font coverage

Visual sources are retained under `assets/pokedex/`: `pocket-dex-logo.png` is the
selected generated transparent logo, and `type-icons/` contains pinned SVGs,
their license and the reference palette. `visual-source.json` records provenance
and the source PNG hash. The symbols are community recreations of game designs,
not an official asset distribution. See the [asset notes](../assets/README.md#images).

Install `@resvg/resvg-js@2.6.2` and Pillow 12.3.0, then run
`python tools/generate_visual_assets.py --resvg <path-to-@resvg/resvg-js>`
(optionally `--node <node>`). This exports 20/24 px type symbols and an
aspect-preserving logo of at most 208×180 px to RGB565A8 C arrays. The arrays
remain in Flash; no full-resolution source PNG is decoded on the ESP32.

`tools/import_pokedex.py --cache <cache>` imports local pinned CSV/JSON/HTML data;
it performs no network access and does not crawl the wiki. Required cache names
are documented by the script. Generation uses Python 3.12, pypinyin 0.55.0 and
beautifulsoup4 4.15.0. Keep source snapshots when regenerating.

Run `python tools/generate_pokedex.py` to generate C tables and the printable
character inventory. To regenerate fonts also supply `--font <NotoSansCJKsc-Regular.otf>`,
`--converter <lv_font_conv.js>` and optionally `--node <node>`.
The converter is pinned to `lv_font_conv@1.5.3`, with uncompressed 2-bpp fonts at
16 and 20 px. FontTools 4.66.1 checks the source cmap, including a known-missing
negative control. `assets/fonts/font-source.json` records source hash and options;
the SIL OFL text accompanies the generated fonts.

Every application text helper explicitly selects one of these fonts. Startup
checks every inventoried code point in both compiled fonts through LVGL's real
glyph lookup and rejects placeholders. Visual rendering on hardware remains a
separate acceptance check.

To render the actual UI without a board, obtain the pinned LVGL 9.5.0 sources,
then build `tests/preview` with CMake (`LVGL_SOURCE_DIR` points at that checkout,
`CMAKE_BUILD_TYPE=Debug` keeps assertions enabled). Run `pokedex_preview` with
`assets/pokedex/sprites.bin` and an existing output directory. It writes 240×320
PPM screenshots, checks the compiled fonts, and renders two 2,000-page cycles.
The second cycle must not consume additional memory; the smallest free block
must stay above 4 KiB. This is a host test, not an ESP32 performance measurement.

On Windows, activate ESP-IDF's `export.ps1`, put a native host C compiler and
Git Bash on PATH, then run `bash tools/validate.sh`. Its firmware phase delegates
to `tools/validate_firmware.ps1`, which uses native IDF Python/CMake/Ninja. Failed
scratch builds are retained for diagnosis; successful builds are archived before
their scratch directory is removed. Linux/macOS keep the original Bash path.

## Persistence, memory and layout

One worker owns application state, storage and page changes. Button callbacks
only enqueue events. Rendering holds the BSP LVGL lock. There are no per-page
timers or producers that survive a deleted page. Only the current sprite is
decoded; one 96×96 RGB565 buffer occupies 18 KiB. Lists render six visible rows.
The LVGL pool is 48 KiB. The search keyboard uses one styled label per key.
NVS uses a single versioned, CRC-checked record and does not erase other namespaces
on errors. A failed write remains dirty and is retried on the next interaction.

This derivative deliberately retains a 3 MiB app at `0x10000`, identity storage
at `0x356000` and factory Recovery at `0x700000`. All assets are embedded in the
app, so the merged image must end before `0x356000`. The legacy five-second UP
bootloader hook is retained. This is this application's choice, not a mandatory
partition contract for other applications based on the current upstream.

The verified merged image is flashable from `0x0`; a raw merged write may reset
NVS/PHY in its padded gaps. Segmented development flashing preserves these gaps.
Neither workflow writes identity/Recovery payloads. No full-chip erase is needed.
Only flash after confirming the target and exact validated image with the user.

## Validation status and device checklist

Build: PASS. Host tests: PASS. Device tests: PASS (user-reported basic use).
On 2026-10-04 the complete
`tools/validate.sh` gate passed with ESP-IDF 5.5.3, GCC 14.2.0 and the committed
dependency lock. The 1,785,264-byte app occupies 56.75% of its 3 MiB partition;
the 1,850,800-byte merged image ends at `0x1c3db0`, below device identity storage.
Its SHA-256 is `1c2ecf28aff6f0d66a007419d646ef0326126f3ffca73a91e2d94e582132b3ad`.
The matching ELF SHA-256 is `bd966a2f944d77e3d6fe0b45e073f3868a3127c183c4c624807926b11bfdcdac`.
The linked UP Recovery hook and preserved partition bounds passed additional checks.
The build's warning that this app exceeds the separate 1 MiB Recovery partition
is expected: this image is only assigned to `factory`, never to `recovery`.

All 1,025 sprite decoders, model/filter/navigation/CRC tests and the original BSP
regressions passed: 13 C programs and 111 Python tests, including startup timeout
and skip behavior. Both real fonts cover all 2,374 inventoried characters.
Actual LVGL rendering was inspected across 19 screenshots, including all 18 types,
dual types, the intro and empty/error states. A 2,000-page warmup retained 24 bytes
once; a further identical 2,000-page cycle retained no additional memory. The
48 KiB host pool's minimum free space was 14,904 bytes and minimum largest block
was 13,392 bytes. Static linked RAM/IRAM remains 139,662 bytes, unchanged from
the original UI release. These are host/linker figures; device heap remains unmeasured.

On 2026-10-04 the user confirmed testing the delivered Rotom UI revision and
approved synchronization to the GitHub fork. This follows their earlier successful
test of the original UI. These are user-reported results; no independent device
log, measurements or completed item-by-item acceptance sheet were collected.
The assistant's validation run did not open the serial port or flash the device.

The following detailed acceptance checks remain unverified individually: startup,
all Chinese pages and empty/error states,
quick presses/hold repeat/back, combined filters, both searches, #0001/#1025,
move-version labels and PLA parameters, favorite persistence across power cycles,
idle dim/wake, battery indication, image colors, and repeated navigation without
heap loss. Record runtime free heap/largest block, input/search/image latency and
the exact firmware hash separately from host/build results.
