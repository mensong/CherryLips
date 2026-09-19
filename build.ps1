<#
.SYNOPSIS
  CherryLips static /MT build script.

.DESCRIPTION
  Builds all dependencies as static /MT libraries, then builds CherryLips.dll
  and CherryLips-Test.exe.  Designed to run on a fresh checkout of the repo
  on another machine that has Visual Studio 2022 + CMake + Git installed.

.PARAMETER Config
  Build configuration: "Release" (default) or "Debug".

.PARAMETER Platform
  Target platform: "x64" (default).  Only x64 is supported for /MT static.

.PARAMETER CleanDeps
  Force a full rebuild of all dependencies (removes deps\lib, deps\include,
  and all install prefixes).

.EXAMPLE
  .\build.ps1
  .\build.ps1 -Config Debug
  .\build.ps1 -CleanDeps
#>
[CmdletBinding()]
param(
    [ValidateSet('Release','Debug')]
    [string]$Config = 'Release',

    [ValidateSet('x64')]
    [string]$Platform = 'x64',

    [switch]$CleanDeps
)

$ErrorActionPreference = 'Stop'

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
$RepoRoot    = Split-Path -Parent $MyInvocation.MyCommand.Path
$DepsDir     = Join-Path $RepoRoot 'deps'
$DepsSrc     = Join-Path $DepsDir 'src'
$DepsLib     = Join-Path $DepsDir 'lib'
$DepsInc     = Join-Path $DepsDir 'include'
$DepsLocal   = Join-Path $DepsDir 'local'
$MinioSrc    = Join-Path $RepoRoot 'minio-cpp'

$ZlibPrefix    = Join-Path $DepsDir 'zlib-install'
$OpensslPrefix = Join-Path $DepsDir 'openssl-install'
$PugixmlPrefix = Join-Path $DepsDir 'pugixml-install'

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
function Write-Step([string]$msg) {
    Write-Host "`n=== $msg ===" -ForegroundColor Cyan
}

function Invoke-Checked([string]$exe, [string[]]$cmdArgs, [string]$wd = $RepoRoot) {
    Write-Host "  > $exe $($cmdArgs -join ' ')"
    Push-Location $wd
    & $exe @cmdArgs
    $code = $LASTEXITCODE
    Pop-Location
    if ($code -ne 0) { throw "Command failed (exit $code): $exe $($cmdArgs -join ' ')" }
}

# Copy a directory tree (replaces destination if it exists).
function Copy-Dir([string]$src, [string]$dst) {
    if (Test-Path $dst) { Remove-Item $dst -Recurse -Force }
    & robocopy $src $dst /E /NFL /NDL /NJH /NJS /NC /NS | Out-Null
    # robocopy returns 0-7 for success; anything >= 8 is an error.
    if ($LASTEXITCODE -ge 8) { throw "robocopy failed (exit $LASTEXITCODE): $src -> $dst" }
}

# ---------------------------------------------------------------------------
# Locate toolchain
# ---------------------------------------------------------------------------
Write-Step 'Locating Visual Studio / MSBuild'

$vswhereExe = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhereExe)) {
    throw "vswhere.exe not found at $vswhereExe. Install Visual Studio 2022."
}
$vsRoot = & $vswhereExe -latest -property installationPath
if (-not $vsRoot) { throw 'Visual Studio 2022 installation not found.' }

$script:MSBuild = Join-Path $vsRoot 'MSBuild\Current\Bin\MSBuild.exe'
if (-not (Test-Path $script:MSBuild)) { throw "MSBuild.exe not found at $($script:MSBuild)" }

# Locate Perl (OpenSSL build needs it).  Check PATH first, then bundled Strawberry Perl.
$script:Perl = $null
$perlInPath = Get-Command perl.exe -ErrorAction SilentlyContinue
if ($perlInPath) {
    $script:Perl = $perlInPath.Source
} else {
    $bundledPerl = Join-Path $DepsDir 'strawberry-perl\perl\bin\perl.exe'
    if (Test-Path $bundledPerl) {
        $script:Perl = $bundledPerl
    } else {
        throw 'Perl not found. Either install Strawberry Perl or extract deps\strawberry-perl-portable.zip.'
    }
}

# Locate CMake.
$script:CMake = (Get-Command cmake.exe -ErrorAction SilentlyContinue).Source
if (-not $script:CMake) { throw 'cmake.exe not found on PATH. Install CMake and add it to PATH.' }

