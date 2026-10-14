from pathlib import Path
import xml.etree.ElementTree as ET
root=Path(__file__).resolve().parents[1]
for name in ["lutdwm/lutdwm.vcxproj","DwmLutGUI/DwmLutGUI/MainWindow.xaml", "DwmLutGUI/DwmLutGUI/DwmLutGUI.csproj"]:
    ET.parse(root/name)
cpp=(root/"lutdwm/dllmain.cpp").read_text()
assert 'Texture3D lut2Tex : register(t3);' in cpp
assert 'if (dualEnabled)' in cpp
assert 'g_aplProbe.OnFrame(g_aplInputSrv)' in cpp
assert 'lut2->textureView' in cpp
assert 'GetApl2LutData' in cpp
assert 'candidate->size==base->size' in cpp
assert 'pq_eotf(saturate(hdr10_res2))' in cpp
inj=(root/"DwmLutGUI/DwmLutGUI/Injector.cs").read_text(encoding="utf-8-sig")
assert '_hdr_apl2.cube' in inj
assert 'monitor.HdrApl2LutPath' in inj
model=(root/"DwmLutGUI/DwmLutGUI/MainViewModel.cs").read_text(encoding="utf-8-sig")
assert 'hdr_apl2_lut' in model
print("PASS: native dual LUT wiring, optional GUI profile, XML parse")
