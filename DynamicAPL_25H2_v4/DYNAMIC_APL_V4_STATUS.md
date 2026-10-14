# Dynamic APL v4 - Build-Validierung und Schutzmaßnahmen

Stand: geänderter Quellcode für Windows 11 25H2. **Nicht unter Windows kompiliert oder auf DWM getestet.**

## Verbesserungen gegenüber v3

- Shaderkompilierung für VS/PS auch in **Release** explizit geprüft; bei Fehlern
  Abbruch statt Dereferenzierung ungültiger Shader-Pointer im DWM-Prozess.
- Zusätzliche APL-Snapshot-Textur auf max. 3840 × 2160 Pixel begrenzt.
  Größere Modi verwenden konservativ nur die 10%-LUT.
- Vertex Shader / Input Layout creation is also checked in Release.
    - C++-Assertions bleiben auch im Release-Testbuild aktiv.
    - `.cube`-Loader prüft Größen (2–65) und Ressourcen-Allokation vor dem Einlesen.
- `scripts/export_embedded_shaders.py`: exportiert den tatsächlichen eingebetteten
  HLSL-Text, inklusive aus C++ eingesetzter Präprozessormakros.
- `scripts/verify_hlsl_windows.ps1`: prüft VS 5.0, PS 5.0 und CS 5.0 mit `fxc`.
  Die Prüfung ist vor den DLL-Build in GitHub Actions geschaltet.

## Lokal unter Linux prüfen

```bash
cmake -S portable_tests -B build/tests -DCMAKE_BUILD_TYPE=Release
cmake --build build/tests --parallel
ctest --test-dir build/tests --output-on-failure
python3 scripts/export_embedded_shaders.py --root . --output build/shaders
```

## Windows-Build

GitHub Actions: `.github/workflows/build-windows.yml` hat Windows-2022-Runner,
kompiliert den HLSL-Code (fxc) und anschließend DLL/GUI mittels MSBuild.
Ohne eine tatsächlich grüne Workflow-Ausführung ist das **kein fertiges Windows-Programm**.
Eine eventuell erzeugte DLL ist als Experiment zu behandeln, nicht als Produktiv-Build.

**Wichtig**: Diese Software greift via DLL-Injektion in `dwm.exe` ein und kann Abstürze,
Schwarzbild, unruhige EOTF-Übergänge oder verstärkte Halos verursachen. Vor Tests
Originalzustand sichern; nicht in System32 installieren und nicht neben Anti-Cheat-Spielen verwenden.

Die 2%- und 10%-APL-Korrekturen sind aus der CalMAN-Messung ohne aktive LUT abgeleitet;
sie sind nicht durch dynamische CalMAN-Nachmessungen bestätigt. APL-Heuristik misst
Bildhelligkeitsverteilung, nicht echte Mini-LED-Zonen oder Monitor-APL.
