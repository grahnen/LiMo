#!/usr/bin/env python3
import sys
from subprocess import Popen, PIPE
import os

preamble = '''
\\documentclass{standalone}
\\usepackage{timeline}
\\begin{document}
'''

postamble = '''
\\end{document}
'''

name = "generated"
out_file = f"{name}.tex"
out_pdf = f"{name}.pdf"

if __name__ == "__main__":
    content = ""
    for line in sys.stdin:
        content += line

    with open(out_file, "w", encoding='utf-8') as f:
        f.write(preamble + content + postamble)



    po = Popen(["pdflatex", out_file], stdout=PIPE)
    po.wait()
    print(out_pdf)
