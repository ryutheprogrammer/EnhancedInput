#!/usr/bin/env pwsh
#
# Build-machine project: a minimal Unigine empty_cpp source tree that REFERENCES
# a chosen installed SDK and carries the EnhancedInput plugin. Compiles only --
# no engine data, no .umount, no launch scripts (the artifact is the plugin .so;
# the project is never run here).
#
#   .\make_project.ps1 <target_dir> [-Sdk <id>] [-From <seed_project>]
#                                   [-NoPull] [-NoBuild]
#
#   - SDK is taken from the SDK Browser registry (browser.json -> sdk.installed):
#       -Sdk <id> | the only installed SDK | interactive picker when several.
#   - only source/ is cloned (app skeleton + cmake modules); engine INCLUDE+LIBS
#     are referenced via UNIGINE_SDK_PATH in CMakeLists. Nothing heavy copied.
#   - plugin sources are refreshed (git pull) then overlaid; build via build.ps1.
#   - on an EXISTING target: the source skeleton is kept; only the SDK reference
#     and the plugin are refreshed.

[CmdletBinding()]
param(
	[Parameter(Mandatory = $true, Position = 0)]
	[string]$Target,
	[string]$Sdk,
	[string]$From,
	[switch]$NoPull,
	[switch]$NoBuild
)

$ErrorActionPreference = 'Stop'

$RepoDir     = $PSScriptRoot
$PluginRel   = 'plugins\Ryutp\EnhancedInput'
$PluginCMake = $PluginRel -replace '\\', '/'

function Find-BrowserJson {
	$cands = @()
	if ($env:APPDATA)         { $cands += Join-Path $env:APPDATA 'unigine\browser.json' }
	if ($env:LOCALAPPDATA)    { $cands += Join-Path $env:LOCALAPPDATA 'unigine\browser.json' }
	if ($env:XDG_CONFIG_HOME) { $cands += Join-Path $env:XDG_CONFIG_HOME 'unigine/browser.json' }
	$cands += Join-Path $HOME '.config/unigine/browser.json'
	$cands | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
}

$bj = Find-BrowserJson
if (-not $bj) { throw "SDK Browser registry (browser.json) not found." }
$data = Get-Content -LiteralPath $bj -Raw | ConvertFrom-Json

# --- resolve SDK (id + path) ---
$installed = $data.sdk.installed
$default   = $data.sdk.default
$ids = @($installed.PSObject.Properties.Name |
	Sort-Object @{ Expression = { $_ -ne $default } }, @{ Expression = { $_ } })
if ($ids.Count -eq 0) { throw "No installed SDKs in $bj." }

if ($Sdk) {
	$SdkId = $Sdk
	$SdkPath = $installed.$Sdk
	if (-not $SdkPath) { throw "-Sdk '$Sdk' not installed. Available: $($ids -join ', ')" }
}
elseif ($ids.Count -eq 1) {
	$SdkId = $ids[0]; $SdkPath = $installed.($ids[0])
}
else {
	Write-Host "Select SDK:"
	for ($i = 0; $i -lt $ids.Count; $i++) { Write-Host "  $($i + 1)) $($ids[$i])" }
	$sel = Read-Host "SDK [1]"; if (-not $sel) { $sel = 1 }
	$idx = [int]$sel - 1
	if ($idx -lt 0 -or $idx -ge $ids.Count) { throw "Invalid selection." }
	$SdkId = $ids[$idx]; $SdkPath = $installed.($ids[$idx])
}
if (-not (Test-Path -LiteralPath (Join-Path $SdkPath 'include\UnigineEngine.h'))) {
	throw "'$SdkPath' is not a valid SDK (no include/UnigineEngine.h)."
}
$SdkFwd = $SdkPath -replace '\\', '/'
Write-Host "== SDK:     $SdkId"
Write-Host "            $SdkPath"

foreach ($d in @("source\$PluginRel", "include\$PluginRel", "bin\$PluginRel")) {
	if (-not (Test-Path -LiteralPath (Join-Path $RepoDir $d))) { throw "Deliverable missing: $d" }
}

