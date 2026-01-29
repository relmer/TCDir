param(
    [Parameter(Mandatory = $true)]
    [string]$TestDll,

    [string[]]$VSTestArgs = @('/logger:trx')
)

$ErrorActionPreference = 'Stop'



$toolsScript = Join-Path $PSScriptRoot 'VSTools.ps1'
if (-not (Test-Path $toolsScript)) {
    throw "Tool helper script not found: $toolsScript"
}

. $toolsScript

if (-not (Test-Path $TestDll)) {
    throw "Test DLL not found: $TestDll"
}

# Detect if we're trying to run ARM64 tests on an x64 host (not supported)
$hostArch = [System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture
$isArm64Test = $TestDll -match '(?i)\\ARM64\\'

if ($isArm64Test -and $hostArch -ne [System.Runtime.InteropServices.Architecture]::Arm64) {
    Write-Host "Skipping ARM64 tests on $hostArch host (cross-arch execution not supported)" -ForegroundColor Yellow
    exit 0
}

$platform = 'Any'
if ($isArm64Test) {
    $platform = 'ARM64'
}



$testRunner = Get-VS2026VSTestPath -Platform $platform
if (-not $testRunner) {
    $testRunner = Get-VS2026VSTestPath -Platform $platform -IncludePrerelease
}

if (-not $testRunner) {
    $testRunner = Get-VS2026VSTestPath
}

if (-not $testRunner) {
    $testRunner = Get-VS2026VSTestPath -IncludePrerelease
}

if (-not $testRunner) {
    $testRunner = Get-VS2026VSTestPath -Platform $platform
}

if (-not $testRunner) {
    throw 'VS 2026 (v18.x) test runner not found (via vswhere).'
}

& $testRunner $TestDll @VSTestArgs
exit $LASTEXITCODE
