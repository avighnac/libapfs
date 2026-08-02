$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"

$installation = & $vswhere `
    -latest `
    -products * `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -format json |
    ConvertFrom-Json |
    Select-Object -First 1

if (-not $installation) {
    throw "No Visual Studio installation with x64 C++ tools found."
}

$major = [int]($installation.installationVersion.Split('.')[0])

if ($installation.displayName -notmatch '(\d{4})') {
    throw "Could not determine Visual Studio year from: $($installation.displayName)"
}

$year = $Matches[1]
$generator = "Visual Studio $major $year"

Write-Host "Using generator: $generator"

cmake -S . -B build `
    -G $generator `
    -A x64 `
    -DBUILD_STATIC=True `
    -DBUILD_CLI=True

if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

cmake --build build `
    --config Debug `
    --parallel `
    -- `
    /p:WindowsTargetPlatformVersion=10.0

exit $LASTEXITCODE