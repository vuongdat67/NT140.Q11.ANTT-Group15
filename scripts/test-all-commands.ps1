# FileVault - Comprehensive CLI Test Script
# Tests all commands, modes, algorithms, and features

param(
    [string]$ExePath = "D:\code\filevault\build\build\Release\bin\release\filevault.exe",
    [switch]$Quick,  # Run quick tests only
    [switch]$Verbose
)

$ErrorActionPreference = "Continue"
$TestDir = "D:\code\filevault\test_output"
$PassCount = 0
$FailCount = 0
$SkipCount = 0

# Colors
function Write-Success { param($msg) Write-Host "✓ $msg" -ForegroundColor Green }
function Write-Failure { param($msg) Write-Host "✗ $msg" -ForegroundColor Red }
function Write-Info { param($msg) Write-Host "ℹ $msg" -ForegroundColor Cyan }
function Write-Section { param($msg) Write-Host "`n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Yellow; Write-Host "  $msg" -ForegroundColor Yellow; Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━`n" -ForegroundColor Yellow }

function Test-Command {
    param(
        [string]$Name,
        [string[]]$Args,
        [string]$Description,
        [scriptblock]$Verify = $null
    )
    
    Write-Host "Testing: $Description..." -NoNewline
    
    try {
        if ($Verbose) {
            Write-Host ""
            Write-Info "Command: $ExePath $($Args -join ' ')"
        }
        
        $output = & $ExePath @Args 2>&1
        $exitCode = $LASTEXITCODE
        
        if ($Verbose) {
            Write-Host $output
        }
        
        if ($exitCode -ne 0) {
            Write-Host ""
            Write-Failure "$Name failed with exit code $exitCode"
            if (-not $Verbose) {
                Write-Host $output -ForegroundColor DarkGray
            }
            $script:FailCount++
            return $false
        }
        
        # Run verification if provided
        if ($null -ne $Verify) {
            $verifyResult = & $Verify
            if (-not $verifyResult) {
                Write-Host ""
                Write-Failure "$Name verification failed"
                $script:FailCount++
                return $false
            }
        }
        
        Write-Host " " -NoNewline
        Write-Success "PASS"
        $script:PassCount++
        return $true
        
    } catch {
        Write-Host ""
        Write-Failure "$Name exception: $_"
        $script:FailCount++
        return $false
    }
}

# Setup
Write-Section "FileVault Comprehensive Test Suite"
Write-Info "Executable: $ExePath"
Write-Info "Test Directory: $TestDir"

if (-not (Test-Path $ExePath)) {
    Write-Failure "Executable not found: $ExePath"
    exit 1
}

# Clean and create test directory
if (Test-Path $TestDir) {
    Remove-Item -Recurse -Force $TestDir
}
New-Item -ItemType Directory -Path $TestDir | Out-Null
Set-Location $TestDir

# Create test files
"Hello, FileVault!" | Out-File -Encoding UTF8 "test.txt"
"Secret data for encryption testing with various modes and algorithms." * 10 | Out-File -Encoding UTF8 "large_test.txt"
"Tiny" | Out-File -Encoding UTF8 "tiny.txt"

# Test 1: Basic Commands
Write-Section "1. Basic Information Commands"

Test-Command "Help" @("--help") "Display help message" -Verify {
    return $true
}

Test-Command "List" @("list") "List all algorithms" -Verify {
    return $true
}

# Test 2: Encryption & Decryption - Mode Presets
Write-Section "2. Encryption with Mode Presets"

Test-Command "Encrypt-Basic" @("encrypt", "test.txt", "test_basic.fvlt", "-m", "basic", "-p", "testpass123") "Encrypt with basic mode"
Test-Command "Decrypt-Basic" @("decrypt", "test_basic.fvlt", "test_basic_dec.txt", "-p", "testpass123") "Decrypt basic mode"
Test-Command "Encrypt-Standard" @("encrypt", "test.txt", "test_standard.fvlt", "-m", "standard", "-p", "testpass123") "Encrypt with standard mode"
Test-Command "Decrypt-Standard" @("decrypt", "test_standard.fvlt", "test_standard_dec.txt", "-p", "testpass123") "Decrypt standard mode"
Test-Command "Encrypt-Advanced" @("encrypt", "test.txt", "test_advanced.fvlt", "-m", "advanced", "-p", "testpass123") "Encrypt with advanced mode"
Test-Command "Decrypt-Advanced" @("decrypt", "test_advanced.fvlt", "test_advanced_dec.txt", "-p", "testpass123") "Decrypt advanced mode"

