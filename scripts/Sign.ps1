<#
.SYNOPSIS
    Signs TCDir executables using Azure Artifact Signing.

.DESCRIPTION
    Signs one or more executables using SignTool with the Azure Artifact
    Signing dlib.  Requires an authenticated Azure session (az login) and
    the Artifact Signing Certificate Profile Signer RBAC role.

.PARAMETER Files
    One or more file paths to sign.  Defaults to the x64 and ARM64
    Release builds.

.PARAMETER Configuration
    Build configuration whose outputs to sign.  Default: Release.

.PARAMETER Verify
    Verify signatures after signing.

.EXAMPLE
    .\scripts\Sign.ps1
    .\scripts\Sign.ps1 -Verify
    .\scripts\Sign.ps1 -Files "x64\Release\TCDir.exe"
#>

[CmdletBinding()]
param(
    [string[]]$Files,

    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',

    [switch]$Verify
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path $PSScriptRoot -Parent

# ── Locate tools ──────────────────────────────────────────────

# SignTool — latest Windows SDK
$signToolDir = Get-ChildItem "C:\Program Files (x86)\Windows Kits\10\bin\10.*\x64\signtool.exe" -ErrorAction SilentlyContinue |
    Sort-Object FullName -Descending |
    Select-Object -First 1
if (-not $signToolDir) {
    Write-Error "signtool.exe not found.  Install the Windows SDK."
    exit 1
}
$signTool = $signToolDir.FullName

# Artifact Signing dlib — NuGet package cache
$dlib = Get-ChildItem "$env:USERPROFILE\.nuget\packages\microsoft.trusted.signing.client" -Directory -ErrorAction SilentlyContinue |
    Sort-Object Name -Descending |
    Select-Object -First 1
if (-not $dlib) {
    Write-Error @"
Azure.CodeSigning.Dlib.dll not found.

Install it with:
    dotnet new console -o `$env:TEMP\ts-helper --force
    Push-Location `$env:TEMP\ts-helper
    dotnet add package Microsoft.Trusted.Signing.Client
    Pop-Location
"@
    exit 1
}
$dlibPath = Join-Path $dlib.FullName "bin\x64\Azure.CodeSigning.Dlib.dll"
if (-not (Test-Path $dlibPath)) {
    Write-Error "dlib not found at expected path: $dlibPath"
    exit 1
}

# Signing metadata config
$configPath = Join-Path $PSScriptRoot "signing-config.json"
if (-not (Test-Path $configPath)) {
    Write-Error "signing-config.json not found at: $configPath"
    exit 1
}

# ── Determine files to sign ───────────────────────────────────

if (-not $Files) {
    $Files = @(
        (Join-Path $repoRoot "x64\$Configuration\TCDir.exe"),
        (Join-Path $repoRoot "ARM64\$Configuration\TCDir.exe")
    )
}

$toSign = @()
foreach ($f in $Files) {
    if (Test-Path $f) {
        $toSign += (Resolve-Path $f).Path
    }
    else {
        Write-Warning "File not found, skipping: $f"
    }
}

if ($toSign.Count -eq 0) {
    Write-Error "No files found to sign."
    exit 1
}

# ── Sign ──────────────────────────────────────────────────────

Write-Host "Signing $($toSign.Count) file(s)..." -ForegroundColor Cyan
Write-Host "  SignTool: $signTool"
Write-Host "  Dlib:     $dlibPath"
Write-Host "  Config:   $configPath"
Write-Host ""

foreach ($file in $toSign) {
    Write-Host "  Signing: $file" -ForegroundColor Yellow

    & $signTool sign `
        /v /fd SHA256 `
        /tr "http://timestamp.acs.microsoft.com" /td SHA256 `
        /dlib $dlibPath `
        /dmdf $configPath `
        $file

    if ($LASTEXITCODE -ne 0) {
        Write-Error "SignTool failed for: $file (exit code $LASTEXITCODE)"
        exit $LASTEXITCODE
    }

    Write-Host "  Signed:  $file" -ForegroundColor Green
    Write-Host ""
}

# ── Verify (optional) ────────────────────────────────────────

if ($Verify) {
    Write-Host "Verifying signatures..." -ForegroundColor Cyan

    foreach ($file in $toSign) {
        Write-Host "  Verifying: $file" -ForegroundColor Yellow

        & $signTool verify /pa /v $file

        if ($LASTEXITCODE -ne 0) {
            Write-Error "Signature verification failed for: $file"
            exit $LASTEXITCODE
        }

        Write-Host "  Verified:  $file" -ForegroundColor Green
        Write-Host ""
    }
}

Write-Host "Done: $($toSign.Count) file(s) signed." -ForegroundColor Cyan
