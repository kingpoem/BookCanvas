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

# booksim 为 MinGW 构建；须与 bookcanvas 同目录分发 libgcc/libstdc++/libwinpthread，否则会报缺少 libgcc_s_seh-1.dll。
# 设置 BOOKSIM_MINGW_BIN（如 C:\msys64\ucrt64\bin），或将用于编译 booksim 的 g++.exe 所在目录加入 PATH。
function Copy-BooksimMingwRuntimeDlls {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Destination
    )

    $mingwBin = $env:BOOKSIM_MINGW_BIN
    if (-not $mingwBin) {
        $gpp = Get-Command "g++.exe" -ErrorAction SilentlyContinue
        if ($gpp) {
            $mingwBin = Split-Path -Parent $gpp.Source
        }
    }
    if (-not $mingwBin -or -not (Test-Path -LiteralPath $mingwBin)) {
        throw "无法定位 MinGW 的 bin（缺少 libgcc_s_seh-1.dll 等）。请设置 BOOKSIM_MINGW_BIN 为编译 booksim 时使用的目录（例如 C:\msys64\ucrt64\bin），或将该环境的 g++.exe 加入 PATH 后重新运行打包脚本。"
    }

    foreach ($name in @("libstdc++-6.dll", "libwinpthread-1.dll")) {
        $src = Join-Path $mingwBin $name
        if (-not (Test-Path -LiteralPath $src)) {
            throw "未找到 booksim 依赖的运行库: $src （检查 BOOKSIM_MINGW_BIN=$mingwBin）"
        }
        Copy-Item -LiteralPath $src -Destination $Destination -Force
    }

    $gccCopied = $false
    foreach ($name in @("libgcc_s_seh-1.dll", "libgcc_s_dw2-1.dll")) {
        $src = Join-Path $mingwBin $name
        if (Test-Path -LiteralPath $src) {
            Copy-Item -LiteralPath $src -Destination $Destination -Force
            $gccCopied = $true
            break
        }
    }
    if (-not $gccCopied) {
        throw "在 $mingwBin 中未找到 libgcc_s_seh-1.dll 或 libgcc_s_dw2-1.dll。"
    }

    Write-Host "    已从 $mingwBin 复制 MinGW 运行库（供 booksim.exe 使用）"
}

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

    Write-Host "==> 复制 booksim 所需的 MinGW 运行库到 dist（windeployqt 不会处理）"
    Copy-BooksimMingwRuntimeDlls -Destination $distDir

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
