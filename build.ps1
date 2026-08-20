#!/usr/bin/env pwsh
#
# Build the EnhancedInput plugin against a Unigine SDK -- no empty_cpp project.
# The plugin is configured standalone from source/plugins/Ryutp/EnhancedInput;
# the engine is referenced from the SDK and the .dll/.so files land in
# <repo>/bin/plugins/Ryutp/EnhancedInput.
#
# Builds every combination: Debug/Release x float/double.
# Generator Ninja, all cores, always-clean, fail fast.
#
# SDK resolution (first hit wins):
#   1. $env:UNIGINE_SDK_PATH (a valid SDK dir)
#   2. -Sdk <id|path>
#   3. SDK Browser registry: the only installed SDK, else interactive picker.
#
# Editor plugin needs Qt 6.5.3 -- $env:UNIGINE_QTROOT must point at it. The x64
# MSVC toolchain (cl.exe, ninja) is pulled in automatically from the newest VS.

[CmdletBinding()]
param([string]$Sdk)

$ErrorActionPreference = 'Stop'

$RepoDir   = $PSScriptRoot
$PluginSrc = Join-Path $RepoDir 'source\plugins\Ryutp\EnhancedInput'
$BuildRoot = Join-Path $RepoDir 'build'

function Test-Sdk($p) { $p -and (Test-Path -LiteralPath (Join-Path $p 'include\UnigineEngine.h')) }

function Find-BrowserJson {
	$c = @()
	if ($env:APPDATA)         { $c += Join-Path $env:APPDATA 'unigine\browser.json' }
	if ($env:LOCALAPPDATA)    { $c += Join-Path $env:LOCALAPPDATA 'unigine\browser.json' }
	if ($env:XDG_CONFIG_HOME) { $c += Join-Path $env:XDG_CONFIG_HOME 'unigine/browser.json' }
	$c += Join-Path $HOME '.config/unigine/browser.json'
	$c | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
}