# Locate Git (minio-cpp CMake may clone cpp-httplib if not provided).
$script:Git = (Get-Command git.exe -ErrorAction SilentlyContinue).Source
if (-not $script:Git) { throw 'git.exe not found on PATH.' }

Write-Host "  MSBuild : $($script:MSBuild)"
Write-Host "  CMake   : $($script:CMake)"
Write-Host "  Perl    : $($script:Perl)"
Write-Host "  Git     : $($script:Git)"

# ---------------------------------------------------------------------------
# Set up the MSVC build environment (cl.exe, nmake.exe, etc.)
# ---------------------------------------------------------------------------
Write-Step 'Setting up MSVC build environment'
$vcvarsall = Join-Path $vsRoot 'VC\Auxiliary\Build\vcvarsall.bat'
if (-not (Test-Path $vcvarsall)) { throw "vcvarsall.bat not found at $vcvarsall" }
$envOutput = & cmd /c "`"$vcvarsall`" x64 && set" 2>&1
foreach ($line in $envOutput) {
    if ($line -match '^([^=]+)=(.*)') {
        [System.Environment]::SetEnvironmentVariable($Matches[1], $Matches[2], 'Process')
    }
}
Write-Host "  cl.exe  : $((Get-Command cl.exe -ErrorAction SilentlyContinue).Source)"
Write-Host "  nmake   : $((Get-Command nmake.exe -ErrorAction SilentlyContinue).Source)"

# vcvarsall.bat may set VCPKG_ROOT to VS's integrated vcpkg; clear it so
# minio-cpp CMake uses our pre-built deps instead of trying vcpkg install.
$env:VCPKG_ROOT = $null

# ---------------------------------------------------------------------------
# Ensure deps\lib and deps\include exist
# ---------------------------------------------------------------------------
New-Item -ItemType Directory -Force -Path $DepsLib | Out-Null
New-Item -ItemType Directory -Force -Path $DepsInc | Out-Null

# ---------------------------------------------------------------------------
# Optional: clean previous dependency builds
# ---------------------------------------------------------------------------
if ($CleanDeps) {
    Write-Step 'Cleaning previous dependency build artifacts'
    foreach ($d in @($ZlibPrefix, $OpensslPrefix, $PugixmlPrefix,
                     (Join-Path $DepsSrc 'zlib-build'),
                     (Join-Path $DepsSrc 'pugixml-build'),
                     (Join-Path $DepsSrc 'minio-cpp-build'))) {
        if (Test-Path $d) { Remove-Item $d -Recurse -Force; Write-Host "  Removed $d" }
    }
    # Clear the flat deps dirs so distribute steps re-run.
    Get-ChildItem $DepsLib -File -ErrorAction SilentlyContinue | Remove-Item -Force
    Get-ChildItem $DepsInc -ErrorAction SilentlyContinue | Remove-Item -Recurse -Force
    if (Test-Path $DepsLocal) { Remove-Item $DepsLocal -Recurse -Force }
    New-Item -ItemType Directory -Force -Path $DepsLib | Out-Null
    New-Item -ItemType Directory -Force -Path $DepsInc | Out-Null
}

# ---------------------------------------------------------------------------
# Build zlib (static /MT)
# ---------------------------------------------------------------------------
Write-Step 'Building zlib 1.3.1 (static /MT)'
$zlibSrc   = Join-Path $DepsSrc 'zlib-1.3.1'
$zlibBuild = Join-Path $DepsSrc 'zlib-build'
if (-not (Test-Path (Join-Path $ZlibPrefix 'lib\zlib.lib'))) {
    if (-not (Test-Path $zlibSrc)) { throw "zlib source not found at $zlibSrc." }
    if (Test-Path $zlibBuild) { Remove-Item $zlibBuild -Recurse -Force }
    Invoke-Checked $script:CMake @(
        '-S', $zlibSrc, '-B', $zlibBuild,
        '-G', '"Visual Studio 17 2022"', '-A', 'x64',
        "-DCMAKE_INSTALL_PREFIX=$ZlibPrefix",
        '-DBUILD_SHARED_LIBS=OFF',
        '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded'
    )
    Invoke-Checked $script:CMake @('--build', $zlibBuild, '--config', 'Release', '--', '/m')
    Invoke-Checked $script:CMake @('--install', $zlibBuild, '--config', 'Release')
}
# Distribute to flat deps dirs (always run -- idempotent).
if (-not (Test-Path (Join-Path $DepsLib 'zlib.lib'))) {
    Copy-Item (Join-Path $ZlibPrefix 'lib\zlib.lib')    $DepsLib -Force
    Copy-Item (Join-Path $ZlibPrefix 'include\zlib.h')  $DepsInc -Force
    Copy-Item (Join-Path $ZlibPrefix 'include\zconf.h') $DepsInc -Force
    Write-Host '  Distributed zlib to deps.'
} else { Write-Host '  zlib already distributed, skipping.' }

# ---------------------------------------------------------------------------
# Build OpenSSL (static /MT, no-asm, no-shared)
# ---------------------------------------------------------------------------
Write-Step 'Building OpenSSL 3.4.0 (static /MT, no-shared)'
$opensslSrc = Join-Path $DepsSrc 'openssl-3.4.0'
if (-not (Test-Path (Join-Path $OpensslPrefix 'lib\libcrypto.lib'))) {
    if (-not (Test-Path $opensslSrc)) { throw "OpenSSL source not found at $opensslSrc." }
    Push-Location $opensslSrc
    & $script:Perl 'Configure' 'VC-WIN64A' 'no-asm' 'no-shared' `
        "--prefix=$OpensslPrefix" `
        "--openssldir=$OpensslPrefix" `
        '-static' '/MT'
    if ($LASTEXITCODE -ne 0) { Pop-Location; throw 'OpenSSL Configure failed.' }
    & nmake 'build_libs'
    if ($LASTEXITCODE -ne 0) { Pop-Location; throw 'OpenSSL nmake build_libs failed.' }
    & nmake 'install_dev'
    if ($LASTEXITCODE -ne 0) { Pop-Location; throw 'OpenSSL nmake install_dev failed.' }
    Pop-Location
}
# Distribute to flat deps dirs (always run -- idempotent).
if (-not (Test-Path (Join-Path $DepsLib 'libcrypto.lib'))) {
    Copy-Item (Join-Path $OpensslPrefix 'lib\libcrypto.lib') $DepsLib -Force
    Copy-Item (Join-Path $OpensslPrefix 'lib\libssl.lib')    $DepsLib -Force
    Copy-Dir (Join-Path $OpensslPrefix 'include\openssl') (Join-Path $DepsInc 'openssl')
    Write-Host '  Distributed OpenSSL to deps.'
} else { Write-Host '  OpenSSL already distributed, skipping.' }

