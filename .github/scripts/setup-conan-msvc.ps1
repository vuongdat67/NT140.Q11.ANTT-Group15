#!/usr/bin/env pwsh
# Setup Conan for MSVC build
# Usage: .\setup-conan-msvc.ps1 [-ConanVersion "2.22.2"]

param(
    [string]$ConanVersion = "2.22.2",
    [string]$BuildType = "Release"
)

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  MSVC Conan Setup Script" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# Step 1: Install Conan
Write-Host "`n[1/5] Installing Conan $ConanVersion..." -ForegroundColor Yellow
pip install "conan==$ConanVersion"
if ($LASTEXITCODE -ne 0) { throw "Failed to install Conan" }

# Step 2: Create Conan profile
Write-Host "`n[2/5] Creating Conan profile..." -ForegroundColor Yellow
$profileDir = Join-Path $env:USERPROFILE ".conan2\profiles"
New-Item -ItemType Directory -Force -Path $profileDir | Out-Null

$profile = @"
[settings]
arch=x86_64
build_type=$BuildType
compiler=msvc
compiler.cppstd=20
compiler.runtime=dynamic
compiler.version=194
os=Windows
"@

$profilePath = Join-Path $profileDir "default"
[System.IO.File]::WriteAllText($profilePath, $profile)

Write-Host "Profile created at: $profilePath" -ForegroundColor Green
Write-Host "--- Profile content ---"
Get-Content $profilePath
Write-Host "-----------------------"

# Step 3: Create build directory
Write-Host "`n[3/5] Creating build directory..." -ForegroundColor Yellow
$buildDir = "build"
New-Item -ItemType Directory -Force -Path $buildDir | Out-Null

# Step 4: Install dependencies
Write-Host "`n[4/5] Installing Conan dependencies..." -ForegroundColor Yellow
conan install . --output-folder=$buildDir --build=missing
if ($LASTEXITCODE -ne 0) { throw "Conan install failed" }

# Step 5: Find toolchain file
Write-Host "`n[5/5] Finding toolchain file..." -ForegroundColor Yellow

# Debug: Show what Conan created
Write-Host "`n--- DEBUG: Directory structure ---" -ForegroundColor Magenta
Get-ChildItem -Path $buildDir -Recurse -Name | ForEach-Object { Write-Host "  $_" }
Write-Host "----------------------------------"

# Find conan_toolchain.cmake
$toolchainFile = Get-ChildItem -Path $buildDir -Recurse -Filter "conan_toolchain.cmake" | Select-Object -First 1

if ($toolchainFile) {
    $toolchainPath = $toolchainFile.FullName
    Write-Host "`nToolchain file found: $toolchainPath" -ForegroundColor Green
    
    # Export for next step
    Write-Host "CONAN_TOOLCHAIN_FILE=$toolchainPath"
    
    # For GitHub Actions
    if ($env:GITHUB_OUTPUT) {
        "CONAN_TOOLCHAIN_FILE=$toolchainPath" | Out-File -FilePath $env:GITHUB_OUTPUT -Append
    }
    
    # Also write to a file for easy access
    $toolchainPath | Out-File -FilePath "$buildDir\toolchain_path.txt" -NoNewline
} else {
    Write-Host "ERROR: conan_toolchain.cmake not found!" -ForegroundColor Red
    exit 1
}

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "  Setup completed successfully!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
