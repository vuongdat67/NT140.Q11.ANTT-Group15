# FileVault Build Fixes Required

## Summary

The project skeleton has been successfully created with:
- ✅ **Conan dependencies installed** (C++20 standard configured)
- ✅ **CMake configured** successfully
- ❌ **Build failed** due to interface mismatches between headers and implementations

## Compilation Errors

### 1. File Format Handler Issues

**Error:** `parse_header` parameter mismatch
- **Header declares:** `FileHeader parse_header(const std::string& file_path)`
- **Implementation uses:** `FileHeader parse_header(const Bytes& data)`
- **Fix needed:** Change implementation to read file and then parse, OR change header to accept Bytes

**Error:** Missing `get_header_size` method
- **Used in:** `encryption_service.cpp:122`
- **Fix:** Add static method to `FileFormatHandler` class

### 2. FileHeader Structure Issues

**Error:** Missing `compression_type` field
- **Used in:** `encryption_service.cpp:137`
- **Current struct has:** `compression` field
- **Fix:** Rename field to `compression_type` in header

### 3. KDF Engine Interface Issues

**Error:** Missing methods in `IKDFEngine`
- `get_default_params()` - used in `encryption_service.cpp:168`
- `get_salt_size()` - used in `encryption_service.cpp:175`

**Header has:**
- `get_params()` - returns current params
- `get_recommended_salt_size()` - similar but different name

**Fix:** Either:
1. Add these methods to interface, OR
2. Update implementation to use existing method names

### 4. Missing ProgressCallback Parameter

**Error:** `callback` identifier not found in multiple places
- Lines: 110, 118, 126, 132, 138, 144, 148 in `encryption_service.cpp`
- **Cause:** Pseudocode comments using `callback` but variable not in scope
- **Fix:** Comment out or remove these pseudocode lines

## Quick Fix Strategy

### Option A: Minimal Compile Fix (Recommended for now)
Create stub implementations that compile but don't do full functionality:

```cpp
// In file_format.cpp - change parse_header to read file first
FileHeader FileFormatHandler::parse_header(const std::string& file_path) {
    // Read file
    std::ifstream file(file_path, std::ios::binary);
    Bytes data((std::istreambuf_iterator<char>(file)),
                std::istreambuf_iterator<char>());
    
    // Parse (existing code)
    // ...
}

// Add missing static method
static size_t get_header_size(const FileHeader& header) {
    return 128; // Placeholder
}
```

### Option B: Align All Interfaces (Better long-term)
1. Update all header files to match what implementations need
2. Ensure consistency across the codebase
3. Remove all pseudocode from `.cpp` files

## Current Project Status

### ✅ Working Components
1. Directory structure (100% complete)
2. Conan package management (Botan, CLI11, spdlog, indicators, Catch2, zlib)
3. CMake configuration with proper targets
4. C++20 standard configured

### 🔧 Needs Fixing (To Compile)
1. `file_format.cpp` - Interface mismatches (3 errors)
2. `encryption_service.cpp` - Remove pseudocode callback references (7 errors)
3. KDF interface - Add missing methods (2 errors)

### ⏳ Not Yet Implemented (TODOs in code)
1. Actual Botan crypto operations (AES encryption/decryption)
2. Argon2/PBKDF2 key derivation
3. Zlib/Zstd compression wrappers
4. CLI command handlers
5. Test implementations

## Next Steps

1. **Immediate:** Fix compilation errors above to get clean build
2. **Short-term:** Implement TODOs in crypto engines using Botan API
3. **Medium-term:** Add comprehensive tests with NIST vectors
4. **Long-term:** CLI polish, documentation, examples

## Files Needing Immediate Attention

1. `include/filevault/core/file_format.hpp` - Add `get_header_size`, fix field names
2. `src/core/file_format.cpp` - Match header signatures
3. `include/filevault/crypto/kdf.hpp` - Add convenience methods
4. `src/services/encryption_service.cpp` - Remove/comment pseudocode

## Build Command Reference

```powershell
# After fixes, rebuild:
cmake --build build --config Release

# Or clean and rebuild:
cmake --build build --config Release --clean-first

# Run tests (once compiling):
ctest --test-dir build -C Release
```

---

**Status:** Ready for interface alignment fixes to achieve first successful build.