# ---------------------------------------------------------------------------
# Build pugixml (static /MT)
# ---------------------------------------------------------------------------
Write-Step 'Building pugixml 1.14 (static /MT)'
$pugiSrc   = Join-Path $DepsSrc 'pugixml-1.14'
$pugiBuild = Join-Path $DepsSrc 'pugixml-build'
if (-not (Test-Path (Join-Path $PugixmlPrefix 'lib\pugixml.lib'))) {
    if (-not (Test-Path $pugiSrc)) { throw "pugixml source not found at $pugiSrc." }
    if (Test-Path $pugiBuild) { Remove-Item $pugiBuild -Recurse -Force }
    Invoke-Checked $script:CMake @(
        '-S', $pugiSrc, '-B', $pugiBuild,
        '-G', '"Visual Studio 17 2022"', '-A', 'x64',
        "-DCMAKE_INSTALL_PREFIX=$PugixmlPrefix",
        '-DBUILD_SHARED_LIBS=OFF',
        '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded'
    )
    Invoke-Checked $script:CMake @('--build', $pugiBuild, '--config', 'Release', '--', '/m')
    Invoke-Checked $script:CMake @('--install', $pugiBuild, '--config', 'Release')
}
# Distribute to flat deps dirs (always run -- idempotent).
if (-not (Test-Path (Join-Path $DepsLib 'pugixml.lib'))) {
    Copy-Item (Join-Path $PugixmlPrefix 'lib\pugixml.lib')        $DepsLib -Force
    Copy-Item (Join-Path $PugixmlPrefix 'include\pugixml.hpp')    $DepsInc -Force
    Copy-Item (Join-Path $PugixmlPrefix 'include\pugiconfig.hpp') $DepsInc -Force
    Write-Host '  Distributed pugixml to deps.'
} else { Write-Host '  pugixml already distributed, skipping.' }