# --- skeleton: clone only source/ (fresh target; engine is referenced) ---
if (-not (Test-Path -LiteralPath $Target)) {
	function Find-Seed {
		foreach ($p in $data.projects) {
			$dir = Split-Path -Parent $p.filepath
			if ((Split-Path -Leaf $dir) -eq 'empty_cpp' -and
				(Test-Path -LiteralPath (Join-Path $dir 'source\CMakeLists.txt'))) { return $dir }
		}
		return $null
	}
	$Seed = if ($From) { $From } else { Find-Seed }
	if (-not $Seed) { throw "No 'empty_cpp' seed project in the registry; pass -From <project_dir>." }
	if (-not (Test-Path -LiteralPath (Join-Path $Seed 'source'))) { throw "'$Seed' is not an empty_cpp project." }
	Write-Host "== seed:    $Seed (source/ only)"
	$rc = @(
		(Join-Path $Seed 'source'), (Join-Path $Target 'source'), '/E',
		'/XD', (Join-Path $Seed 'source\build'),
		'/NFL', '/NDL', '/NJH', '/NJS', '/NP'
	)
	& robocopy @rc | Out-Null
	if ($LASTEXITCODE -ge 8) { throw "robocopy failed ($LASTEXITCODE)." }
	$global:LASTEXITCODE = 0
	New-Item -ItemType Directory -Force (Join-Path $Target 'bin') | Out-Null
}
else {
	Write-Host "== existing target: keeping source skeleton, refreshing SDK ref + plugin"
}

# --- patch source/CMakeLists.txt ---
Write-Host "== patch:   source/CMakeLists.txt"
$cml   = Join-Path $Target 'source\CMakeLists.txt'
$lines = Get-Content -LiteralPath $cml | ForEach-Object {
	if ($_ -match '^set\(UNIGINE_SDK_PATH ') { "set(UNIGINE_SDK_PATH $SdkFwd/)" }
	elseif ($_ -match '^set\(UNIGINE_DOUBLE ') { 'if(NOT DEFINED UNIGINE_DOUBLE)'; "`tset(UNIGINE_DOUBLE 1)"; 'endif()' }
	else { $_ }
}
if (-not ($lines -match ':_double>\)')) {
	$lines = $lines | ForEach-Object {
		if ($_ -match 'string\(APPEND binary_name "_x64"\)') {
			'string(APPEND binary_name $<$<BOOL:${UNIGINE_DOUBLE}>:_double>)'
		}
		$_
	}
}
if (-not ($lines -match [regex]::Escape("add_subdirectory($PluginCMake)"))) {
	$lines += ''; $lines += "    add_subdirectory($PluginCMake)"
}
Set-Content -LiteralPath $cml -Value $lines

# --- plugin: pull then overlay ---
if (-not $NoPull) {
	Write-Host "== pull:    $RepoDir"
	& git -C $RepoDir pull --ff-only
	if ($LASTEXITCODE -ne 0) { Write-Warning "git pull failed; overlaying current working tree." }
}
Write-Host "== overlay: EnhancedInput plugin"
foreach ($d in @("source\$PluginRel", "include\$PluginRel", "bin\$PluginRel")) {
	$dst = Join-Path $Target $d
	if (Test-Path -LiteralPath $dst) { Remove-Item -Recurse -Force -LiteralPath $dst }
	New-Item -ItemType Directory -Force $dst | Out-Null
	Copy-Item -Path (Join-Path (Join-Path $RepoDir $d) '*') -Destination $dst -Recurse -Force
}
Copy-Item -Path (Join-Path $RepoDir 'build.sh'), (Join-Path $RepoDir 'build.ps1') -Destination $Target -Force

# --- build ---
if (-not $NoBuild) {
	Write-Host "== build:   $Target"
	Push-Location $Target
	try { & (Join-Path $Target 'build.ps1') }   # throws on failure -> propagates
	finally { Pop-Location }
}

Write-Host "Project ready: $Target  (SDK: $SdkId)"
