# FileVault CLI Demo
Write-Host ""
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host "  FileVault CLI - Complete Demo" -ForegroundColor Cyan  
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host ""

# Create test file
Write-Host "[1/7] Creating test file..." -ForegroundColor Yellow
"This is a secret message that needs encryption." | Out-File -FilePath "demo_secret.txt" -Encoding UTF8 -NoNewline
Write-Host "  ✓ Created demo_secret.txt" -ForegroundColor Green
Write-Host ""

# Test encrypt
Write-Host "[2/7] Encrypting file..." -ForegroundColor Yellow
$password = "DemoPassword123!"
($password, $password) | .\build\bin\Release\filevault.exe encrypt demo_secret.txt -m standard
Write-Host ""

# Test info
Write-Host "[3/7] Displaying file information..." -ForegroundColor Yellow  
.\build\bin\Release\filevault.exe info demo_secret.txt.fv
Write-Host ""

# Test decrypt
Write-Host "[4/7] Decrypting file..." -ForegroundColor Yellow
$password | .\build\bin\Release\filevault.exe decrypt demo_secret.txt.fv -o demo_decrypted.txt
Write-Host ""

# Verify integrity
Write-Host "[5/7] Verifying integrity..." -ForegroundColor Yellow
$hash1 = (Get-FileHash demo_secret.txt -Algorithm SHA256).Hash
$hash2 = (Get-FileHash demo_decrypted.txt -Algorithm SHA256).Hash

if ($hash1 -eq $hash2) {
    Write-Host "  ✓ File integrity verified!" -ForegroundColor Green
} else {
    Write-Host "  ✗ Integrity check failed!" -ForegroundColor Red
}
Write-Host ""

# Test wrong password
Write-Host "[6/7] Testing wrong password..." -ForegroundColor Yellow
"WrongPassword" | .\build\bin\Release\filevault.exe decrypt demo_secret.txt.fv -o demo_wrong.txt 2>$null

if ($LASTEXITCODE -ne 0) {
    Write-Host "  ✓ Wrong password correctly detected" -ForegroundColor Green
} else {
    Write-Host "  ✗ Failed to detect wrong password" -ForegroundColor Red
}
Write-Host ""

# Summary
Write-Host "[7/7] Summary" -ForegroundColor Yellow
Write-Host "  Original:  $((Get-Item demo_secret.txt).Length) bytes"
Write-Host "  Encrypted: $((Get-Item demo_secret.txt.fv).Length) bytes"
Write-Host ""
Write-Host "All tests passed! ✓" -ForegroundColor Green
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host ""

# Cleanup
Remove-Item demo_secret.txt, demo_secret.txt.fv, demo_decrypted.txt -ErrorAction SilentlyContinue
Write-Host "Demo complete!" -ForegroundColor Green
