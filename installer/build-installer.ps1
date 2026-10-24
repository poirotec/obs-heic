param(
    [string]$PluginDll,
    [string]$VcpkgRoot,
    [string]$InnoCompiler
)

$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path $PSScriptRoot -Parent
$workspaceRoot = Split-Path (Split-Path $projectRoot -Parent) -Parent
$payload = Join-Path $PSScriptRoot 'payload'

if (-not $PluginDll) {
    $PluginDll = @(
        (Join-Path $projectRoot 'build_x64\RelWithDebInfo\obs-heic.dll'),
        (Join-Path $projectRoot 'build_x64_libheif2\RelWithDebInfo\obs-heic.dll')
    ) | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
}

if (-not $VcpkgRoot) {
    $vcpkgBase = if ($env:VCPKG_ROOT) { $env:VCPKG_ROOT } else { Join-Path $workspaceRoot 'work\vcpkg' }
    $VcpkgRoot = Join-Path $vcpkgBase 'installed\x64-windows'
}

if (-not $InnoCompiler) {
    $InnoCompiler = @(
        (Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6\ISCC.exe'),
        (Join-Path $env:ProgramFiles 'Inno Setup 6\ISCC.exe'),
        (Join-Path $workspaceRoot 'work\tools\InnoSetup6\ISCC.exe')
    ) | Where-Object { $_ -and (Test-Path -LiteralPath $_) } | Select-Object -First 1
}

$required = @(
    $PluginDll,
    (Join-Path $VcpkgRoot 'bin\heif.dll'),
    (Join-Path $VcpkgRoot 'bin\libde265.dll'),
    $InnoCompiler
)
foreach ($file in $required) {
    if (-not (Test-Path -LiteralPath $file)) {
        throw "Required build file not found: $file"
    }
}

New-Item -ItemType Directory -Force -Path `
    (Join-Path $payload 'bin\64bit'), `
    (Join-Path $payload 'data\locale'), `
    (Join-Path $payload 'licenses') | Out-Null

Copy-Item -Force $PluginDll (Join-Path $payload 'bin\64bit\obs-heic.dll')
Copy-Item -Force (Join-Path $VcpkgRoot 'bin\heif.dll') (Join-Path $payload 'bin\64bit\heif.dll')
Copy-Item -Force (Join-Path $VcpkgRoot 'bin\libde265.dll') (Join-Path $payload 'bin\64bit\libde265.dll')
Copy-Item -Force (Join-Path $projectRoot 'data\locale\en-US.ini') (Join-Path $payload 'data\locale\en-US.ini')
Copy-Item -Force (Join-Path $projectRoot 'data\locale\ja-JP.ini') (Join-Path $payload 'data\locale\ja-JP.ini')
Copy-Item -Force (Join-Path $VcpkgRoot 'share\libheif\copyright') (Join-Path $payload 'licenses\libheif.txt')
Copy-Item -Force (Join-Path $VcpkgRoot 'share\libde265\copyright') (Join-Path $payload 'licenses\libde265.txt')

& $InnoCompiler (Join-Path $PSScriptRoot 'obs-heic.iss')
if ($LASTEXITCODE -ne 0) {
    throw "Inno Setup compiler failed with exit code $LASTEXITCODE"
}
