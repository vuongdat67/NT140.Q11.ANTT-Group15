# FileVault - Quick Build & Test Guide

## 🚀 Quick Start

### Step 1: Clean Build
```powershell
# Remove old build directory
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue

# Install all dependencies (includes new benchmark/1.8.3)
conan install . --output-folder=build --build=missing -s build_type=Release

# Configure CMake
cmake --preset conan-default
```

### Step 2: Build All Targets
```powershell
# Build everything (CLI + Benchmarks)
cmake --build build --config Release

# Or build individually
cmake --build build --config Release --target filevault_cli
cmake --build build --config Release --target bench_crypto
cmake --build build --config Release --target bench_compression
cmake --build build --config Release --target bench_kdf
```

---

## ✅ Testing the 3 Fixes

### Fix 1: CLI Algorithm Selection

#### Test Basic Usage
```powershell
# Create test file
"Hello World from FileVault!" | Out-File test.txt

# Test AES-256-GCM (default)
.\build\cli\Release\filevault.exe encrypt test.txt
.\build\cli\Release\filevault.exe decrypt test.txt.fv

# Test AES-128-CBC
.\build\cli\Release\filevault.exe encrypt test.txt -a aes128 -m cbc -o test_aes128.fv
.\build\cli\Release\filevault.exe decrypt test_aes128.fv

# Test DES-CBC
.\build\cli\Release\filevault.exe encrypt test.txt -a des -m cbc -o test_des.fv
.\build\cli\Release\filevault.exe decrypt test_des.fv
```

#### Test KDF Selection
```powershell
# Argon2id (default, slower but more secure)
.\build\cli\Release\filevault.exe encrypt test.txt --kdf argon2id

# PBKDF2 (faster)
.\build\cli\Release\filevault.exe encrypt test.txt --kdf pbkdf2
```

#### Test Backward Compatibility (Presets)
```powershell
# Old syntax still works
.\build\cli\Release\filevault.exe encrypt test.txt --preset basic
.\build\cli\Release\filevault.exe encrypt test.txt --preset standard
.\build\cli\Release\filevault.exe encrypt test.txt --preset advanced
```

#### Test Validation
```powershell
# Should fail with error message
.\build\cli\Release\filevault.exe encrypt test.txt -a invalid_algo
# Expected: "✗ Error: Invalid algorithm. Supported: aes256, aes192, aes128, des, 3des"

.\build\cli\Release\filevault.exe encrypt test.txt -m invalid_mode
# Expected: "✗ Error: Invalid mode. Supported: gcm, cbc, ctr, ecb"
```

#### Test Help Output
```powershell
.\build\cli\Release\filevault.exe encrypt --help
# Should show all new options: -a, -m, --kdf, -c, --preset
```

---

### Fix 2: Compression

#### Test Zlib Compression
```powershell
# Create large compressible file
1..10000 | ForEach-Object { "This is line number $_" } | Out-File large_text.txt

# Encrypt with Zlib compression
.\build\cli\Release\filevault.exe encrypt large_text.txt -c zlib
# Should show compression ratio

# Decrypt and verify
.\build\cli\Release\filevault.exe decrypt large_text.txt.fv
Compare-Object (Get-Content large_text.txt) (Get-Content large_text.txt.fv.dec)
# Should be identical
```

#### Test Zstd Fallback
```powershell
# Encrypt with Zstd (will use Zlib fallback)
.\build\cli\Release\filevault.exe encrypt large_text.txt -c zstd
# Expected output: "⚠ Warning: Zstd not available, using Zlib fallback"

# Decrypt should work
.\build\cli\Release\filevault.exe decrypt large_text.txt.fv
```

#### Test No Compression
```powershell
# Encrypt without compression
.\build\cli\Release\filevault.exe encrypt large_text.txt -c none
# Encrypted file should be ~same size as original
```

---

### Fix 3: Benchmarks

#### Run All Benchmarks
```powershell
# Crypto benchmarks (AES-256-GCM, CBC, CTR, AES-128)
.\build\benchmarks\Release\bench_crypto.exe

# Compression benchmarks (Zlib levels 1/6/9)
.\build\benchmarks\Release\bench_compression.exe

# KDF benchmarks (Argon2id vs PBKDF2)
.\build\benchmarks\Release\bench_kdf.exe
```

#### Filter Specific Tests
```powershell
# Only AES-256-GCM tests
.\build\benchmarks\Release\bench_crypto.exe --benchmark_filter=AES256_GCM

# Only encryption tests
.\build\benchmarks\Release\bench_crypto.exe --benchmark_filter=Encrypt

# Only Level 6 compression
.\build\benchmarks\Release\bench_compression.exe --benchmark_filter=Level6

# Only Argon2 tests
.\build\benchmarks\Release\bench_kdf.exe --benchmark_filter=Argon2
```

#### Export Results
```powershell
# Save to JSON
.\build\benchmarks\Release\bench_crypto.exe --benchmark_format=json --benchmark_out=crypto_results.json

# Save to CSV
.\build\benchmarks\Release\bench_crypto.exe --benchmark_format=csv --benchmark_out=crypto_results.csv

# Save to console output
.\build\benchmarks\Release\bench_crypto.exe --benchmark_format=console > crypto_results.txt
```

#### Compare Performance
```powershell
# Run twice and compare
.\build\benchmarks\Release\bench_crypto.exe --benchmark_out=run1.json --benchmark_format=json
.\build\benchmarks\Release\bench_crypto.exe --benchmark_out=run2.json --benchmark_format=json

# Use compare.py from Google Benchmark tools
python compare.py benchmarks run1.json run2.json
```

---

## 📊 Expected Benchmark Results

