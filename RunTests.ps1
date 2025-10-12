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
$testRunner = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\Extensions\TestPlatform\vstest.console.exe"
if (Test-Path $testRunner) {
    Write-Host "Running tests with: $testRunner"
    & $testRunner $latestDll.FullName
} else {
    Write-Host "Test runner not found at: $testRunner"
    exit 1
}
