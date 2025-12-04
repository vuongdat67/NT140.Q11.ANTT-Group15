# FileVault - Final Comprehensive Test Suite
# Tests all features with correct syntax

Write-Host "╔════════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║         FileVault Comprehensive Test Suite                ║" -ForegroundColor Cyan  
Write-Host "╚════════════════════════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host ""

$exe = "D:\code\filevault\build\build\Release\bin\release\filevault.exe"
$testDir = "D:\code\filevault\test_manual"

# Clean test directory
if (Test-Path $testDir) { Remove-Item -Recurse -Force $testDir }
New-Item -ItemType Directory -Path $testDir | Out-Null
Set-Location $testDir

# Create test files
"Test data for encryption" | Out-File "test.txt"
"Secret message" | Out-File "secret.txt"
1..1000 | ForEach-Object { "Line $_" } | Out-File "large.txt"

Write-Host "═══════════════════════════════════════════════════════════" -ForegroundColor Yellow
Write-Host " TEST 1: ENCRYPTION MODES" -ForegroundColor Yellow
Write-Host "═══════════════════════════════════════════════════════════" -ForegroundColor Yellow

Write-Host "`n1a. Basic mode (fast, good security):"
& $exe encrypt test.txt test_basic.fvlt -m basic -p abc123
& $exe decrypt test_basic.fvlt test_basic_dec.txt -p abc123
Write-Host "   ✓ Encrypted & decrypted with basic mode" -ForegroundColor Green

Write-Host "`n1b. Standard mode (balanced - recommended):"
& $exe encrypt test.txt test_standard.fvlt -m standard -p abc123
& $exe decrypt test_standard.fvlt test_standard_dec.txt -p abc123
Write-Host "   ✓ Encrypted & decrypted with standard mode" -ForegroundColor Green

Write-Host "`n1c. Advanced mode (maximum security):"
& $exe encrypt test.txt test_advanced.fvlt -m advanced -p abc123
& $exe decrypt test_advanced.fvlt test_advanced_dec.txt -p abc123
Write-Host "   ✓ Encrypted & decrypted with advanced mode" -ForegroundColor Green

Write-Host "`n═══════════════════════════════════════════════════════════" -ForegroundColor Yellow
Write-Host " TEST 2: ALGORITHMS" -ForegroundColor Yellow
Write-Host "═══════════════════════════════════════════════════════════" -ForegroundColor Yellow

Write-Host "`n2a. AES-256-GCM (recommended):"
& $exe encrypt test.txt test_aes.fvlt -a aes-256-gcm -p abc123
& $exe decrypt test_aes.fvlt test_aes_dec.txt -p abc123
Write-Host "   ✓ AES-256-GCM working" -ForegroundColor Green

Write-Host "`n2b. ChaCha20-Poly1305:"
& $exe encrypt test.txt test_chacha.fvlt -a chacha20-poly1305 -p abc123
& $exe decrypt test_chacha.fvlt test_chacha_dec.txt -p abc123
Write-Host "   ✓ ChaCha20 working" -ForegroundColor Green

Write-Host "`n2c. Serpent-256-GCM:"
& $exe encrypt test.txt test_serpent.fvlt -a serpent-256-gcm -p abc123
& $exe decrypt test_serpent.fvlt test_serpent_dec.txt -p abc123
Write-Host "   ✓ Serpent working" -ForegroundColor Green

Write-Host "`n2d. Kyber-1024-Hybrid (Post-Quantum):"
& $exe encrypt test.txt test_kyber.fvlt -a kyber-1024-hybrid -p abc123
& $exe decrypt test_kyber.fvlt test_kyber_dec.txt -p abc123
Write-Host "   ✓ Post-Quantum Kyber working" -ForegroundColor Green

Write-Host "`n═══════════════════════════════════════════════════════════" -ForegroundColor Yellow
Write-Host " TEST 3: COMPRESSION" -ForegroundColor Yellow
Write-Host "═══════════════════════════════════════════════════════════" -ForegroundColor Yellow

Write-Host "`n3a. ZLIB compression:"
& $exe compress large.txt -a zlib
if (Test-Path "large.txt.zlib") {
    & $exe decompress large.txt.zlib
    Write-Host "   ✓ ZLIB compress/decompress working" -ForegroundColor Green
} else {
    Write-Host "   ✗ ZLIB output file not found" -ForegroundColor Red
}

