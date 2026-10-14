#!/usr/bin/env python3
"""Export the exact runtime-compiled shaders to .hlsl for Windows SDK fxc checks."""
from pathlib import Path
import argparse, re
def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[1])
    ap.add_argument('--output',type=Path,required=True)
    args=ap.parse_args()
    root=args.root
    source=(root/'lutdwm/dllmain.cpp').read_text(encoding='utf-8-sig')
    match=re.search(r'char shaders\[\]\s*=\s*R"\((.*?)\)";',source,re.S)
    if not match: raise ValueError('Embedded DWM shader not found')
    shader=match.group(1)
    # C++ concatenation of adjacent string literals around STRINGIFY(X).
    macros={'DITHER_GAMMA':'2.2', 'NOISE_SIZE':'64'}
    pattern=r'\)"\s*STRINGIFY\((\w+)\)\s*R"\('
    def sub(m):
        if m.group(1) not in macros: raise ValueError('Unknown shader macro: '+m.group(1))
        return macros[m.group(1)]
    shader=re.sub(pattern,sub,shader)
    if 'STRINGIFY' in shader:raise ValueError('Unexpanded macro in shader')
    histogram=(root/'lutdwm/AplHistogramEmbedded.hpp').read_text(encoding='utf-8')
    match2=re.search(r'R"APL_CS\((.*?)\)APL_CS"', histogram, re.S)
    if not match2:raise ValueError('Histogram shader not found')
    args.output.mkdir(parents=True,exist_ok=True)
    (args.output/'DwmDualLut.hlsl').write_text(shader,encoding='utf-8')
    (args.output/'AplHistogramCS.hlsl').write_text(match2.group(1),encoding='utf-8')
    assert 'Texture3D lut2Tex : register(t3)' in shader
    assert 'float4 PS(' in shader and 'VS_OUTPUT VS(' in shader
    assert 'RWStructuredBuffer<uint> histogram' in match2.group(1)
    print('Exported runtime shaders:',args.output)
if __name__=='__main__':main()
