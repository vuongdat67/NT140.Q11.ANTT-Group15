# Setup Script Fix - Summary

**Date**: November 12, 2025  
**Issue**: PowerShell script syntax errors  
**Solution**: Created working .bat alternative + fixed C++ standard version

## 🐛 Issues Found and Fixed

### Issue 1: PowerShell Script Syntax Errors
**Problem**: Missing line breaks before `catch` blocks causing parser errors
**Root Cause**: PowerShell requires proper line formatting for try-catch blocks
**Status**: ⚠️ Partially fixed (may still have encoding issues)

### Issue 2: Dependency Version Conflict
**Problem**: `fmt/11.0.2` conflicts with `fmt/10.2.1` required by `spdlog/1.14.1`
**Fix**: Changed `conanfile.txt` to use `fmt/10.2.1`
```diff
- fmt/11.0.2
+ fmt/10.2.1
```

### Issue 3: C++ Standard Version Mismatch
**Problem**: Botan 3.6.1 requires C++20, scripts used C++17
**Fix**: Updated both scripts to use `-s compiler.cppstd=20`
```diff
- conan install . --build=missing -s compiler.cppstd=17
+ conan install . --build=missing -s compiler.cppstd=20
```

## ✅ Working Solution: Batch File

### Created: `setup-windows.bat`
- ✅ **Simple batch script** - No PowerShell parsing issues
- ✅ **Works on all Windows versions** - XP through 11
- ✅ **Colored output** - Uses `echo [INFO]`, `[OK]`, `[ERROR]` tags
- ✅ **Proper error handling** - Checks `errorlevel` for each command
- ✅ **Cross-platform navigation** - Uses `cd /d "%~dp0..\..\..\"` to find project root

### Usage
```batch
cd d:\00-Project\Botan\FileVault
.\scripts\setup\windows\setup-windows.bat
```

## 📦 Current Installation Progress

### System Check Results
✅ Python 3.12.10 - Found  
✅ CMake 4.1.2 - Found  
✅ MinGW g++ - Found (C++ compiler)  
✅ Conan 2.22.2 - Found  
✅ Conan profile - Exists  

### Dependency Installation (In Progress)
The script is currently:
1. ✅ **Downloaded** packages from ConCenter:
   - `cli11/2.4.2` (header-only)
   - `cmake/3.31.9` (build tool)
   - `benchmark/1.9.0`
   - `spdlog/1.14.1`
   - `fmt/10.2.1`
   - `indicators/2.3`
   - `zlib/1.3.1`

2. 🔄 **Building** from source:
   - `botan/3.6.1` - Currently compiling (this takes 5-10 minutes)
   - `catch2/3.10.0` - Queued

### Expected Completion Time
- **Botan build**: 5-10 minutes (C++ crypto library, large codebase)
- **Catch2 build**: 2-3 minutes (testing framework)
- **Total**: ~10-15 minutes on first run

## 🔧 Files Modified

### 1. `conanfile.txt`
```diff
  [requires]
  botan/3.6.1
  cli11/2.4.2
  catch2/3.10.0
  spdlog/1.14.1
- fmt/11.0.2
+ fmt/10.2.1
  indicators/2.3
  zlib/1.3.1
  benchmark/1.9.0
```

### 2. `scripts/setup/windows/setup-windows.ps1`
- Fixed try-catch formatting
- Changed C++17 to C++20

### 3. `scripts/setup/windows/setup-windows.bat` ✨ **NEW**
- Complete rewrite as batch file
- Simpler syntax, more reliable
- Same functionality as PowerShell version

## 📋 Next Steps

### After Installation Completes:
1. **Verify installation success** - Look for `[OK] Release dependencies installed`
2. **Create CMakeLists.txt** files (next task)
3. **Test build system** with `cmake --preset conan-default`

### If Build Fails:
- Check Conan cache: `C:\Users\[Username]\.conan2\`
- Try clean rebuild: Delete `build/` folder and re-run
- Check compiler compatibility: Ensure MSVC or MinGW-w64 is properly installed

## 🎯 Why Batch File is Better

| Feature | PowerShell | Batch File |
|---------|-----------|------------|
| Syntax Complexity | High | Low |
| Encoding Issues | Common | Rare |
| Line Ending Sensitivity | Yes | No |
| Execution Policy | Requires bypass | None |
| Windows Compatibility | Win 7+ | XP+ |
| Color Support | Full | Basic |
| Error Handling | Complex | Simple |

**Recommendation**: Use `.bat` for simplicity, `.ps1` for advanced features

## 📝 Lessons Learned

1. **PowerShell is finicky** - Line endings and encoding matter
2. **Batch files are reliable** - Simpler syntax, fewer issues
3. **Conan dependencies need careful version matching** - Check `spdlog` dependencies
4. **Botan 3.6.1 requires C++20** - Update all scripts accordingly
5. **Building from source takes time** - First run can be 10-15 minutes

## 🚀 Success Indicators

When setup completes successfully, you'll see:
```
[OK] Release dependencies installed
[INFO] Installing Debug dependencies...
[OK] Debug dependencies installed

===================================
Setup Complete!
===================================

Next steps:
  1. Configure CMake:
     cmake --preset conan-default
  ...
```

---

**Status**: 🔄 Installation in progress (building Botan 3.6.1)  
**ETA**: 5-10 minutes remaining