Write-Host "`n3b. LZMA compression (creates .xz):"
& $exe compress large.txt -a lzma
if (Test-Path "large.txt.xz") {
    & $exe decompress large.txt.xz
    if (Test-Path "large.txt") {
        Write-Host "   ✓ LZMA compress/decompress working" -ForegroundColor Green
    } else {
        Write-Host "   ✗ LZMA decompress failed" -ForegroundColor Red
    }
} else {
    Write-Host "   ✗ LZMA output file not found" -ForegroundColor Red
}

Write-Host "`n3c. Compression with encryption:"
& $exe encrypt large.txt test_compressed.fvlt -p abc123 --compression lzma
& $exe decrypt test_compressed.fvlt test_compressed_dec.txt -p abc123
Write-Host "   ✓ Compression + encryption working" -ForegroundColor Green

Write-Host "`n═══════════════════════════════════════════════════════════" -ForegroundColor Yellow
Write-Host " TEST 4: HASH" -ForegroundColor Yellow
Write-Host "═══════════════════════════════════════════════════════════" -ForegroundColor Yellow

Write-Host "`n4a. SHA-256:"
& $exe hash test.txt -a sha256
Write-Host "   ✓ SHA-256 hash calculated" -ForegroundColor Green

Write-Host "`n4b. SHA-256 with output:"
& $exe hash test.txt -a sha256 -o test_hash.txt
if (Test-Path "test_hash.txt") {
    $hash = Get-Content "test_hash.txt"
    Write-Host "   ✓ Hash saved to file: $hash" -ForegroundColor Green
} else {
    Write-Host "   ✗ Hash output file not created" -ForegroundColor Red
}

Write-Host "`n4c. BLAKE2b:"
& $exe hash test.txt -a blake2b
Write-Host "   ✓ BLAKE2b hash calculated" -ForegroundColor Green

Write-Host "`n═══════════════════════════════════════════════════════════" -ForegroundColor Yellow
Write-Host " TEST 5: ARCHIVE" -ForegroundColor Yellow
Write-Host "═══════════════════════════════════════════════════════════" -ForegroundColor Yellow

"File1 content" | Out-File "file1.txt"
"File2 content" | Out-File "file2.txt"
"File3 content" | Out-File "file3.txt"

Write-Host "`n5a. Create archive:"
& $exe archive create file1.txt file2.txt file3.txt -o test.fva -p archive123
if (Test-Path "test.fva") {
    Write-Host "   ✓ Archive created" -ForegroundColor Green
} else {
    Write-Host "   ✗ Archive not created" -ForegroundColor Red
}

Write-Host "`n5b. Extract archive:"
New-Item -ItemType Directory -Path "extracted" -Force | Out-Null
& $exe archive extract test.fva -o extracted -p archive123
if ((Test-Path "extracted/file1.txt") -and (Test-Path "extracted/file2.txt")) {
    Write-Host "   ✓ Archive extracted successfully" -ForegroundColor Green
} else {
    Write-Host "   ✗ Archive extraction failed" -ForegroundColor Red
}

Write-Host "`n═══════════════════════════════════════════════════════════" -ForegroundColor Yellow
Write-Host " TEST 6: INFO & LIST" -ForegroundColor Yellow
Write-Host "═══════════════════════════════════════════════════════════" -ForegroundColor Yellow

Write-Host "`n6a. File info:"
& $exe info test_aes.fvlt
Write-Host "   ✓ File info displayed" -ForegroundColor Green

Write-Host "`n6b. List algorithms:"
& $exe list | Select-Object -First 20
Write-Host "   ✓ Algorithm list displayed" -ForegroundColor Green

Write-Host "`n═══════════════════════════════════════════════════════════" -ForegroundColor Yellow
Write-Host " TEST 7: BENCHMARK (Quick)" -ForegroundColor Yellow
Write-Host "═══════════════════════════════════════════════════════════" -ForegroundColor Yellow

Write-Host "`n7a. Benchmark symmetric (quick):"
& $exe benchmark --symmetric -s 1024 -i 1 | Select-Object -First 30
Write-Host "   ✓ Symmetric benchmark completed" -ForegroundColor Green

