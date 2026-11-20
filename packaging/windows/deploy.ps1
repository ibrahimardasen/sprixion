# Run from a "x64 Native Tools" / msvc-dev-cmd shell with Qt's bin dir (windeployqt) and Ninja on PATH.
$ErrorActionPreference = 'Stop'
$root = Resolve-Path "$PSScriptRoot\..\.."
Set-Location $root
$ver = '0.1.0'

cmake -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
if ($LASTEXITCODE) { throw 'build failed' }

$dist = "build-release\Sprixion-$ver"
Remove-Item -Recurse -Force $dist -ErrorAction SilentlyContinue
New-Item -ItemType Directory $dist | Out-Null
# The Qt/MSVC setup on CI puts executables in the build root, not src\app, so look it up.
$exe = Get-ChildItem build-release -Recurse -Filter sprixion.exe | Where-Object { $_.FullName -notlike "*\Sprixion-$ver\*" } | Select-Object -First 1
if (-not $exe) { throw 'sprixion.exe not found in build-release' }
Copy-Item $exe.FullName $dist
windeployqt --release --compiler-runtime --no-translations "$dist\sprixion.exe"
if ($LASTEXITCODE) { throw 'windeployqt failed' }
# Svg icons load at runtime; don't rely on windeployqt noticing them.
foreach ($p in 'iconengines\qsvgicon.dll', 'imageformats\qsvg.dll') {
  if (-not (Test-Path "$dist\$p")) {
    $qt = Split-Path (Split-Path (Get-Command windeployqt).Source)
    New-Item -ItemType Directory -Force (Split-Path "$dist\$p") | Out-Null
    Copy-Item "$qt\plugins\$p" "$dist\$p"
  }
}

Compress-Archive -Force -Path $dist -DestinationPath "Sprixion-$ver-win64.zip"

$iscc = (Get-Command iscc -ErrorAction SilentlyContinue).Source
if (-not $iscc) { $iscc = 'C:\Program Files (x86)\Inno Setup 6\ISCC.exe' }
& $iscc "/DDistDir=$root\$dist" "/O$root" packaging\windows\sprixion.iss
if ($LASTEXITCODE) { throw 'ISCC failed' }
