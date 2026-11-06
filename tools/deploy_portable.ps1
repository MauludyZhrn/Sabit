# Sabit 1.0 Portable Deployment Script
# Creates a standalone, zero-admin portable distribution in dist/Sabit/
# Compatible with FR-36 & NFR-09

$ErrorActionPreference = "Stop"

$ProjectRoot = Resolve-Path "$PSScriptRoot\.."
$DistDir = Join-Path $ProjectRoot "dist\Sabit"
$BuildDir = Join-Path $ProjectRoot "build"
$MinGWBin = "D:\mingw64\bin"
$QtBin = "D:\Qt\6.8.0\mingw_64\bin"
$QtPlugins = "D:\Qt\6.8.0\mingw_64\plugins"

Write-Host ">>> Assembling Sabit Portable Distribution in $DistDir <<<" -ForegroundColor Cyan

# 1. Clean & create directory tree
if (Test-Path $DistDir) {
    Remove-Item -Recurse -Force $DistDir
}
New-Item -ItemType Directory -Path "$DistDir\platforms" -Force | Out-Null
New-Item -ItemType Directory -Path "$DistDir\styles" -Force | Out-Null
New-Item -ItemType Directory -Path "$DistDir\imageformats" -Force | Out-Null
New-Item -ItemType Directory -Path "$DistDir\models" -Force | Out-Null
New-Item -ItemType Directory -Path "$DistDir\assets\icons" -Force | Out-Null
New-Item -ItemType Directory -Path "$DistDir\config" -Force | Out-Null
New-Item -ItemType Directory -Path "$DistDir\src\ui\theme" -Force | Out-Null

# 2. Main Executable
Write-Host "  Copying Sabit.exe..."
Copy-Item "$BuildDir\Sabit.exe" "$DistDir\Sabit.exe"

# 3. MinGW C++ Runtime Libraries
Write-Host "  Copying MinGW runtime DLLs..."
Copy-Item "$MinGWBin\libwinpthread-1.dll" "$DistDir\"
Copy-Item "$MinGWBin\libgcc_s_seh-1.dll" "$DistDir\"
Copy-Item "$MinGWBin\libstdc++-6.dll" "$DistDir\"

# 4. Qt6 Core/Gui/Widgets/Svg Libraries
Write-Host "  Copying Qt6 core libraries..."
Copy-Item "$QtBin\Qt6Core.dll" "$DistDir\"
Copy-Item "$QtBin\Qt6Gui.dll" "$DistDir\"
Copy-Item "$QtBin\Qt6Widgets.dll" "$DistDir\"
if (Test-Path "$QtBin\Qt6Svg.dll") {
    Copy-Item "$QtBin\Qt6Svg.dll" "$DistDir\"
}

# 5. Qt Plugins
Write-Host "  Copying Qt plugins..."
Copy-Item "$QtPlugins\platforms\qwindows.dll" "$DistDir\platforms\"
Copy-Item "$QtPlugins\platforms\qoffscreen.dll" "$DistDir\platforms\"
if (Test-Path "$QtPlugins\styles\qmodernwindowsstyle.dll") {
    Copy-Item "$QtPlugins\styles\qmodernwindowsstyle.dll" "$DistDir\styles\"
}
Copy-Item "$QtPlugins\imageformats\qjpeg.dll" "$DistDir\imageformats\"
Copy-Item "$QtPlugins\imageformats\qico.dll" "$DistDir\imageformats\"
if (Test-Path "$QtPlugins\imageformats\qgif.dll") {
    Copy-Item "$QtPlugins\imageformats\qgif.dll" "$DistDir\imageformats\"
}
if (Test-Path "$QtPlugins\imageformats\qsvg.dll") {
    Copy-Item "$QtPlugins\imageformats\qsvg.dll" "$DistDir\imageformats\"
}

# 6. Hardware Camera SDKs & AI Runtime
Write-Host "  Copying Camera & AI Runtime DLLs..."
if (Test-Path "$ProjectRoot\vendor\zwo\ASICamera2.dll") {
    Copy-Item "$ProjectRoot\vendor\zwo\ASICamera2.dll" "$DistDir\"
}
if (Test-Path "C:\Windows\System32\onnxruntime.dll") {
    Copy-Item "C:\Windows\System32\onnxruntime.dll" "$DistDir\"
}

# 7. AI Models & Manifest
Write-Host "  Copying Hilal AI model & manifest..."
Copy-Item "$ProjectRoot\models\hilal_v1.onnx" "$DistDir\models\"
Copy-Item "$ProjectRoot\models\manifest.json" "$DistDir\models\"

# 8. Icons & Assets
Write-Host "  Copying UI icons & themes..."
if (Test-Path "$ProjectRoot\assets\icons") {
    Copy-Item "$ProjectRoot\assets\icons\*" "$DistDir\assets\icons\" -Recurse
}
if (Test-Path "$ProjectRoot\assets\logo.ico") { Copy-Item "$ProjectRoot\assets\logo.ico" "$DistDir\assets\" }
if (Test-Path "$ProjectRoot\assets\logo.png") { Copy-Item "$ProjectRoot\assets\logo.png" "$DistDir\assets\" }
if (Test-Path "$ProjectRoot\assets\splash.png") { Copy-Item "$ProjectRoot\assets\splash.png" "$DistDir\assets\" }
Copy-Item "$ProjectRoot\src\ui\theme\tokens.json" "$DistDir\src\ui\theme\tokens.json"
Copy-Item "$ProjectRoot\src\ui\theme\tokens.json" "$DistDir\tokens.json"

# 9. Default Configuration
Write-Host "  Creating default settings.json..."
$DefaultSettings = @"
{
    "schema_version": 2,
    "general": {
        "theme": "Dark",
        "language": "id",
        "observer_name": "Sabit Observer",
        "default_location": "Semarang"
    },
    "camera": {
        "default_driver": "sim",
        "default_exposure_ms": 33.3,
        "default_gain": 100.0,
        "default_binning": 1,
        "auto_reconnect": true
    },
    "imaging": {
        "default_stack_method": "Mean",
        "stack_ram_mb": 512,
        "auto_stretch": true,
        "clahe": false,
        "sharpen": false,
        "invert": false
    },
    "ai": {
        "auto_init": true,
        "conf_threshold": 0.25,
        "nms_threshold": 0.45,
        "max_fps": 3
    },
    "recording": {
        "format": "fits",
        "fits_compress": false,
        "csv_interval_ms": 1000
    }
}
"@
Set-Content -Path "$DistDir\config\settings.json" -Value $DefaultSettings -Encoding UTF8
Set-Content -Path "$DistDir\settings.json" -Value $DefaultSettings -Encoding UTF8

# 10. Portable qt.conf
Write-Host "  Generating qt.conf..."
$QtConf = @"
[Paths]
Prefix = .
"@
Set-Content -Path "$DistDir\qt.conf" -Value $QtConf -Encoding UTF8

# 11. Run script launcher
$Launcher = @"
@echo off
setlocal
set PATH=%~dp0;%PATH%
start "" "%~dp0Sabit.exe" %*
"@
Set-Content -Path "$DistDir\Run_Sabit.bat" -Value $Launcher -Encoding ASCII

Write-Host ">>> Sabit Portable Distribution Assembled Successfully! <<<" -ForegroundColor Green
Get-ChildItem -Path $DistDir | Select-Object Name, Length, LastWriteTime | Format-Table -AutoSize