# ---------------------------------------------------------------------------
# Build inih (static /MT, using cl.exe directly)
# ---------------------------------------------------------------------------
Write-Step 'Building inih r58 (static /MT)'
$inihSrc = Join-Path $DepsSrc 'inih-r58'
if (-not (Test-Path (Join-Path $DepsLib 'inih.lib'))) {
    if (-not (Test-Path $inihSrc)) { throw "inih source not found at $inihSrc." }
   Push-Location $inihSrc
   & cl.exe /c /MT /O2 /EHsc /W3 /D_CRT_SECURE_NO_WARNINGS `
       /I. /Icpp ini.c
   if ($LASTEXITCODE -ne 0) { Pop-Location; throw 'inih: cl.exe compile of ini.c failed.' }
   & cl.exe /c /MT /O2 /EHsc /W3 /D_CRT_SECURE_NO_WARNINGS `
        /I. /Icpp /FoINIReader.obj `
       cpp\INIReader.cpp /TP
    if ($LASTEXITCODE -ne 0) { Pop-Location; throw 'inih: cl.exe compile of INIReader.cpp failed.' }
    & lib.exe /OUT:inih.lib ini.obj INIReader.obj
    if ($LASTEXITCODE -ne 0) { Pop-Location; throw 'inih: lib.exe failed.' }
    Pop-Location
    Copy-Item (Join-Path $inihSrc 'inih.lib')     $DepsLib -Force
    Copy-Item (Join-Path $inihSrc 'ini.h')        $DepsInc -Force
    Copy-Item (Join-Path $inihSrc 'cpp\INIReader.h') $DepsInc -Force
    Write-Host '  Built and distributed inih.'
} else { Write-Host '  inih already built, skipping.' }

# ---------------------------------------------------------------------------
# Set up header-only libraries (nlohmann-json, cpp-httplib)
# ---------------------------------------------------------------------------
Write-Step 'Setting up header-only libraries (nlohmann-json, cpp-httplib)'

# nlohmann-json
if (-not (Test-Path (Join-Path $DepsInc 'nlohmann\json.hpp'))) {
   $jsonSrc = Join-Path $DepsSrc 'nlohmann-json-include'
   if (-not (Test-Path $jsonSrc)) { throw "nlohmann-json source not found at $jsonSrc." }
    Copy-Dir (Join-Path $jsonSrc 'include\nlohmann') (Join-Path $DepsInc 'nlohmann')
    Write-Host '  Copied nlohmann-json headers.'
} else { Write-Host '  nlohmann-json already present, skipping.' }

# cpp-httplib (header-only, just copy the single header)
if (-not (Test-Path (Join-Path $DepsInc 'httplib.h'))) {
    $httplibSrc = Get-ChildItem -Path $DepsSrc, $MinioSrc -Filter 'httplib.h' -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($httplibSrc) {
        Copy-Item $httplibSrc.FullName $DepsInc -Force
        Write-Host "  Copied httplib.h from $($httplibSrc.FullName)"
    } else {
        Write-Host '  httplib.h not found locally, downloading from GitHub...'
        $url = 'https://raw.githubusercontent.com/yhirose/cpp-httplib/v0.53.1/httplib.h'
        Invoke-WebRequest -Uri $url -OutFile (Join-Path $DepsInc 'httplib.h')
    }
} else { Write-Host '  cpp-httplib already present, skipping.' }

# ---------------------------------------------------------------------------
# Create CMake config files for find_package
# ---------------------------------------------------------------------------
Write-Step 'Creating CMake config files for find_package'
$cmakeDir = Join-Path $DepsLocal 'lib\cmake'
New-Item -ItemType Directory -Force -Path $cmakeDir | Out-Null

# Helper: write text as UTF-8 without BOM (PS 5.1 -Encoding utf8 adds BOM).
function Write-FileNoBom([string]$path, [string]$content) {
    [System.IO.File]::WriteAllText($path, $content, (New-Object System.Text.UTF8Encoding $false))
}

# nlohmann_json
New-Item -ItemType Directory -Force -Path (Join-Path $cmakeDir 'nlohmann_json') | Out-Null
Write-FileNoBom (Join-Path $cmakeDir 'nlohmann_json\nlohmann_jsonConfig.cmake') @'
# nlohmann_jsonConfig.cmake -- header-only imported interface target
set(nlohmann_json_FOUND TRUE)
if(NOT TARGET nlohmann_json::nlohmann_json)
    add_library(nlohmann_json::nlohmann_json INTERFACE IMPORTED GLOBAL)
    set_target_properties(nlohmann_json::nlohmann_json PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}/../../../../include"
    )
endif()
'@

# httplib
New-Item -ItemType Directory -Force -Path (Join-Path $cmakeDir 'httplib') | Out-Null
Write-FileNoBom (Join-Path $cmakeDir 'httplib\httplibConfig.cmake') @'
# httplibConfig.cmake -- header-only imported interface target
set(httplib_FOUND TRUE)
set(httplib_VERSION "0.53.1")
set(HTTPLIB_IS_USING_BROTLI FALSE)
if(NOT TARGET httplib::httplib)
    add_library(httplib::httplib INTERFACE IMPORTED GLOBAL)
    set_target_properties(httplib::httplib PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}/../../../../include"
        INTERFACE_COMPILE_DEFINITIONS "CPPHTTPLIB_OPENSSL_SUPPORT"
    )
endif()
'@

# unofficial-inih
New-Item -ItemType Directory -Force -Path (Join-Path $cmakeDir 'unofficial-inih') | Out-Null
Write-FileNoBom (Join-Path $cmakeDir 'unofficial-inih\unofficial-inihConfig.cmake') @'
# unofficial-inihConfig.cmake -- static imported target for inih + INIReader
set(unofficial-inih_FOUND TRUE)
if(NOT TARGET unofficial::inih::inireader)
    add_library(unofficial::inih::inireader STATIC IMPORTED GLOBAL)
    set_target_properties(unofficial::inih::inireader PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/inih.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}/../../../../include"
    )
endif()
'@
Write-Host '  CMake config files written.'

# ---------------------------------------------------------------------------
# Build minio-cpp (static /MT)
# ---------------------------------------------------------------------------
Write-Step 'Building minio-cpp (static /MT)'
$minioBuild = Join-Path $DepsSrc 'minio-cpp-build'
if (-not (Test-Path (Join-Path $DepsLib 'minio.lib'))) {
    if (-not (Test-Path $MinioSrc)) { throw "minio-cpp source not found at $MinioSrc." }
    if (Test-Path $minioBuild) { Remove-Item $minioBuild -Recurse -Force }
    $prefixPaths = @($ZlibPrefix, $OpensslPrefix, $PugixmlPrefix, $DepsLocal) -join ';'
    Invoke-Checked $script:CMake @(
        '-S', $MinioSrc, '-B', $minioBuild,
        '-G', '"Visual Studio 17 2022"', '-A', 'x64',
        "-DCMAKE_PREFIX_PATH=$prefixPaths",
        '-DBUILD_SHARED_LIBS=OFF',
        '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded',
        '-DMINIO_CPP_TEST=OFF'
    )
    Invoke-Checked $script:CMake @('--build', $minioBuild, '--config', 'Release', '--', '/m')
    $minioLib = Join-Path $minioBuild 'Release\minio.lib'
    if (-not (Test-Path $minioLib)) { throw "minio.lib not found at $minioLib after build." }
    Copy-Item $minioLib $DepsLib -Force
    Write-Host '  Built and distributed minio-cpp.'
} else { Write-Host '  minio-cpp already built, skipping.' }

# ---------------------------------------------------------------------------
# Build CherryLips solution
# ---------------------------------------------------------------------------
Write-Step "Building CherryLips.sln ($Config | $Platform)"
$sln = Join-Path $RepoRoot 'CherryLips.sln'
Invoke-Checked $script:MSBuild @(
    $sln,
    "/t:Build",
    "/p:Configuration=$Config",
    "/p:Platform=$Platform",
    '/m',
    '/v:minimal'
)

# ---------------------------------------------------------------------------
# Done
# ---------------------------------------------------------------------------
$outDir = Join-Path $RepoRoot "$Platform\$Config"
Write-Host ""
Write-Host "Build complete. Output in $outDir" -ForegroundColor Green
Get-ChildItem $outDir -Filter 'CherryLips*' | Format-Table Name, Length -AutoSize
Write-Host ""
Write-Host "  DLL: $outDir\CherryLips.dll"
Write-Host "  EXE: $outDir\CherryLips-Test.exe"