# Test 3: Different Algorithms
Write-Section "3. Encryption with Different Algorithms"

$algorithms = @(
    @{name="AES-256-GCM"; algo="aes-256-gcm"},
    @{name="ChaCha20"; algo="chacha20-poly1305"},
    @{name="Serpent"; algo="serpent-256-gcm"},
    @{name="Twofish"; algo="twofish-256-gcm"}
)

if (-not $Quick) {
    $algorithms += @(
        @{name="Camellia"; algo="camellia-256-gcm"},
        @{name="ARIA"; algo="aria-256-gcm"},
        @{name="SM4"; algo="sm4-gcm"}
    )
}

foreach ($algo in $algorithms) {
    $outFile = "test_$($algo.name.ToLower()).fvlt"
    $decFile = "test_$($algo.name.ToLower())_dec.txt"
    
    Test-Command "Encrypt-$($algo.name)" @("encrypt", "test.txt", $outFile, "-a", $algo.algo, "-p", "testpass123") "Encrypt with $($algo.name)"
    Test-Command "Decrypt-$($algo.name)" @("decrypt", $outFile, $decFile, "-p", "testpass123") "Decrypt $($algo.name)"
    Test-Command "Info-$($algo.name)" @("info", $outFile) "File info for $($algo.name)"
}

# Test 4: Post-Quantum Crypto
Write-Section "4. Post-Quantum Cryptography"

$pqAlgos = @("kyber-512-hybrid", "kyber-768-hybrid", "kyber-1024-hybrid")

foreach ($algo in $pqAlgos) {
    $outFile = "test_$($algo).fvlt"
    $decFile = "test_$($algo)_dec.txt"
    
    Test-Command "PQC-$algo" @("encrypt", "test.txt", $outFile, "-a", $algo, "-p", "testpass123") "Encrypt with $algo"
    Test-Command "PQC-Decrypt-$algo" @("decrypt", $outFile, $decFile, "-p", "testpass123") "Decrypt $algo"
}

# Test 5: Hash Functions
Write-Section "5. Hash Functions"

$hashAlgos = @("md5", "sha1", "sha256", "sha512", "sha3-256", "blake2b")

foreach ($algo in $hashAlgos) {
    Test-Command "Hash-$algo" @("hash", "test.txt", "-a", $algo) "Calculate $algo hash"
    Test-Command "Hash-$algo-Output" @("hash", "test.txt", "-a", $algo, "-o", "hash_$algo.txt") "Calculate $algo hash with output"
}

# Test 6: Compression
Write-Section "6. Compression & Decompression"

$compAlgos = @("zlib", "lzma")
if (-not $Quick) {
    # $compAlgos += "bzip2"  # Skip if disabled
}

foreach ($algo in $compAlgos) {
    Test-Command "Compress-$algo" @("compress", "large_test.txt", "-a", $algo) "Compress with $algo"
    Test-Command "Decompress-$algo" @("decompress", "large_test.txt.$($algo -eq 'lzma' ? 'xz' : $algo)") "Decompress $algo"
}

# Test 7: Archive Operations
Write-Section "7. Archive Operations"

New-Item -ItemType Directory -Path "archive_test" -Force | Out-Null
"File 1" | Out-File "archive_test/file1.txt"
"File 2" | Out-File "archive_test/file2.txt"
"File 3" | Out-File "archive_test/file3.txt"

Test-Command "Archive-Create" @("archive", "create", "archive_test/file1.txt", "archive_test/file2.txt", "archive_test/file3.txt", "-o", "test_archive.fva", "-p", "archivepass") "Create encrypted archive"

New-Item -ItemType Directory -Path "archive_extract" -Force | Out-Null
Test-Command "Archive-Extract" @("archive", "extract", "test_archive.fva", "-o", "archive_extract", "-p", "archivepass") "Extract archive"

# Test 8: Steganography
Write-Section "8. Steganography"

# Create a simple PNG (100x100 white image)
$pngPath = "test_image.png"
if (Get-Command magick -ErrorAction SilentlyContinue) {
    magick -size 100x100 xc:white $pngPath 2>$null
} else {
    # Skip stego tests if ImageMagick not available
    Write-Info "ImageMagick not found, creating dummy PNG"
    # Create minimal valid PNG
    [byte[]]$pngData = @(137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,1,0,0,0,1,8,6,0,0,0,31,21,196,137,0,0,0,10,73,68,65,84,120,156,99,0,1,0,0,5,0,1,13,10,45,180,0,0,0,0,73,69,78,68,174,66,96,130)
    [System.IO.File]::WriteAllBytes($pngPath, $pngData)
}

