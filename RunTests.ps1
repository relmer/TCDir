# Simple PowerShell script to run the unit tests
Write-Host "Looking for UnitTest.dll..."

# Find the most recent UnitTest.dll
$dlls = Get-ChildItem -Path . -Recurse -Filter "UnitTest.dll" | Sort-Object LastWriteTime -Descending
if ($dlls.Count -eq 0) {
    Write-Host "No UnitTest.dll found!"
    exit 1
}

$latestDll = $dlls[0]
Write-Host "Found latest DLL: $($latestDll.FullName)"
Write-Host "Last modified: $($latestDll.LastWriteTime)"

# Try to run the tests
$toolsScript = Join-Path $PSScriptRoot 'scripts\VSTools.ps1'
if (-not (Test-Path $toolsScript)) {
    Write-Host "Tool helper script not found: $toolsScript"
    exit 1
}

. $toolsScript



$testRunner = Get-VS2026VSTestPath
if (-not $testRunner) {
    $testRunner = Get-VS2026VSTestPath -IncludePrerelease
}

if ($testRunner) {
    Write-Host "Running tests with: $testRunner"
    & $testRunner $latestDll.FullName
} else {
    Write-Host "VS 2026 (v18.x) test runner not found (via vswhere)."
    exit 1
}
