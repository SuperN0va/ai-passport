<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# 资源目录（Assets）

本目录集中存放可复用的资源（字库、图片、音乐等），按资源类型分子目录管理。每个资源放在其类型对应的子目录，并记录放置路径、命名方式、集成方式与来源/许可。二进制资源（字体、图片、音频）不属于纯 markdown 文档，请勿与文档混放。涉及版权/授权的资源需注明来源与许可。

## 字库（fonts）

口袋图鉴内嵌 `fonts/dex_font_16.c` 和 `fonts/dex_font_20.c`，使用 Noto Sans CJK SC Regular 的 16/20 px、未压缩 2 bpp 子集，由 `tools/generate_pokedex.py` 和 `lv_font_conv@1.5.3` 生成。字符清单、来源哈希分别记录在 `fonts/pokedex-glyphs.txt`、`fonts/font-source.json`，SIL OFL 许可在 `fonts/NotoSansCJK-LICENSE.txt`。复现方式和实际 LVGL 字形检查见[口袋图鉴说明](../docs/pokedex.zh_CN.md)。

可复用的字库文件与生成的字库源码放在 `fonts/`。

- 命名要能反映字族、字重、字级与格式。
- 记录来源、许可、字符范围、转换命令与目标放置路径。
- 添加字库前评估 Flash 与内部 RAM 影响；ESP32-C3 无 PSRAM。
- 不提交许可不允许分发的字库。

## 图片（images）

`pokedex/pocket-dex-logo.png` 是 2026-10-04 使用内置图像生成工具制作的透明像素
标题，文字为“口袋图鉴”。洛托姆图鉴轮廓参考用户提供的示例与
[《太阳／月亮》官方图](https://www.pokemon.co.jp/ex/sun_moon/collect/160602_01.html)。
这是同人应用的生成 Logo，不是官方宝可梦标志，也不代表官方关联。
源图尺寸与哈希记录在 `pokedex/visual-source.json`。

`pokedex/type-icons/` 的 18 个 SVG 来自
[partywhale/pokemon-type-icons](https://github.com/partywhale/pokemon-type-icons)，
固定提交为 `fcbe6978c61c359680bc07636c3f9bdc0f346b43`，保留其 MIT 许可。
它们复现游戏中的属性符号；`source.json` 保存从用户属性参考图采样的色板。
深色或白色前景用于改善小屏对比度，宝可梦设计仍属于各自权利人。
`tools/generate_visual_assets.py` 导出常驻 Flash 的 RGB565A8 数组，复现方法见
[应用说明](../docs/pokedex.zh_CN.md#数据再生成与字体检查)。

口袋图鉴的 `pokedex/catalog.json` 是可审阅的数据集，`pokedex/sprites.bin` 包含 1025 张 48×48 RGB565 压缩图片，固件从 Flash 读取并只解码当前图片。[应用文档](../docs/pokedex.zh_CN.md)记录来源、游戏版本规则及第三方权利；这些游戏数据与图片不被声明为原创 MIT 素材。应用不内嵌任何音频。

可复用的源图与生成的显示资产放在 `images/`。

| 文件 | 尺寸与格式 | 用途与来源 |
| --- | --- | --- |
| [`images/home.jpg`](images/home.jpg) | 3840 × 2160，JPEG | 嵌入中英文项目 README 的产品主图，突出 AI Passport 产品形象与开放、人人可创作的理念。 |
| [`images/readme-hardware-specs.png`](images/readme-hardware-specs.png) | 2172 × 724，PNG RGBA | 保留为可选技术参考图，不再用于首页主视觉。于 2026-09-17 使用内置图像生成工具为本仓库生成；已根据文档中的硬件能力契约核对图中的六项标签与参数。 |
| [`images/logo-wordmark.png`](images/logo-wordmark.png) | 1648 × 336，PNG RGBA | 从仓库原始 `images/logo.png` 中精确裁切并去除背景的黑色字标；用于中英文项目 README 的浅色主题。 |
| [`images/logo-wordmark-dark.png`](images/logo-wordmark-dark.png) | 1648 × 336，PNG RGBA | 提取字标的白色版本；README 使用 `<picture>` 在 GitHub 深色主题下显示。 |

- 使用描述性命名，并记录尺寸、像素格式、转换步骤与目标路径。
- 优先采用适合 240 × 320 RGB565 显示的格式，并纳入 Flash 与内部 RAM 考量。
- 许可允许时保留可编辑源文件，并记录来源与许可。
- 图片中不得包含设备二维码秘密、凭证或个人数据。

## 音乐与音效（music）

可复用的音乐与音效源码放在 `music/`。

- 记录来源、许可、采样率、位深、声道、转换命令与目标路径。
- 与当前 BSP 音频路径匹配时优先采用 16 kHz、16 位单声道 PCM。
- 嵌入音频前评估 Flash 与内部 RAM 成本；长录音应流式或分块。
- 无再分发许可不提交媒体文件。
