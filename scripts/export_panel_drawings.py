"""Build standalone vector panel artwork from the same TikZ used by the manuals."""

import argparse
from pathlib import Path
import shutil
import subprocess
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--module', choices=('fourier', 'spectre', 'all'), default='all')
    parser.add_argument('--output', type=Path, default=ROOT / '.build/manual-figures')
    parser.add_argument('--pdflatex', default='pdflatex')
    parser.add_argument('--pdftocairo', default='pdftocairo')
    args = parser.parse_args()
    for program in (args.pdflatex, args.pdftocairo):
        if shutil.which(program) is None:
            parser.error(f'{program} is unavailable; install TeX/Poppler or supply its path')
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    modules = ('fourier', 'spectre') if args.module == 'all' else (args.module,)
    for module in modules:
        for variant in ('guide', 'panel', 'guide-mono', 'panel-mono'):
            stem = f'{module}-{variant}'
            switches = ''
            if variant.startswith('panel'):
                switches += r'\def\PanelBare{1}' + '\n'
            if variant.endswith('mono'):
                switches += r'\def\PanelMonochrome{1}' + '\n'
            tex = (r'\documentclass[border=8pt]{standalone}' + '\n'
                   + r'\usepackage[T1]{fontenc}' + '\n'
                   + r'\usepackage[scaled]{helvet}' + '\n'
                   + r'\usepackage{tikz}' + '\n' + switches
                   + r'\input{docs/figures/PanelDrawing.tex}' + '\n'
                   + r'\begin{document}' + '\n'
                   + f'\\input{{docs/manual-{module}/img/PanelLayout.tex}}\n'
                   + r'\end{document}' + '\n')
            source = output / f'{stem}.tex'
            source.write_text(tex)
            command = [args.pdflatex, '-halt-on-error', '-interaction=nonstopmode',
                       '-file-line-error', '-no-shell-escape',
                       f'-output-directory={output}', str(source)]
            result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
            (output / f'{stem}-build.log').write_text(result.stdout + result.stderr)
            if result.returncode:
                parser.exit(1, f'{stem} failed; see {output / (stem + "-build.log")}\n')
            pdf = output / f'{stem}.pdf'
            svg = output / f'{stem}.svg'
            subprocess.run([args.pdftocairo, '-svg', str(pdf), str(svg)], check=True)
            # A panel illustration must remain vector-only in web/print exports.
            tree = ET.parse(svg)
            root = tree.getroot()
            namespace = 'http://www.w3.org/2000/svg'
            if root.findall(f'.//{{{namespace}}}image'):
                parser.exit(1, f'Unexpected raster image in {svg}\n')
            ET.register_namespace('', namespace)
            ET.register_namespace('xlink', 'http://www.w3.org/1999/xlink')
            title = ET.Element(f'{{{namespace}}}title', id=f'{stem}-title')
            title.text = f'{module.capitalize()} panel reference'
            description = ET.Element(f'{{{namespace}}}desc', id=f'{stem}-description')
            description.text = (
                'Vector line drawing of the module controls and schematic display. '
                + ('Numbered callouts correspond to the included control key.'
                   if variant.startswith('guide') else 'Unannotated panel illustration.'))
            root.insert(0, title)
            root.insert(1, description)
            root.set('role', 'img')
            root.set('aria-labelledby', f'{stem}-title {stem}-description')
            tree.write(svg, encoding='utf-8', xml_declaration=True)
            print(f'{pdf}\n{svg}')


if __name__ == '__main__':
    main()
