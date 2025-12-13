param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',

    [ValidateSet('x64', 'Win32')]
    [string]$Platform = 'x64',

    [ValidateSet('Build', 'Clean', 'Rebuild')]
    [string]$Target = 'Build'
)

$ErrorActionPreference = 'Stop'

$repoRoot     = $PSScriptRoot
$solutionPath = Join-Path $repoRoot 'TCDir.sln'

$toolsScript = Join-Path $repoRoot 'scripts\VSTools.ps1'
if (-not (Test-Path $toolsScript)) {
    throw "Tool helper script not found: $toolsScript"
}

. $toolsScript

if (-not (Test-Path $solutionPath)) {
    throw "Solution not found: $solutionPath"
}

$msbuildPath = Get-VS2026MSBuildPath
if (-not $msbuildPath) {
    $msbuildPath = Get-VS2026MSBuildPath -IncludePrerelease
}

if (-not $msbuildPath) {
    throw 'VS 2026 (v18.x) MSBuild not found (via vswhere). Install VS 2026 with MSBuild.'
}

$msbuildArgs = @(
    $solutionPath,
    "-p:Configuration=$Configuration",
    "-p:Platform=$Platform"
)

if ($Target -ne 'Build') {
    $msbuildArgs += "-t:$Target"
}

Write-Host "Using MSBuild: $msbuildPath"
Write-Host "Building: $solutionPath ($Configuration|$Platform) Target=$Target"

& $msbuildPath @msbuildArgs
exit $LASTEXITCODE
