# FileVault - Build Error Analysis & Fix Strategy

## Current Status
❌ **Build fails with 100+ compilation errors**

## Root Cause
The project has **dual implementations**: Many classes are fully implemented in BOTH header files (inline) AND separate .cpp files, causing redefinition errors.

### Affected Files:
1. **secure_memory.hpp** - Has inline implementation + secure_memory.cpp exists
2. **cipher.hpp** - Defines AESEngine class + aes_engine.cpp redefines it  
3. **kdf.hpp** - Defines Argon2Engine/PBKDF2Engine + kdf_engines.cpp redefines them
4. **compressor.hpp** - Defines ZlibCompressor/ZstdCompressor + compressors.cpp redefines them

## Quick Fix Options

### Option A: Use Header-Only Implementations (FASTEST - 15 minutes)
**Delete these .cpp files:**
- `src/core/secure_memory.cpp` 
- `src/crypto/cipher/aes_engine.cpp`
- `src/crypto/kdf/kdf_engines.cpp`
- `src/compression/compressors.cpp`

**Keep:**
- All header files (they have complete implementations)
- `src/core/file_format.cpp` (needs fixes)
- `src/services/encryption_service.cpp` (needs fixes)

**Then fix remaining issues:**
1. Add `#include <string>` to secure_memory.hpp
2. Fix FileHeader field names in file_format.cpp
3. Add missing type aliases

### Option B: Use Separate .cpp Files (BETTER PRACTICE - 1 hour)
**Remove inline implementations from headers:**
- Edit cipher.hpp - remove AESEngine class definition, keep only forward declaration
- Edit kdf.hpp - remove Argon2Engine/PBKDF2Engine definitions
- Edit compressor.hpp - remove ZlibCompressor/ZstdCompressor definitions  
- Edit secure_memory.hpp - remove inline implementations

**Keep all .cpp files** with full implementations

### Option C: Start Fresh with Minimal Skeleton (CLEANEST - 2 hours)
1. Keep only interface definitions in headers (pure virtual classes)
2. Move ALL implementations to .cpp files
3. Add proper forward declarations and includes
4. Build incrementally, testing after each file

## Recommended Approach: **Option A** (Header-Only)

This gets us to a compiling state fastest. We can refactor later.

###  Step-by-Step Fix (Option A):

```powershell
# 1. Delete conflicting .cpp files
Remove-Item src/core/secure_memory.cpp
Remove-Item src/crypto/cipher/aes_engine.cpp  
Remove-Item src/crypto/kdf/kdf_engines.cpp
Remove-Item src/compression/compressors.cpp

# 2. Fix remaining files (see below)

# 3. Update CMakeLists.txt to remove deleted files from build

# 4. Rebuild
cmake --build build --config Release
```

### Required Header Fixes:

**1. secure_memory.hpp** - Add missing include:
```cpp
#pragma once

#include <vector>
#include <cstdint>
#include <cstring>
#include <string>  // ADD THIS LINE

namespace filevault::core {
// ... rest of file
```

**2. file_format.cpp** - Fix field name mismatches:
- Change `header.version_major` → `header.major_version`
- Change `header.version_minor` → `header.minor_version`  
- Change `header.iv_nonce` → `header.iv_or_nonce`
- Remove builder methods that don't exist in header

**3. filevault.hpp** - Add missing type aliases:
```cpp
#pragma once

#include "types.hpp"
#include "crypto/cipher.hpp"
#include "crypto/kdf.hpp"
#include "compression/compressor.hpp"
#include "core/file_format.hpp"
#include <filesystem>
#include <functional>

namespace filevault {

// ADD THESE:
using ProgressCallback = std::function<void(double percent, const std::string& message)>;

enum class SecurityMode {
    BASIC,      // DES + PBKDF2
    STANDARD,   // AES-256-GCM + Argon2id
    ADVANCED    // AES-256-GCM + Argon2id + Zstd
};

// ... rest of file
```

**4. src/CMakeLists.txt** - Remove deleted .cpp files:
```cmake
add_library(filevault STATIC
    # Core
    core/file_format.cpp
    # secure_memory.cpp - REMOVE THIS LINE
    
    # Crypto
    # crypto/cipher/aes_engine.cpp - REMOVE THIS LINE
    # crypto/cipher/classical/classical_ciphers.cpp - KEEP OR REMOVE based on header
    # crypto/kdf/kdf_engines.cpp - REMOVE THIS LINE
    
    # Compression
    # compression/compressors.cpp - REMOVE THIS LINE
    
    # Services
    services/encryption_service.cpp
)
```

## After These Fixes

The project should compile with warnings about unimplemented TODOs (which is correct - those are placeholders for future Botan integration).

## Next Steps After Successful Build

1. Implement actual Botan crypto operations
2. Add NIST test vectors
3. Implement CLI command handlers
4. Add comprehensive error handling

---

**Current Choice:** Waiting for user decision on which option to pursue.

**Estimated Time to Working Build:**
- Option A: 15-20 minutes  
- Option B: 60-90 minutes
- Option C: 120+ minutes