### Crypto (Typical Values)
```
Benchmark                              Time             CPU   Iterations
-------------------------------------------------------------------------
BM_AES256_GCM_Encrypt/1024         2.15 us         2.14 us       327680   (470 MB/s)
BM_AES256_GCM_Encrypt/102400        175 us          174 us         4022   (570 MB/s)
BM_AES256_GCM_Encrypt/1024000      1754 us         1748 us          400   (565 MB/s)

BM_AES128_GCM_Encrypt/1024         1.85 us         1.84 us       380000   (540 MB/s)
BM_AES128_GCM_Encrypt/1024000      1520 us         1515 us          461   (650 MB/s)
```

### Compression (Typical Values)
```
Benchmark                              Time             CPU   Iterations
-------------------------------------------------------------------------
BM_Zlib_Compress_Text_Level1        850 us          847 us          823   (Ratio: 45%)
BM_Zlib_Compress_Text_Level6       1250 us         1245 us          561   (Ratio: 35%)
BM_Zlib_Compress_Text_Level9       2100 us         2095 us          334   (Ratio: 33%)

BM_Zlib_Decompress                  420 us          418 us         1671
```

### KDF (Typical Values)
```
Benchmark                              Time             CPU   Iterations
-------------------------------------------------------------------------
BM_PBKDF2_Default                    125 ms          124 ms            6
BM_Argon2id_Default                  890 ms          887 ms            1   (More secure!)
```

---

## 🐛 Troubleshooting

### Build Fails: "benchmark not found"
```powershell
# Make sure benchmark/1.8.3 is in conanfile.txt
Get-Content conanfile.txt | Select-String "benchmark"

# Re-run Conan install
conan install . --output-folder=build --build=missing -s build_type=Release
```

### Benchmark Crashes
```powershell
# Build in Release mode (Debug is MUCH slower)
cmake --build build --config Release --target bench_crypto

# Not Debug mode!
# cmake --build build --config Debug --target bench_crypto
```

### CLI Option Not Recognized
```powershell
# Check CLI11 version
Get-Content conanfile.txt | Select-String "cli11"
# Should be: cli11/2.3.2 or later

# Rebuild CLI
cmake --build build --config Release --target filevault_cli
```

### Compression Warning Appears
```powershell
# This is expected! Zstd uses Zlib fallback
.\build\cli\Release\filevault.exe encrypt test.txt -c zstd
# Output: "⚠ Warning: Zstd not available, using Zlib fallback"

# To remove warning, use Zlib directly
.\build\cli\Release\filevault.exe encrypt test.txt -c zlib
```

---

## ✅ Success Criteria

All tests pass if:

### Fix 1: CLI
- [x] `filevault encrypt test.txt -a aes256 -m gcm` works
- [x] `filevault encrypt test.txt -a aes128 -m cbc` works
- [x] `filevault encrypt test.txt -a des -m cbc` works
- [x] `--kdf argon2id` and `--kdf pbkdf2` both work
- [x] `--preset basic/standard/advanced` still works
- [x] Invalid options show error messages
- [x] `--help` shows all new options

### Fix 2: Compression
- [x] `-c zlib` compresses and decompresses correctly
- [x] `-c zstd` shows warning but works (Zlib fallback)
- [x] `-c none` disables compression
- [x] Decrypted file matches original (no data loss)
- [x] Compression reduces file size (text files)

### Fix 3: Benchmarks
- [x] `bench_crypto.exe` runs without errors
- [x] `bench_compression.exe` runs without errors
- [x] `bench_kdf.exe` runs without errors
- [x] Results show MB/s or ms timing
- [x] Can filter tests with `--benchmark_filter`
- [x] Can export to JSON/CSV

---

## 🎯 Performance Targets

### Should Achieve (Release build):
- **AES-256-GCM**: ≥400 MB/s (software), ≥2 GB/s (with AES-NI)
- **AES-128-GCM**: ≥500 MB/s (software), ≥3 GB/s (with AES-NI)
- **Zlib Level 6**: ≥50 MB/s compression, ≥200 MB/s decompression
- **Argon2id**: ~1-3 seconds (security vs speed tradeoff)
- **PBKDF2**: ~100-200 ms

### Warning Signs (Debug build):
If performance is much lower:
- Check you're using Release build: `--config Release`
- Check CPU supports AES-NI: `wmic cpu get caption`
- Check no other programs running
- Check power settings (not in battery saver mode)

---

## 📝 Quick Command Reference

```powershell
# Clean rebuild
Remove-Item -Recurse -Force build; conan install . --output-folder=build --build=missing -s build_type=Release; cmake --preset conan-default; cmake --build build --config Release

# Test all features
.\build\cli\Release\filevault.exe encrypt test.txt -a aes256 -m gcm -c zlib
.\build\cli\Release\filevault.exe encrypt test.txt -a aes128 -m cbc --kdf pbkdf2
.\build\cli\Release\filevault.exe encrypt test.txt --preset advanced

# Run all benchmarks
.\build\benchmarks\Release\bench_crypto.exe
.\build\benchmarks\Release\bench_compression.exe
.\build\benchmarks\Release\bench_kdf.exe

# Export results
.\build\benchmarks\Release\bench_crypto.exe --benchmark_format=json --benchmark_out=results.json
```

---

**Happy Testing!** 🎉

If any test fails, check:
1. Build is Release (not Debug)
2. All dependencies installed (conan install)
3. CMake configuration up-to-date (cmake --preset conan-default)
4. Latest code changes compiled (cmake --build build --config Release)

For detailed explanations, see:
- `docs/FIXES_COMPLETED.md` - Full documentation of all fixes
- `docs/ANALYSIS_AND_FIXES.md` - Technical deep dive
- `docs/ACTION_PLAN.md` - Step-by-step guide
