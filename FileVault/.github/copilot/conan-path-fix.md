# Conan Cache Path Fix for Windows

## Problem
The error occurs because your Windows username "ThinkPad P1 Gen 4" contains spaces, and Botan's build system doesn't handle spaces in paths properly:

```
ERROR:root:Unhandled option(s): P1 Gen 4\.conan2\p\zlib98d2cd36e4f46\p\include P1 Gen 4\.conan2\p\zlib98d2cd36e4f46\p\lib
```

The path gets split at spaces: `C:\Users\ThinkPad P1 Gen 4\...` becomes `C:\Users\ThinkPad` and `P1 Gen 4\...`

## Solution 1: Change Conan Cache Location (RECOMMENDED)

Set Conan to use a cache path without spaces:

```powershell
# Create conan cache in project folder or root drive
mkdir D:\conan-cache

# Set Conan home environment variable
[System.Environment]::SetEnvironmentVariable('CONAN_USER_HOME', 'D:\conan-cache', 'User')

# Or for current session only
$env:CONAN_USER_HOME = "D:\conan-cache"

# Verify
conan config home
```

Then run setup again:
```powershell
.\scripts\setup\windows\setup-windows.bat
```

## Solution 2: Use System-Wide Cache

```powershell
# Use C drive root (requires admin for some operations)
$env:CONAN_USER_HOME = "C:\conan"
mkdir C:\conan
```

## Solution 3: Clean and Rebuild

If you already have failed builds:

```powershell
# Remove Conan cache
conan cache clean "*" --source --build --download

# Or remove entire .conan2 folder
Remove-Item -Recurse -Force "$env:USERPROFILE\.conan2"

# Then set new cache location and run setup
$env:CONAN_USER_HOME = "D:\conan-cache"
.\scripts\setup\windows\setup-windows.bat
```

## Solution 4: Use Precompiled Binaries

Add to conanfile.txt:

```ini
[conf]
tools.system.package_manager:mode=install
```

This tells Conan to prefer downloading prebuilt binaries instead of building from source.

## Recommended Steps

1. **Set Conan cache to project folder:**
   ```powershell
   $env:CONAN_USER_HOME = "D:\00-Project\conan-cache"
   mkdir D:\00-Project\conan-cache
   ```

2. **Clean existing cache:**
   ```powershell
   Remove-Item -Recurse -Force "$env:USERPROFILE\.conan2" -ErrorAction SilentlyContinue
   ```

3. **Re-detect profile:**
   ```powershell
   conan profile detect --force
   ```

4. **Run setup:**
   ```powershell
   .\scripts\setup\windows\setup-windows.bat
   ```

## Version Notes

- **Botan 3.6.1** is correct - keep it (latest stable)
- **fmt/11.0.2** is correct - keep it (latest)
- **fmt/10.2.1** was needed temporarily due to spdlog conflict, but 11.0.2 should work

The problem was never the versions - it's the spaces in the Windows username path!
