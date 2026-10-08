# Compila el motor de Atlas Studio para Windows desde este fork (rama wolo/4.7). Ver wolo/README.md.
#
#   powershell -ExecutionPolicy Bypass -File wolo/build-windows.ps1 [-Install C:\ruta\WoloSimulator] [-Zip]
#
# Pide Visual Studio (Build Tools 2019 o más nuevo, con C++), Python 3 y SCons
# (python -m pip install scons). Con la ruta del checkout muy larga, el compilador choca con el
# límite de 260 caracteres de Windows: clonar el fork en una ruta corta si pasa.
#
#   -Install   copia el resultado a <WoloSimulator>/wolo-labs-simulator/engine/godot.exe, en lugar del
#              que baja tools/fetch-engine.sh, y escribe engine/ENGINE_VERSION = "local".
#   -Zip       deja bin/wolo-godot-windows.zip, el asset que publica el workflow de releases.
#   -CachePath la caché de SCons (la usa el CI entre corridas).
#   -Jobs      cuántos archivos compilar a la vez (por defecto, uno por núcleo). Con poca memoria
#              libre, cl.exe falla con "C1356: no se puede encontrar mspdbcore.dll": bajarlo.
#
# La versión sale como 4.7.2.stable.wolo.<commit>: "wolo" en lugar de "custom_build" (o de
# "official") es como Studio y el empaquetado reconocen el motor del fork.
#
# Con MSVC, production=yes no usa LTO: Godot lo apaga porque con ese compilador no ayuda
# (platform/windows/detect.py). Medido en 12 núcleos: 13 minutos desde cero.
param(
    [string]$Install,
    [switch]$Zip,
    [string]$CachePath,
    [int]$Jobs = [Environment]::ProcessorCount
)

$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent

# Las dependencias que las compilaciones oficiales traen: AccessKit, ANGLE y el SDK de Direct3D 12.
# Sin MSYSTEM (que Git Bash define) los scripts las dejan en %LOCALAPPDATA%\Godot\build_deps, compartidas
# entre checkouts; con él, en bin/build_deps. Se usa la que exista y se pasan con rutas absolutas.
Remove-Item Env:MSYSTEM -ErrorAction SilentlyContinue
$depsDir = Join-Path $env:LOCALAPPDATA "Godot\build_deps"
if (-not (Test-Path (Join-Path $depsDir "mesa-x86_64-msvc")) -and (Test-Path (Join-Path $root "bin\build_deps\mesa-x86_64-msvc"))) {
    $depsDir = Join-Path $root "bin\build_deps"
}
if (-not (Test-Path (Join-Path $depsDir "mesa-x86_64-msvc"))) {
    Push-Location $root
    foreach ($script in @("install_accesskit", "install_angle", "install_d3d12_sdk_windows")) {
        python "misc/scripts/$script.py"
        if ($LASTEXITCODE -ne 0) { throw "Falló $script" }
    }
    Pop-Location
}
$deps = $depsDir -replace "\\", "/"

# WinRT (las voces de OneCore) pide el Windows SDK 10.0.22621 o más nuevo. Si no está, se compila
# sin: Studio no usa esas voces.
$sdks = Get-ChildItem "${env:ProgramFiles(x86)}\Windows Kits\10\Include" -Directory -ErrorAction SilentlyContinue |
    Where-Object { [version]$_.Name -ge [version]"10.0.22621.0" }
$winrt = if ($sdks) { "yes" } else { "no" }
$scons = @(
    "-m", "SCons", "platform=windows", "target=editor", "production=yes", "winrt=$winrt",
    "-j$Jobs",
    "accesskit_sdk_path=$deps/accesskit", "angle_libs=$deps/angle",
    "mesa_libs=$deps/mesa", "agility_sdk_path=$deps/agility_sdk"
)
if ($CachePath) { $scons += "cache_path=$CachePath" }
$env:BUILD_NAME = "wolo"
$watch = [Diagnostics.Stopwatch]::StartNew()
Push-Location $root
python @scons
$code = $LASTEXITCODE
Pop-Location
if ($code -ne 0) { throw "SCons terminó con error $code" }
$binary = Join-Path $root "bin\godot.windows.editor.x86_64.exe"
Write-Host "  ✔ $binary ($([math]::Round($watch.Elapsed.TotalMinutes, 1)) min, winrt=$winrt)"

if ($Zip) {
    # El zip lleva el ejecutable con el nombre con el que lo usa Studio: fetch-engine.sh lo descomprime
    # directo en engine/.
    $stage = Join-Path $root "bin\wolo-zip"
    Remove-Item $stage -Recurse -Force -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Force -Path $stage | Out-Null
    Copy-Item $binary (Join-Path $stage "godot.exe")
    $zipPath = Join-Path $root "bin\wolo-godot-windows.zip"
    Compress-Archive -Path (Join-Path $stage "godot.exe") -DestinationPath $zipPath -Force
    Remove-Item $stage -Recurse -Force
    Write-Host "  ✔ $zipPath"
}

if ($Install) {
    $engine = Join-Path $Install "wolo-labs-simulator\engine"
    if (-not (Test-Path (Join-Path $Install "wolo-labs-simulator"))) { throw "$Install no parece el repo WoloSimulator" }
    New-Item -ItemType Directory -Force -Path $engine | Out-Null
    Copy-Item $binary (Join-Path $engine "godot.exe") -Force
    foreach ($marker in @("._sc_", "_sc_", ".gdignore")) { New-Item -ItemType File -Force -Path (Join-Path $engine $marker) | Out-Null }
    # fetch-engine.sh compara este archivo con el tag fijado: "local" hace que lo reemplace la
    # próxima vez que se corra, en vez de confundirlo con una release.
    Set-Content -Path (Join-Path $engine "ENGINE_VERSION") -Value "local" -NoNewline -Encoding ascii
    Write-Host "  ✔ instalado en $engine\godot.exe"
}
