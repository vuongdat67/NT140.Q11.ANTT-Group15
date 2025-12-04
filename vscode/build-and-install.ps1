# FileVault VSCode Extension - Build and Install Script
# This script compiles, packages, and installs the extension

Write-Host "=====================================" -ForegroundColor Cyan
Write-Host "FileVault VSCode Extension Builder" -ForegroundColor Cyan
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host ""

$ErrorActionPreference = "Stop"
$extensionDir = "d:\code\filevault\vscode"

try {
    # Navigate to extension directory
    Write-Host "📁 Navigating to extension directory..." -ForegroundColor Yellow
    Set-Location $extensionDir
    
    # Check if node_modules exists
    if (-not (Test-Path "node_modules")) {
        Write-Host "📦 Installing dependencies..." -ForegroundColor Yellow
        npm install
        if ($LASTEXITCODE -ne 0) {
            throw "npm install failed"
        }
    } else {
        Write-Host "✓ Dependencies already installed" -ForegroundColor Green
    }
    
    # Compile TypeScript
    Write-Host "🔨 Compiling TypeScript..." -ForegroundColor Yellow
    npm run compile
    if ($LASTEXITCODE -ne 0) {
        throw "Compilation failed"
    }
    Write-Host "✓ Compilation successful" -ForegroundColor Green
    
    # Check for vsce
    $vsceInstalled = $null -ne (Get-Command vsce -ErrorAction SilentlyContinue)
    if (-not $vsceInstalled) {
        Write-Host "📦 Installing vsce globally..." -ForegroundColor Yellow
        npm install -g @vscode/vsce
        if ($LASTEXITCODE -ne 0) {
            throw "vsce installation failed"
        }
    }
    
    # Package extension
    Write-Host "📦 Packaging extension..." -ForegroundColor Yellow
    $vsixFile = "filevault-extension.vsix"
    if (Test-Path $vsixFile) {
        Remove-Item $vsixFile -Force
    }
    
    vsce package --out $vsixFile
    if ($LASTEXITCODE -ne 0) {
        throw "Packaging failed"
    }
    Write-Host "✓ Extension packaged: $vsixFile" -ForegroundColor Green
    
    # Install extension
    Write-Host "🚀 Installing extension to VSCode..." -ForegroundColor Yellow
    code --install-extension $vsixFile --force
    if ($LASTEXITCODE -ne 0) {
        throw "Installation failed"
    }
    Write-Host "✓ Extension installed successfully!" -ForegroundColor Green
    
    Write-Host ""
    Write-Host "=====================================" -ForegroundColor Cyan
    Write-Host "✨ Build Complete!" -ForegroundColor Green
    Write-Host "=====================================" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Extension file: $extensionDir\$vsixFile" -ForegroundColor White
    Write-Host "Restart VSCode to activate the extension." -ForegroundColor Yellow
    Write-Host ""
    
    # Offer to restart VSCode
    $restart = Read-Host "Restart VSCode now? (y/n)"
    if ($restart -eq 'y' -or $restart -eq 'Y') {
        Write-Host "🔄 Restarting VSCode..." -ForegroundColor Yellow
        # Kill all VSCode processes
        Get-Process -Name "Code" -ErrorAction SilentlyContinue | Stop-Process -Force
        Start-Sleep -Seconds 2
        # Start VSCode
        Start-Process code -ArgumentList $extensionDir
        Write-Host "✓ VSCode restarted" -ForegroundColor Green
    }
    
} catch {
    Write-Host ""
    Write-Host "❌ Error: $_" -ForegroundColor Red
    Write-Host ""
    exit 1
}