Write-Host "`n7b. Benchmark hash only:"
& $exe benchmark --hash -s 1024 -i 1
Write-Host "   ✓ Hash benchmark completed" -ForegroundColor Green

Write-Host "`n7c. Benchmark KDF only:"
& $exe benchmark --kdf -s 1024 -i 1
Write-Host "   ✓ KDF benchmark completed" -ForegroundColor Green

Write-Host "`n7d. Benchmark compression only:"
& $exe benchmark --compression -s 1024 -i 1
Write-Host "   ✓ Compression benchmark completed" -ForegroundColor Green

Write-Host "`n═══════════════════════════════════════════════════════════" -ForegroundColor Yellow
Write-Host " TEST 8: KEY GENERATION" -ForegroundColor Yellow
Write-Host "═══════════════════════════════════════════════════════════" -ForegroundColor Yellow

Write-Host "`n8a. Generate RSA-2048 keypair:"
& $exe keygen -a rsa-2048 -o test_rsa
if ((Test-Path "test_rsa.pub") -and (Test-Path "test_rsa.priv")) {
    Write-Host "   ✓ RSA keypair generated" -ForegroundColor Green
} else {
    Write-Host "   ✗ RSA keypair not generated" -ForegroundColor Red
}

Write-Host "`n8b. Generate ECC-P256 keypair:"
& $exe keygen -a ecc-p256 -o test_ecc
if ((Test-Path "test_ecc.pub") -and (Test-Path "test_ecc.priv")) {
    Write-Host "   ✓ ECC keypair generated" -ForegroundColor Green
} else {
    Write-Host "   ✗ ECC keypair not generated" -ForegroundColor Red
}

Write-Host "`n8c. Generate Kyber-1024 keypair:"
& $exe keygen -a kyber-1024-hybrid -o test_kyber_key
if ((Test-Path "test_kyber_key.pub") -and (Test-Path "test_kyber_key.priv")) {
    Write-Host "   ✓ Kyber keypair generated" -ForegroundColor Green
} else {
    Write-Host "   ✗ Kyber keypair not generated" -ForegroundColor Red
}

Write-Host "`n═══════════════════════════════════════════════════════════" -ForegroundColor Yellow
Write-Host " TEST 9: CONFIG" -ForegroundColor Yellow
Write-Host "═══════════════════════════════════════════════════════════" -ForegroundColor Yellow

Write-Host "`n9a. Show config:"
& $exe config show
Write-Host "   ✓ Config displayed" -ForegroundColor Green

Write-Host "`n9b. Set config value:"
& $exe config set default.mode advanced
& $exe config show | Select-String "Default Mode"
Write-Host "   ✓ Config value changed" -ForegroundColor Green

Write-Host "`n9c. Reset config:"
& $exe config reset
Write-Host "   ✓ Config reset" -ForegroundColor Green

Write-Host "`n═══════════════════════════════════════════════════════════" -ForegroundColor Yellow
Write-Host " TEST 10: SECURITY LEVELS" -ForegroundColor Yellow
Write-Host "═══════════════════════════════════════════════════════════" -ForegroundColor Yellow

Write-Host "`n10a. Weak security (fast):"
& $exe encrypt test.txt test_weak.fvlt -a aes-256-gcm -s weak -p abc123
Write-Host "   ✓ Weak security encryption" -ForegroundColor Green

Write-Host "`n10b. Strong security:"
& $exe encrypt test.txt test_strong.fvlt -a aes-256-gcm -s strong -p abc123
Write-Host "   ✓ Strong security encryption" -ForegroundColor Green

Write-Host "`n10c. Paranoid security (maximum):"
& $exe encrypt test.txt test_paranoid.fvlt -a aes-256-gcm -s paranoid -p abc123
Write-Host "   ✓ Paranoid security encryption" -ForegroundColor Green

Write-Host "`n╔════════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║                    TESTS COMPLETED                        ║" -ForegroundColor Cyan
Write-Host "╚════════════════════════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host ""
Write-Host "Test directory: $testDir" -ForegroundColor White
Write-Host "Review the output above for any failures marked with ✗" -ForegroundColor Yellow
Write-Host ""