function Resolve-SdkPath {
	if (Test-Sdk $env:UNIGINE_SDK_PATH) { return $env:UNIGINE_SDK_PATH.TrimEnd('\', '/') }
	if (Test-Sdk $Sdk)                  { return $Sdk.TrimEnd('\', '/') }

	$bj = Find-BrowserJson
	if (-not $bj) { throw "No SDK: set `$env:UNIGINE_SDK_PATH, pass -Sdk <dir>, or install one in the SDK Browser." }
	$data = Get-Content -LiteralPath $bj -Raw | ConvertFrom-Json
	$installed = $data.sdk.installed
	$default   = $data.sdk.default
	$ids = @($installed.PSObject.Properties.Name |
		Sort-Object @{ Expression = { $_ -ne $default } }, @{ Expression = { $_ } })
	if ($ids.Count -eq 0) { throw "No installed SDKs in $bj." }
	if ($Sdk) {
		$p = $installed.$Sdk
		if (-not $p) { throw "-Sdk '$Sdk' not found. Installed: $($ids -join ', ')" }
		return $p.TrimEnd('\', '/')
	}
	if ($ids.Count -eq 1) { return $installed.($ids[0]).TrimEnd('\', '/') }
	Write-Host "Select SDK:"
	for ($i = 0; $i -lt $ids.Count; $i++) {
		$leaf = Split-Path -Leaf $installed.($ids[$i])
		Write-Host "  $($i + 1)) $($ids[$i])  ($leaf)"
	}
	$sel = Read-Host "SDK [1]"; if (-not $sel) { $sel = 1 }
	$idx = [int]$sel - 1
	if ($idx -lt 0 -or $idx -ge $ids.Count) { throw "Invalid selection." }
	return $installed.($ids[$idx]).TrimEnd('\', '/')
}

function Find-VsWhere {
	foreach ($root in ${env:ProgramFiles(x86)}, $env:ProgramFiles) {
		if (-not $root) { continue }
		$p = Join-Path $root 'Microsoft Visual Studio\Installer\vswhere.exe'
		if (Test-Path -LiteralPath $p) { return $p }
	}
}

# CMake needs cl.exe and ninja on PATH; a plain PowerShell session has neither.
# Pull the x64 MSVC environment in ourselves rather than making the user start
# from a "x64 Native Tools Command Prompt for VS".
function Initialize-BuildTools {
	$vswhere = Find-VsWhere
	$vsPath = if ($vswhere) {
		& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
			-property installationPath | Select-Object -First 1
	}

	if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
		if (-not $vsPath) { throw "No Visual Studio with the C++ toolset (Microsoft.VisualStudio.Component.VC.Tools.x86.x64) -- install it, or run from a 'x64 Native Tools Command Prompt for VS'." }
		$devShell = Join-Path $vsPath 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll'
		if (-not (Test-Path -LiteralPath $devShell)) { throw "Missing '$devShell'; run from a 'x64 Native Tools Command Prompt for VS' instead." }
		Import-Module $devShell
		# VsDevCmd.bat internals call bare vswhere.exe, so it must be on PATH.
		$env:PATH = "$(Split-Path -Parent $vswhere);$env:PATH"
		Enter-VsDevShell -VsInstallPath $vsPath -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64' | Out-Null
		Write-Host "MSVC: $vsPath (x64)"
	}

	# Vars set via setx or the System dialog never reach an already-running
	# shell, so read UNIGINE_QTROOT back from the registry instead of making
	# the user restart the terminal.
	if (-not ($env:UNIGINE_QTROOT -and (Test-Path -LiteralPath $env:UNIGINE_QTROOT))) {
		foreach ($scope in 'User', 'Machine') {
			$qt = [Environment]::GetEnvironmentVariable('UNIGINE_QTROOT', $scope)
			if ($qt -and (Test-Path -LiteralPath $qt)) { $env:UNIGINE_QTROOT = $qt; break }
		}
		if ($env:UNIGINE_QTROOT) { Write-Host "Qt: $env:UNIGINE_QTROOT" }
		else { Write-Warning "UNIGINE_QTROOT is unset -- the editor plugin needs Qt 6.5.3." }
	}

	if (-not (Get-Command ninja.exe -ErrorAction SilentlyContinue)) {
		$ninjaDir = if ($vsPath) { Join-Path $vsPath 'Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja' }
		if (-not ($ninjaDir -and (Test-Path -LiteralPath (Join-Path $ninjaDir 'ninja.exe')))) {
			throw "Ninja not on PATH -- install it, or add the 'C++ CMake tools for Windows' VS component."
		}
		$env:PATH = "$ninjaDir;$env:PATH"
		Write-Host "Ninja: $ninjaDir"
	}
}

$SdkPath = Resolve-SdkPath
if (-not (Test-Sdk $SdkPath)) { throw "'$SdkPath' is not a valid SDK." }
$SdkFwd  = $SdkPath -replace '\\', '/'
$RepoFwd = $RepoDir -replace '\\', '/'
Write-Host "SDK: $SdkPath"
Initialize-BuildTools

$configs = @(
	@{ type = 'Debug';   precision = 'double'; dbl = 1 },
	@{ type = 'Release'; precision = 'double'; dbl = 1 },
	@{ type = 'Debug';   precision = 'float';  dbl = 0 },
	@{ type = 'Release'; precision = 'float';  dbl = 0 }
)

foreach ($c in $configs) {
	$buildDir = Join-Path $BuildRoot "$($c.type)-$($c.precision)"

	Write-Host '=============================================================='
	Write-Host " $($c.type) / $($c.precision)"
	Write-Host '=============================================================='

	if (Test-Path -LiteralPath $buildDir) { Remove-Item -Recurse -Force -LiteralPath $buildDir }

	& cmake -S $PluginSrc -B $buildDir -G Ninja `
		"-DCMAKE_BUILD_TYPE=$($c.type)" `
		"-DUNIGINE_DOUBLE=$($c.dbl)" `
		"-DUNIGINE_SDK_PATH=$SdkFwd/" `
		"-DUNIGINE_BIN_DIR=$RepoFwd/bin" `
		"-DUNIGINE_LIB_DIR=$SdkFwd/lib" `
		"-DUNIGINE_INCLUDE_DIR=$SdkFwd/include;$RepoFwd/include"
	if ($LASTEXITCODE -ne 0) { throw "configure failed ($($c.type)/$($c.precision))" }

	& cmake --build $buildDir
	if ($LASTEXITCODE -ne 0) { throw "build failed ($($c.type)/$($c.precision))" }
}

Write-Host "All builds succeeded. Plugins -> $RepoDir\bin\plugins\Ryutp\EnhancedInput"