if (Test-Path $pngPath) {
    "Secret message for steganography" | Out-File "secret.txt"
    
    Test-Command "Stego-Capacity" @("stego", "capacity", $pngPath) "Check image capacity"
    Test-Command "Stego-Embed" @("stego", "embed", "secret.txt", $pngPath, "stego_output.png") "Embed data in image"
    Test-Command "Stego-Extract" @("stego", "extract", "stego_output.png", "extracted_secret.txt") "Extract data from image"
}

# Test 9: Key Generation
Write-Section "9. Key Generation"

Test-Command "Keygen-RSA2048" @("keygen", "-a", "rsa-2048", "-o", "test_rsa2048") "Generate RSA-2048 keypair"
Test-Command "Keygen-ECC" @("keygen", "-a", "ecc-p256", "-o", "test_ecc") "Generate ECC-P256 keypair"
Test-Command "Keygen-Kyber" @("keygen", "-a", "kyber-1024-hybrid", "-o", "test_kyber") "Generate Kyber-1024 keypair"

# Test 10: Benchmark
Write-Section "10. Benchmarking"

Test-Command "Benchmark-All" @("benchmark", "-s", "1024", "-i", "1") "Benchmark all algorithms (quick)"
Test-Command "Benchmark-Symmetric" @("benchmark", "--symmetric", "-s", "1024", "-i", "1") "Benchmark symmetric only"
Test-Command "Benchmark-Hash" @("benchmark", "--hash", "-s", "1024", "-i", "1") "Benchmark hash functions"
Test-Command "Benchmark-KDF" @("benchmark", "--kdf", "-s", "1024", "-i", "1") "Benchmark KDF"
Test-Command "Benchmark-Compression" @("benchmark", "--compression", "-s", "1024", "-i", "1") "Benchmark compression"

# Test 11: Config
Write-Section "11. Configuration"

Test-Command "Config-Show" @("config", "show") "Show configuration"
Test-Command "Config-Set" @("config", "set", "default.mode", "advanced") "Set config value"
Test-Command "Config-Show-After" @("config", "show") "Show configuration after change" -Verify {
    return $true
}

# Test 12: Security Levels
Write-Section "12. Security Levels"

$secLevels = @("weak", "medium", "strong", "paranoid")

foreach ($level in $secLevels) {
    $outFile = "test_sec_$level.fvlt"
    $decFile = "test_sec_${level}_dec.txt"
    
    Test-Command "Security-$level" @("encrypt", "test.txt", $outFile, "-a", "aes-256-gcm", "-s", $level, "-p", "testpass123") "Encrypt with $level security"
    Test-Command "Security-$level-Decrypt" @("decrypt", $outFile, $decFile, "-p", "testpass123") "Decrypt $level security"
}

# Test 13: Compression with Encryption
Write-Section "13. Combined Compression + Encryption"

Test-Command "Encrypt-Compressed-ZLIB" @("encrypt", "large_test.txt", "test_comp_zlib.fvlt", "-p", "testpass123", "--compression", "zlib") "Encrypt with ZLIB compression"
Test-Command "Encrypt-Compressed-LZMA" @("encrypt", "large_test.txt", "test_comp_lzma.fvlt", "-p", "testpass123", "--compression", "lzma") "Encrypt with LZMA compression"

Test-Command "Decrypt-Compressed-ZLIB" @("decrypt", "test_comp_zlib.fvlt", "test_comp_zlib_dec.txt", "-p", "testpass123") "Decrypt ZLIB compressed"
Test-Command "Decrypt-Compressed-LZMA" @("decrypt", "test_comp_lzma.fvlt", "test_comp_lzma_dec.txt", "-p", "testpass123") "Decrypt LZMA compressed"

# Summary
Write-Section "Test Summary"

$total = $PassCount + $FailCount + $SkipCount
$passRate = if ($total -gt 0) { [math]::Round(($PassCount / $total) * 100, 2) } else { 0 }

Write-Host ""
Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Cyan
Write-Host "Total Tests:    $total" -ForegroundColor White
Write-Success "Passed:         $PassCount ($passRate%)"
if ($FailCount -gt 0) {
    Write-Failure "Failed:         $FailCount"
}
if ($SkipCount -gt 0) {
    Write-Host "Skipped:        $SkipCount" -ForegroundColor Yellow
}
Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Cyan
Write-Host ""

if ($FailCount -eq 0) {
    Write-Success "All tests passed! 🎉"
    exit 0
} else {
    Write-Failure "Some tests failed. Review output above."
    exit 1
}
