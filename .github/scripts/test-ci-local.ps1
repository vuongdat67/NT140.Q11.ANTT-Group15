#!/usr/bin/env pwsh
# Test CI workflow locally on Windows
# Usage: .\test-ci-local.ps1 [-Compiler msvc|mingw]

param(
    [ValidateSet("msvc", "mingw")]
    [string]$Compiler = "msvc"
)

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Local CI Test - $Compiler" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# Clean build directory
if (Test-Path "build") {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force "build"
}

New-Item -ItemType Directory -Force -Path "build" | Out-Null
Set-Location "build"

try {
    # Step 1: Conan install
    Write-Host "`n[1/4] Running Conan install..." -ForegroundColor Yellow
    conan install .. --output-folder=. --build=missing
    
    # Step 2: Find toolchain
    Write-Host "`n[2/4] Finding toolchain..." -ForegroundColor Yellow
    Write-Host "=== DEBUG: CMake files ===" -ForegroundColor Magenta
    Get-ChildItem -Recurse -Filter "*.cmake" | ForEach-Object {
        Write-Host "  $($_.FullName)"
    }
    
    $toolchain = Get-ChildItem -Recurse -Filter "conan_toolchain.cmake" | Select-Object -First 1
    if (-not $toolchain) {
        throw "conan_toolchain.cmake not found!"
    }
    $toolchainPath = $toolchain.FullName
    Write-Host "Found: $toolchainPath" -ForegroundColor Green
    
    # Step 3: CMake configure
    Write-Host "`n[3/4] CMake configure..." -ForegroundColor Yellow
    if ($Compiler -eq "msvc") {
        cmake .. -G Ninja `
            -DCMAKE_BUILD_TYPE=Release `
            -DCMAKE_TOOLCHAIN_FILE="$toolchainPath" `
            -DBUILD_TESTS=ON
    } else {
        # For MinGW, need to use MSYS2 shell
        Write-Host "MinGW build should be run from MSYS2 shell" -ForegroundColor Yellow
        Write-Host "Run: cd build && cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=`"$toolchainPath`" -DBUILD_TESTS=ON"
        return
    }
    
    # Step 4: Build
    Write-Host "`n[4/4] Building..." -ForegroundColor Yellow
    cmake --build . --parallel
    
    Write-Host "`n========================================" -ForegroundColor Cyan
    Write-Host "  Build successful!" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Cyan
    
    # Run tests
    Write-Host "`nRunning tests..." -ForegroundColor Yellow
    ctest --output-on-failure -j $env:NUMBER_OF_PROCESSORS
    
} catch {
    Write-Host "`nERROR: $_" -ForegroundColor Red
    exit 1
} finally {
    Set-Location ..
}
