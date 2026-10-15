"""Render the builder's Segoe UI Black Italic numbers as game EXE icons."""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from project_paths import add_python_libraries
add_python_libraries(__file__)
from PIL import Image, ImageDraw, ImageFont


def main():
    assets = ROOT / 'launcher/ForzaMastro.Launcher/Assets'
    scale = 4
    font = ImageFont.truetype(r'C:\Windows\Fonts\seguibli.ttf', 190 * scale)
    for number, accent in [('5', '#FF8A44')]:
        image = Image.new('RGBA', (256 * scale, 256 * scale))
        draw = ImageDraw.Draw(image)
        draw.rounded_rectangle((4*scale, 4*scale, 252*scale, 252*scale),
                               radius=54*scale, fill='#191B1F')
        draw.rounded_rectangle((5*scale, 5*scale, 251*scale, 251*scale),
                               radius=53*scale, outline='#343842', width=2*scale)
        left, top, right, bottom = draw.textbbox((0, 0), number, font=font)
        draw.text(((256*scale-right-left)/2, (256*scale-bottom-top)/2),
                  number, font=font, fill=accent)
        image = image.resize((256, 256), Image.Resampling.LANCZOS)
        base = assets / ('ForzaMotorsport' + number)
        image.save(base.with_suffix('.png'))
        image.save(base.with_suffix('.ico'), sizes=[(n,n) for n in (16,24,32,48,64,128,256)])
        print(base.with_suffix('.ico'))


if __name__ == '__main__':
    main()
