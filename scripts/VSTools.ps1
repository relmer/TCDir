$script:VS2026VersionRange = '[18.0,19.0)'





function Get-VSWherePath {
    $pf86 = [Environment]::GetFolderPath('ProgramFilesX86')
    $path = Join-Path $pf86 'Microsoft Visual Studio\Installer\vswhere.exe'

    if (-not (Test-Path $path)) {
        throw "vswhere.exe not found at: $path"
    }

    return $path
}





function Get-VS2026ToolPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Find,
        [string[]]$Requires,
        [switch]$RequiresAny,
        [switch]$IncludePrerelease
    )

    $vswhere = Get-VSWherePath

    $vswhereArgs = @(
        '-latest',
        '-products', '*',
        '-version', $script:VS2026VersionRange
    )

    if ($IncludePrerelease) {
        $vswhereArgs = @('-prerelease') + $vswhereArgs
    }

    if ($Requires -and $Requires.Count -gt 0) {
        $vswhereArgs += '-requires'
        $vswhereArgs += $Requires

        if ($RequiresAny) {
            $vswhereArgs += '-requiresAny'
        }
    }

    $vswhereArgs += '-find'
    $vswhereArgs += $Find

    $toolPath = & $vswhere @vswhereArgs | Select-Object -First 1
    return $toolPath
}





function Get-VS2026MSBuildPath {
    param(
        [switch]$IncludePrerelease
    )

    return Get-VS2026ToolPath -Find 'MSBuild\Current\Bin\MSBuild.exe' -Requires 'Microsoft.Component.MSBuild' -IncludePrerelease:$IncludePrerelease
}





function Get-VS2026VSTestPath {
    param(
        [switch]$IncludePrerelease
    )

    $testRunner = Get-VS2026ToolPath -Find 'Common7\IDE\Extensions\TestPlatform\vstest.console.exe' -IncludePrerelease:$IncludePrerelease
    if ($testRunner) {
        return $testRunner
    }

    $testRunner = Get-VS2026ToolPath -Find 'Common7\IDE\CommonExtensions\Microsoft\TestWindow\vstest.console.exe' -IncludePrerelease:$IncludePrerelease
    return $testRunner
}
