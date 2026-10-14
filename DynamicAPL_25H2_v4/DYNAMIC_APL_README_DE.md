# Dynamic APL – Windows 11 25H2 (EXPERIMENTELLER QUELLCODE)

Dieses Projekt ist ein **direkt geänderter Fork** des hochgeladenen Quellcodes
`dwm_lut-windows-25h2-main` (ursprünglich 0x401gg / ledoge).

**Keine fertige DLL:** Windows-DLL und GUI wurden in dieser Linux-Umgebung **nicht** kompiliert,
HLSL wurde nicht durch den DirectX-Compiler validiert, und ein Test in DWM ist nicht erfolgt.
Die drei portablen C++-Tests des APL-Prototyps wurden bestanden. Der Patch ist daher
EIN EXPERIMENT und sollte NICHT ungeprüft auf einem Produktivsystem installiert werden.

## Inhalt
- `lutdwm/dllmain.cpp`: Zwei HDR-LUTs im vorhandenen DWM-Pixelshader (t1 und t3),
  weiche Überblendung **im linearen HDR-Licht**. Shadergrößen müssen übereinstimmen.
- `lutdwm/D3D11AplProbe.cpp` und Header: optionaler GPU-Histogramm-Compute-Pass
  für pre-LUT scRGB, asynchroner Staging-Readback (kein blockierendes Wait).
- `DwmLutGUI/`: Neues Feld **2% APL HDR LUT (dynamic, optional)**, in `config.xml`
  als `hdr_apl2_lut` gespeichert. `Injector` exportiert `_hdr_apl2.cube` neben der
  vorhandenen `_hdr.cube`, bevor die Injektion erfolgt.
- `profiles/`: unabhängige 2%- und 10%-APL-Testprofile aus den CalMAN-Daten
  **ohne vorher aktive LUT**. Sie sind **nicht gegen eine Nachmessung validiert**.
- `tests/` im separaten `portable_tests`-Ordner des Pakets: C++-Referenztests.

## Funktionsablauf
1. Benutzer wählt die 10%-APL-HDR-LUT und daneben optional die 2%-APL-LUT.
2. Wenn kein 2%-Profil angegeben ist, verhält sich die DLL wie das Original.
3. Bei passender zweiter LUT wird alle ca. 250 ms ein Histogramm der **vorherigen,
   unkorrigierten HDR-Ausgabe** erzeugt. CPU-Auslesen erfolgt per Event-Query.
4. Ein geglättetes Mischgewicht (0 = 10%-Profil, 1 = 2%-Profil) steuert die beiden LUTs.
   In dunklen/uneindeutigen Szenen bleibt das 10%-Profil als Rückfall erhalten.
5. Das Mischgewicht beschreibt **Bildstatistik, nicht tatsächliche Mini-LED-Zonen-/APL-Steuerung**.
   Der Monitor kann abhängig von Local Dimming, ABL, Fenstergröße und HDR-Metadaten
   anders reagieren. 2% und 10% können deshalb nicht exakt gleichzeitig garantiert werden.

## Grenzen / technische Risiken
- In dieser Version wird aus Sicherheitsgründen nur **ein Monitor mit dualem APL-Profil** unterstützt.
  Wenn mehrere Monitore 2%-LUTs konfigurieren, gilt auf allen der 10%-Fallback.
- Das Histogramm untersucht den vom DWM-Hook gelieferten HDR-Backbuffer. Das ist
  nicht zwingend immer das vollständige sichtbare Bild (z. B. bei Overlay/MPO/DirectFlip).
- Kein Profil für SDR und keine Adaption an Local-Dimming-Zonengrenzen.
- Keine Garantie für Exclusive Fullscreen, Anti-Cheat-Spiele oder HDR-Optimierung.
- Die originale Injektion greift in `dwm.exe` ein. Eine inkompatible DLL kann
  Schwarzbild, Abstürze oder instabiles Verhalten verursachen.
- Helligkeitsanhebungen in kleinen Mini-LED-Highlights können **Halos verstärken**.
- Der native D3D11-Histogramm-Pfad und das HLSL-Binding sind erst nach einem
  erfolgreichen Windows-Build und überprüfbaren CalMAN-Messungen freizugeben.

## Build unter Windows (Visual Studio 2022 / Windows 11 25H2)

1. **Originale funktionierende Dateien sichern** (`DwmLutGUI.exe`, `dwm_lut.dll`, `config.xml`).
2. VS 2022 Desktopentwicklung mit C++, Windows SDK, .NET Desktop Buildtools,
   `vcpkg` und `MinHook` entsprechend dem mitgelieferten Original-README einrichten.
3. `DwmLutGUI/DwmLutGUI.sln` im Release/x64-Modus kompilieren.
4. `lutdwm.sln` im Release/x64-Modus kompilieren.
5. **Vor dem Inject**: DirectX-HLSL-Laufzeitkompilierung prüfen; ein Fehler würde
   den Hook-Start verhindern. In `dwm_lut.log` muss außerdem
   `DynamicAPL: histogram initialized` erscheinen.
6. Den GUI-Build und die eigene DLL in einen **separaten Testordner** legen.
   **NICHT** nach `System32` kopieren, **NICHT** ungetestet die laufende DLL ersetzen.
7. Mit nur einer 10%-LUT zunächst das originale Verhalten prüfen. Danach die
   zweite 2%-LUT setzen, „Apply“ und jeweils 2%/10% APL in CalMAN messen.
   Nach jedem Windows-Update erneut testen.

## Testprotokoll in dieser Umgebung
- CMake/CTest: `AplEstimator`, `CubeParser`, `TetrahedralAndPQ`: alle bestanden.
- XML-Projekte (`.vcxproj`, `.csproj`, `.xaml`): erfolgreich geparst.
- **Nicht getestet:** Windows C++/WPF Build, GPU-Compute, HLSL, DWM-Injektion,
  Spiel-Kompatibilität und EOTF am physischen Monitor.

## Wiederherstellen
Vor Tests die originale DLL und GUI separat sichern. Bei Bildproblemen
LUT sofort deaktivieren und mit den gesicherten Originaldateien neu starten.
Für Anti-Cheat-Spiele den DWM-Hook nicht verwenden.

Lizenz und Third-Party-Notices des Forks sind unverändert enthalten.
