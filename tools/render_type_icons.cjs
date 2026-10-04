/* Rasterize editable SVG sources using the pinned resvg 2.6.2 renderer. */
const fs = require('fs');
const path = require('path');
const { Resvg } = require(process.argv[2]);
const root = path.resolve(__dirname, '..');
const source = path.join(root, 'assets/pokedex/type-icons');
const output = process.argv[3];
const metadata = JSON.parse(fs.readFileSync(path.join(source, 'source.json')));
function luminance(hex) {
    const rgb = hex.slice(1).match(/../g).map(x => parseInt(x, 16) / 255)
        .map(x => x <= 0.04045 ? x / 12.92 : ((x + 0.055) / 1.055) ** 2.4);
    return rgb[0] * 0.2126 + rgb[1] * 0.7152 + rgb[2] * 0.0722;
}
const foregrounds = {};
for (const [type, color] of Object.entries(metadata.colors)) {
    const lum = luminance(color);
    const darkContrast = (lum + 0.05) / (luminance('#122a42') + 0.05);
    const lightContrast = 1.05 / (lum + 0.05);
    const foreground = darkContrast >= lightContrast ? '#122a42' : '#ffffff';
    foregrounds[type] = foreground;
    let svg = fs.readFileSync(path.join(source, type + '.svg'), 'utf8');
    svg = svg.replace(/fill:\s*(#[a-fA-F0-9]+);/g, (_, hex) => {
        if (hex.length === 4) hex = '#' + [...hex.slice(1)].map(c => c + c).join('');
        return `fill: ${luminance(hex) > 0.9 ? foreground : color};`;
    });
    for (const size of [20, 24]) {
        const png = new Resvg(svg, { fitTo: { mode: 'width', value: size } }).render().asPng();
        fs.writeFileSync(path.join(output, `${type}-${size}.png`), png);
    }
}
fs.writeFileSync(path.join(output, 'foregrounds.json'), JSON.stringify(foregrounds));
