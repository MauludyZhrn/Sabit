# ====================================================================
# SABIT - Windows Installer Builder Script (Inno Setup 7)
# ====================================================================

$ErrorActionPreference = "Stop"

$ProjectRoot = Resolve-Path "$PSScriptRoot\.."
$ISCCPath = "C:\Program Files\Inno Setup 7\ISCC.exe"
if (-not (Test-Path $ISCCPath)) {
    $ISCCPath = (Get-Command iscc.exe -ErrorAction SilentlyContinue).Source
}

if (-not $ISCCPath -or -not (Test-Path $ISCCPath)) {
    Write-Error "Inno Setup 7 compiler (ISCC.exe) was not found! Install via: winget install --id JRSoftware.InnoSetup.7 -e -s winget -i"
    exit 1
}

$PackageDir = Join-Path $ProjectRoot "dist\package\Sabit"
$Executable = Join-Path $PackageDir "Sabit.exe"
if (-not (Test-Path $Executable)) {
    Write-Error "Deployment package not found at $Executable! Please run release build and dependency deployment first."
    exit 1
}

$IssFile = Join-Path $PSScriptRoot "Sabit.iss"
Write-Host ">>> Compiling SABIT Windows Installer using Inno Setup 7 <<<" -ForegroundColor Cyan
Write-Host "  ISCC: $ISCCPath"
Write-Host "  Script: $IssFile"

& $ISCCPath "$IssFile"

if ($LASTEXITCODE -ne 0) {
    Write-Error "Inno Setup compilation failed with exit code $LASTEXITCODE"
    exit $LASTEXITCODE
}

$InstallerPath = Join-Path $ProjectRoot "dist\Sabit-Setup-v1.0.0-x64.exe"
if (Test-Path $InstallerPath) {
    $Item = Get-Item $InstallerPath
    $Hash = (Get-FileHash -Algorithm SHA256 $InstallerPath).Hash
    Write-Host "`n>>> Installer Generated Successfully! <<<" -ForegroundColor Green
    Write-Host "  Filename: $($Item.Name)"
    Write-Host "  Size: $([math]::Round($Item.Length / 1MB, 2)) MB ($($Item.Length) bytes)"
    Write-Host "  SHA-256: $Hash"
} else {
    Write-Error "Expected installer not found at $InstallerPath"
    exit 1
}
