param(
    [string]$BuildDir = "build-vs2022",
    [string]$Config = "Release",
    [string]$Generator = "Visual Studio 17 2022",
    [string]$Arch = "x64",
    [switch]$SkipConfigure
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ($env:OS -ne "Windows_NT") {
    throw "This script is for Windows packaging only."
}

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir "..")
$distDir = Join-Path $repoRoot "dist"
$actionCliDir = Join-Path $repoRoot "action-cli"
$issPath = Join-Path $actionCliDir "InstallerScript.iss"

Push-Location $repoRoot
try {
    if (-not (Test-Path $actionCliDir)) {
        New-Item -ItemType Directory -Path $actionCliDir | Out-Null
    }

    if (-not $SkipConfigure) {
        Write-Host "==> Configure CMake: $Generator / $Arch"
        & cmake -S . -B $BuildDir -G $Generator -A $Arch
        if ($LASTEXITCODE -ne 0) {
            throw "CMake configure failed."
        }
    }

    Write-Host "==> Build targets: BookCanvas + booksim2 ($Config)"
    & cmake --build $BuildDir --config $Config --target BookCanvas booksim2
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed."
    }

    $bookCanvasExe = Join-Path $BuildDir (Join-Path $Config "BookCanvas.exe")
    if (-not (Test-Path $bookCanvasExe)) {
        throw "BookCanvas executable not found: $bookCanvasExe"
    }

    $booksimExe = Join-Path $repoRoot "3rdpart/booksim2/src/booksim.exe"
    if (-not (Test-Path $booksimExe)) {
        throw "booksim executable not found: $booksimExe"
    }

    if (Test-Path $distDir) {
        Remove-Item -Recurse -Force $distDir
    }
    New-Item -ItemType Directory -Path $distDir | Out-Null

    Write-Host "==> Stage runtime files to dist/"
    Copy-Item $bookCanvasExe (Join-Path $distDir "bookcanvas.exe")
    Copy-Item $booksimExe (Join-Path $distDir "booksim.exe")

    $windeployqt = $null
    $qtHint = "C:\Qt\qt6\6.7.0\msvc2019_64\bin"
    $candidatePaths = @(
        (Join-Path $qtHint "windeployqt.exe"),
        "windeployqt.exe"
    )
    foreach ($candidate in $candidatePaths) {
        $cmd = Get-Command $candidate -ErrorAction SilentlyContinue
        if ($cmd) {
            $windeployqt = $cmd.Source
            break
        }
    }
    if (-not $windeployqt) {
        throw "windeployqt.exe not found. Please install Qt tools and add it to PATH."
    }

    Write-Host "==> Run windeployqt"
    & $windeployqt --release --compiler-runtime (Join-Path $distDir "bookcanvas.exe")
    if ($LASTEXITCODE -ne 0) {
        throw "windeployqt failed."
    }

    if (-not (Test-Path $issPath)) {
        throw "Installer script not found: $issPath (run CMake configure first)."
    }

    $iscc = $null
    $isccCandidates = @(
        "C:\Program Files (x86)\Inno Setup 6\ISCC.exe",
        "C:\Program Files\Inno Setup 6\ISCC.exe",
        "ISCC.exe"
    )
    foreach ($candidate in $isccCandidates) {
        $cmd = Get-Command $candidate -ErrorAction SilentlyContinue
        if ($cmd) {
            $iscc = $cmd.Source
            break
        }
    }
    if (-not $iscc) {
        throw "ISCC.exe not found. Please install Inno Setup 6."
    }

    Write-Host "==> Compile installer with Inno Setup"
    & $iscc $issPath
    if ($LASTEXITCODE -ne 0) {
        throw "Inno Setup compile failed."
    }

    $installer = Get-ChildItem -Path $actionCliDir -Filter "BookCanvas-*-Setup.exe" -File |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1

    if ($null -eq $installer) {
        throw "Packaging succeeded but installer not found in '$actionCliDir'."
    }

    Write-Host ""
    Write-Host "Installer generated:"
    Write-Host $installer.FullName
}
finally {
    Pop-Location
}
