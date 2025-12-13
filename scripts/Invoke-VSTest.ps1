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



$testRunner = Get-VS2026VSTestPath
if (-not $testRunner) {
    $testRunner = Get-VS2026VSTestPath -IncludePrerelease
}

if (-not $testRunner) {
    throw 'VS 2026 (v18.x) test runner not found (via vswhere).'
}

& $testRunner $TestDll @VSTestArgs
exit $LASTEXITCODE
