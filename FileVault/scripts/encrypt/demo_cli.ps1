# ========================================
# FileVault CLI Demo - Complete Workflow
# ========================================

Write-Host "`n" -NoNewline
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host "  FileVault CLI - Complete Demo" -ForegroundColor Cyan  
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host ""

# Create test files
Write-Host "[1/7] Creating test files..." -ForegroundColor Yellow
@"
This is a secret message that needs encryption.
It contains sensitive information that should be protected.
FileVault will encrypt this using AES-256-GCM with Argon2id KDF.
"@ | Out-File -FilePath "demo_secret.txt" -Encoding UTF8

$fileSize = (Get-Item demo_secret.txt).Length
Write-Host "  ✓ Created: demo_secret.txt ($fileSize bytes)`n" -ForegroundColor Green

# Test 1: Help command
Write-Host "[2/7] Testing help command..." -ForegroundColor Yellow
.\build\bin\Release\filevault.exe --help
Write-Host ""

# Test 2: Encrypt with standard mode  
Write-Host "[3/7] Encrypting file (standard mode)..." -ForegroundColor Yellow
$password = "DemoPassword123!"
($password, $password) | .\build\bin\Release\filevault.exe encrypt demo_secret.txt -m standard

if ($LASTEXITCODE -eq 0) {
    Write-Host "  ✓ Encrypted file created: demo_secret.txt.fv`n" -ForegroundColor Green
} else {
    Write-Host "  ✗ Encryption failed!`n" -ForegroundColor Red
    exit 1
}

# Test 3: Info command
Write-Host "[4/7] Displaying file information..." -ForegroundColor Yellow  
.\build\bin\Release\filevault.exe info demo_secret.txt.fv
Write-Host ""

# Test 4: Decrypt
Write-Host "[5/7] Decrypting file..." -ForegroundColor Yellow
$password | .\build\bin\Release\filevault.exe decrypt demo_secret.txt.fv -o demo_decrypted.txt

if ($LASTEXITCODE -eq 0) {
    Write-Host "  ✓ File decrypted: demo_decrypted.txt`n" -ForegroundColor Green
} else {
    Write-Host "  ✗ Decryption failed!`n" -ForegroundColor Red
    exit 1
}

# Test 5: Verify integrity
Write-Host "[6/7] Verifying file integrity..." -ForegroundColor Yellow
$originalHash = (Get-FileHash demo_secret.txt -Algorithm SHA256).Hash
$decryptedHash = (Get-FileHash demo_decrypted.txt -Algorithm SHA256).Hash

if ($originalHash -eq $decryptedHash) {
    Write-Host "  ✓ File integrity verified! Hashes match:" -ForegroundColor Green
    Write-Host "    Original:  $originalHash" -ForegroundColor Gray
    Write-Host "    Decrypted: $decryptedHash`n" -ForegroundColor Gray
} else {
    Write-Host "  ✗ Integrity check failed! Hashes don't match`n" -ForegroundColor Red
    exit 1
}

# Test 6: Wrong password test
Write-Host "[7/7] Testing wrong password detection..." -ForegroundColor Yellow
"WrongPassword" | .\build\bin\Release\filevault.exe decrypt demo_secret.txt.fv -o demo_wrong.txt 2>&1 | Out-Null

if ($LASTEXITCODE -ne 0) {
    Write-Host "  ✓ Wrong password correctly detected`n" -ForegroundColor Green
} else {
    Write-Host "  ✗ Wrong password was not detected!`n" -ForegroundColor Red
    exit 1
}

# Summary
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host "  Demo Summary" -ForegroundColor Cyan
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host "Original:  $((Get-Item demo_secret.txt).Length) bytes" -ForegroundColor White
Write-Host "Encrypted: $((Get-Item demo_secret.txt.fv).Length) bytes" -ForegroundColor White
Write-Host "Overhead:  $((Get-Item demo_secret.txt.fv).Length - (Get-Item demo_secret.txt).Length) bytes (header + auth tag)" -ForegroundColor White
Write-Host ""
Write-Host "All tests passed! ✓" -ForegroundColor Green
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host ""

# Cleanup
Write-Host "Cleaning up demo files..." -ForegroundColor Gray
Remove-Item demo_secret.txt, demo_secret.txt.fv, demo_decrypted.txt -ErrorAction SilentlyContinue
Write-Host "Demo complete!`n" -ForegroundColor Green
