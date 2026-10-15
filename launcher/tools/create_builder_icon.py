"""Draw the original, code-native minimal tyre mark for the builder only."""
from pathlib import Path
import math
from PIL import Image, ImageDraw

assets = Path(__file__).resolve().parents[1] / 'ForzaMastro.Launcher/Assets'
scale = 4
image = Image.new('RGBA', (256 * scale, 256 * scale))
draw = ImageDraw.Draw(image)

def box(values):
    return tuple(round(v * scale) for v in values)

draw.rounded_rectangle(box((4, 4, 252, 252)), radius=54 * scale, fill='#191B1F')
draw.ellipse(box((40, 40, 216, 216)), fill='#FF8A44')
draw.ellipse(box((80, 80, 176, 176)), fill='#191B1F')
svg_paths = []
for index in range(12):
    angle = index * math.tau / 12
    points = []
    for radius, offset in [(90, -.035), (90, .035), (67, .17), (67, .10)]:
        points.append((128 + radius * math.cos(angle + offset),
                       128 + radius * math.sin(angle + offset)))
    draw.polygon([(round(x * scale), round(y * scale)) for x, y in points], fill='#191B1F')
    svg_paths.append('<polygon points="' + ' '.join(f'{x:.3f},{y:.3f}' for x, y in points) + '" fill="#191B1F"/>')
image = image.resize((256, 256), Image.Resampling.LANCZOS)
image.save(assets / 'Builder.png')
image.save(assets / 'Builder.ico', sizes=[(n, n) for n in [16, 24, 32, 48, 64, 128, 256]])
(assets / 'Builder.svg').write_text(
    '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 256 256">\n'
    '<rect x="4" y="4" width="248" height="248" rx="54" fill="#191B1F"/>\n'
    '<circle cx="128" cy="128" r="88" fill="#FF8A44"/>\n'
    '<circle cx="128" cy="128" r="48" fill="#191B1F"/>\n' +
    '\n'.join(svg_paths) + '\n</svg>\n', encoding='utf-8')
print('Created original builder tyre icon: SVG, PNG and seven-size ICO.')
