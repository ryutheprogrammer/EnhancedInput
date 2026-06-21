#!/usr/bin/env pwsh
#
# Configure & build every combination of:
#   build type : Debug, Release
#   precision  : float (UNIGINE_DOUBLE=0), double (UNIGINE_DOUBLE=1)
#
# - Generator: Ninja
# - Separate build directory per combination
# - All cores (Ninja default)
# - Always clean: each build directory is wiped before configuring
# - Fail fast: the first failed configure/build aborts the whole run
#
# Run from a "x64 Native Tools Command Prompt for VS" (or after vcvarsall) so
# cl.exe is on PATH for the Ninja generator.

$ErrorActionPreference = 'Stop'

$SrcDir    = Join-Path $PSScriptRoot 'source'
$BuildRoot = Join-Path $SrcDir 'build'

$configs = @(
	@{ type = 'Debug';   precision = 'double'; dbl = 1 },
	@{ type = 'Release'; precision = 'double'; dbl = 1 },
	@{ type = 'Debug';   precision = 'float';  dbl = 0 },
	@{ type = 'Release'; precision = 'float';  dbl = 0 }
)

foreach ($c in $configs) {
	$buildDir = Join-Path $BuildRoot "$($c.type)-$($c.precision)"

	Write-Host '=============================================================='
	Write-Host " $($c.type) / $($c.precision)  ->  $buildDir"
	Write-Host '=============================================================='

	if (Test-Path -LiteralPath $buildDir) { Remove-Item -Recurse -Force -LiteralPath $buildDir }

	& cmake -S $SrcDir -B $buildDir -G Ninja -DCMAKE_BUILD_TYPE=$($c.type) -DUNIGINE_DOUBLE=$($c.dbl)
	if ($LASTEXITCODE -ne 0) { throw "configure failed ($($c.type)/$($c.precision))" }

	& cmake --build $buildDir
	if ($LASTEXITCODE -ne 0) { throw "build failed ($($c.type)/$($c.precision))" }
}

Write-Host 'All builds succeeded.'
