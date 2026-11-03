# K-54 · Keller54 · by Frank E. - Windows-Build und MSI
# Wird von "K-54 Windows bauen.bat" gestartet (als Administrator).
$ErrorActionPreference = 'Stop'
$Src      = $PSScriptRoot
$BuildDir = Join-Path $env:SystemDrive 'K54-build'      # kurzer Pfad, nicht im OneDrive
$Dist     = Join-Path $Src 'dist'
$Log      = Join-Path $Src 'build-windows.log'
Start-Transcript -Path $Log -Force | Out-Null

function Step($t) { Write-Host ""; Write-Host "==> $t" -ForegroundColor Yellow }
function Fail($t) {
    Write-Host ""; Write-Host "FEHLER: $t" -ForegroundColor Red
    Write-Host "Protokoll: $Log"
    Stop-Transcript | Out-Null
    exit 1
}
function Refresh-Path {
    $m = [Environment]::GetEnvironmentVariable('Path', 'Machine')
    $u = [Environment]::GetEnvironmentVariable('Path', 'User')
    $env:Path = "$m;$u;$env:USERPROFILE\.dotnet\tools"
}
function Have($cmd) { [bool](Get-Command $cmd -ErrorAction SilentlyContinue) }
function Winget-Install($id, $extra) {
    Write-Host "   installiere $id ..."
    $wargs = @('install', '--id', $id, '-e', '--silent', '--accept-package-agreements', '--accept-source-agreements') + $extra
    & winget @wargs
    # 0 = ok, -1978335189 = schon installiert / kein Update
    if ($LASTEXITCODE -ne 0 -and $LASTEXITCODE -ne -1978335189) { Write-Host "   (winget meldet $LASTEXITCODE - mache weiter)" }
}

Write-Host "K-54 Windows-Build  -  Keller54 · by Frank E." -ForegroundColor Cyan
Write-Host "Beim ersten Mal 20-40 Minuten (mehrere GB Download). Fenster bitte offen lassen."

# Version aus CMakeLists.txt
$cm = Get-Content (Join-Path $Src 'CMakeLists.txt') -Raw
if ($cm -notmatch 'project\(K54 VERSION ([0-9.]+)') { Fail 'Version in CMakeLists.txt nicht gefunden' }
$Version = $Matches[1]
Write-Host "Version $Version"

# ---------------------------------------------------------------- Werkzeuge
Step '1/5  Werkzeuge pruefen'
if (-not (Have 'winget')) {
    Fail 'winget fehlt. Im Microsoft Store "App-Installer" installieren/aktualisieren und neu starten.'
}
Refresh-Path
if (-not (Have 'cmake'))  { Winget-Install 'Kitware.CMake' @() }
if (-not (Have 'git'))    { Winget-Install 'Git.Git' @() }
if (-not (Have 'dotnet')) { Winget-Install 'Microsoft.DotNet.SDK.8' @() }

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs = $null
if (Test-Path $vswhere) {
    $vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
}
if (-not $vs) {
    Write-Host '   Visual Studio C++ Build Tools fehlen - werden installiert (dauert, ca. 3-6 GB) ...'
    Winget-Install 'Microsoft.VisualStudio.2022.BuildTools' @('--override', '--quiet --wait --norestart --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended')
    if (Test-Path $vswhere) {
        $vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    }
    if (-not $vs) { Fail 'C++ Build Tools wurden nicht gefunden. Bitte Rechner neu starten und die .bat nochmal starten.' }
}
Write-Host "   Visual Studio: $vs"
Refresh-Path
foreach ($c in 'cmake', 'git', 'dotnet') { if (-not (Have $c)) { Fail "$c nicht gefunden - bitte Rechner neu starten und nochmal starten." } }

# WiX v5
if (-not (Have 'wix')) {
    Write-Host '   installiere WiX Toolset 5 ...'
    & dotnet tool install --global wix --version 5.0.2
    Refresh-Path
}
if (-not (Have 'wix')) { Fail 'WiX nicht gefunden.' }
& wix extension add -g WixToolset.UI.wixext/5.0.2 | Out-Null

# ---------------------------------------------------------------- Konfigurieren
Step '2/5  Projekt einrichten (laedt beim ersten Mal JUCE von GitHub)'
& cmake -S $Src -B $BuildDir -A x64 -DK54_COPY_AFTER_BUILD=OFF
if ($LASTEXITCODE -ne 0) { Fail 'CMake-Konfiguration fehlgeschlagen' }

# ---------------------------------------------------------------- Bauen
Step '3/5  K-54 bauen (VST3 + App)'
& cmake --build $BuildDir --config Release --target K54_VST3 K54_Standalone --parallel
if ($LASTEXITCODE -ne 0) { Fail 'Kompilieren fehlgeschlagen' }

$Art = Join-Path $BuildDir 'K54_artefacts\Release'
if (-not (Test-Path (Join-Path $Art 'VST3\K-54.vst3'))) { Fail "VST3 nicht gefunden in $Art" }
if (-not (Test-Path (Join-Path $Art 'Standalone\K-54.exe'))) { Fail "K-54.exe nicht gefunden in $Art" }

# ---------------------------------------------------------------- MSI
Step '4/5  MSI erzeugen'
New-Item -ItemType Directory -Force -Path $Dist | Out-Null
$Msi = Join-Path $Dist "K-54-$Version.msi"
& wix build (Join-Path $Src 'Installer\windows\K54.wxs') -ext WixToolset.UI.wixext -culture de-DE `
    -d "Version=$Version" -d "BuildDir=$Art" -arch x64 -o $Msi
if ($LASTEXITCODE -ne 0) { Fail 'MSI-Erstellung fehlgeschlagen' }

# ---------------------------------------------------------------- Fertig
Step '5/5  Fertig'
Write-Host "MSI: $Msi" -ForegroundColor Green
Write-Host 'Doppelklick auf die MSI installiert den K-54 (VST3 nach C:\Program Files\Common Files\VST3).'
Stop-Transcript | Out-Null
Start-Process explorer.exe "/select,`"$Msi`""
exit 0
