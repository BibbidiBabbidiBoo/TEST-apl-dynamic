param([string]$Root = (Resolve-Path "$PSScriptRoot/..").Path)
$ErrorActionPreference = 'Stop'
$out = Join-Path $Root 'build/shader-validation'
python "$PSScriptRoot/export_embedded_shaders.py" --root $Root --output $out
if ($LASTEXITCODE -ne 0) { throw 'Shader extraction failed' }
$fxc = (Get-Command fxc.exe -ErrorAction SilentlyContinue).Source
if (!$fxc) {
  $kits = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits/10/bin'
  $candidates = @(Get-ChildItem $kits -Recurse -Filter fxc.exe -ErrorAction SilentlyContinue |
                  Where-Object { $_.FullName -match '[\\/]x64[\\/]fxc\.exe$' } |
                  Sort-Object FullName -Descending)
  if (!$candidates) { throw 'Windows SDK fxc.exe not found' }
  $fxc=$candidates[0].FullName
}
$checks = @(
   @{source='DwmDualLut.hlsl'; entry='VS'; target='vs_5_0'; out='dwmlut_vs.dxbc'},
   @{source='DwmDualLut.hlsl'; entry='PS'; target='ps_5_0'; out='dwmlut_ps.dxbc'},
   @{source='AplHistogramCS.hlsl'; entry='main'; target='cs_5_0'; out='apl_cs.dxbc'}
)
foreach($check in $checks) {
  & $fxc /nologo /WX /T $check.target /E $check.entry /Fo (Join-Path $out $check.out) (Join-Path $out $check.source)
  if ($LASTEXITCODE -ne 0) { throw "Shader failed: $($check.target) $($check.entry)" }
  Write-Host "PASS: $($check.target) $($check.entry)"
}
