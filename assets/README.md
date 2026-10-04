<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Assets

This directory stores reusable fonts, images, music, and sound effects, organized by asset type.

Keep each asset in the matching subdirectory and document its destination, naming, integration method, and source/license. Do not mix binary assets with Markdown documentation.

## Fonts

The Pocket Dex application embeds `fonts/dex_font_16.c` and `fonts/dex_font_20.c`.
They are Noto Sans CJK SC Regular subsets (16/20 px, uncompressed 2 bpp), generated
by `tools/generate_pokedex.py` using `lv_font_conv@1.5.3`. The inventory and source
hash are in `fonts/pokedex-glyphs.txt` and `fonts/font-source.json`; the source is licensed
under the accompanying `fonts/NotoSansCJK-LICENSE.txt`. See [reproduction and runtime
coverage checks](../docs/pokedex.md#reproduction-and-font-coverage).

Store reusable font files and generated font sources in `fonts/`.

- Use descriptive names that include the family, weight, size, and format when relevant.
- Document the source, license, character range, conversion command, and expected destination.
- Check Flash and internal-RAM impact before adding a font; the ESP32-C3 has no PSRAM.
- Do not commit fonts whose license does not permit redistribution.

## Images

Pocket Dex's `pokedex/pocket-dex-logo.png` is a transparent pixel-art title made
with the built-in image generation tool on 2026-10-04, with the four-character
Chinese title meaning Pocket Dex. Its Rotom Dex silhouette references the user's examples and the
[official Sun/Moon artwork](https://www.pokemon.co.jp/ex/sun_moon/collect/160602_01.html).
It is a generated fan-app logo, not an official Pokémon logo or a claim of affiliation.
Source dimensions and hash are in `pokedex/visual-source.json`.

The 18 SVGs in `pokedex/type-icons/` are from
[partywhale/pokemon-type-icons](https://github.com/partywhale/pokemon-type-icons)
at `fcbe6978c61c359680bc07636c3f9bdc0f346b43`, retaining its MIT license.
They recreate the game symbols; `source.json` preserves the palette sampled
from the user's type chart. Dark or white foregrounds improve small-screen contrast.
Pokémon designs remain the property of their respective rights holders.
`tools/generate_visual_assets.py` exports these assets as Flash-only RGB565A8
arrays; reproduction is documented in [Pocket Dex](../docs/pokedex.md#reproduction-and-font-coverage).

Store reusable source images and generated display assets in `images/`.

| File | Dimensions and format | Use and source |
| --- | --- | --- |
| [`images/home.jpg`](images/home.jpg) | 3840 × 2160, JPEG | Product hero image embedded in both project README files to foreground AI Passport and its open, maker-oriented identity. |
| [`images/readme-hardware-specs.png`](images/readme-hardware-specs.png) | 2172 × 724, PNG RGBA | Optional technical infographic retained as a reference asset; it is no longer used as the homepage hero. Generated for this repository with the built-in image generation tool on 2026-09-17; the six labels and values were checked against the documented hardware contract. |
| [`images/logo-wordmark.png`](images/logo-wordmark.png) | 1648 × 336, PNG RGBA | Transparent black wordmark extracted from the repository's original `images/logo.png`; embedded in both project README files for light backgrounds. |
| [`images/logo-wordmark-dark.png`](images/logo-wordmark-dark.png) | 1648 × 336, PNG RGBA | White version of the extracted wordmark, used by the README `<picture>` element when GitHub is in dark mode. |

- Use descriptive names and document dimensions, pixel format, conversion steps, and destination.
- Prefer formats suitable for the 240 × 320 RGB565 display and account for Flash and internal RAM.
- Preserve editable sources where licensing permits, and record the source and license.
- Never commit device QR secrets, credentials, or personal data in images.

## Music and sound effects

Pocket Dex embeds no audio. Its `pokedex/catalog.json` and `pokedex/sprites.bin`
contain the reviewable catalog and 1,025 compressed 48×48 RGB565 images. Their
provenance, game-version policy and third-party rights are documented in
[Pocket Dex](../docs/pokedex.md#data-and-provenance). These data/media are not
represented as original MIT-licensed artwork. The firmware reads them from Flash
and decodes only the current image.

Store reusable music and sound-effect sources in `music/`.

- Document the source, license, sample rate, bit depth, channels, conversion command, and destination.
- Prefer 16 kHz, 16-bit mono PCM when it matches the current BSP audio path.
- Check Flash and internal-RAM cost before embedding audio; stream or chunk long recordings.
- Do not commit media without redistribution permission.
