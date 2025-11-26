#!/bin/bash
# Setup Conan for MinGW build (run inside MSYS2)
# Usage: ./setup-conan-mingw.sh [CONAN_VERSION]

set -e

CONAN_VERSION="${1:-2.22.2}"
BUILD_TYPE="${2:-Release}"

echo "========================================"
echo "  MinGW Conan Setup Script"
echo "========================================"

# Step 1: Install Conan
echo ""
echo "[1/5] Installing Conan $CONAN_VERSION..."
pip install --break-system-packages "conan==$CONAN_VERSION"

# Step 2: Create Conan profile at Windows path
# IMPORTANT: MSYS2's ~ is different from Windows USERPROFILE
echo ""
echo "[2/5] Creating Conan profile..."

# Use Windows path directly
PROFILE_DIR="/c/Users/$USER/.conan2/profiles"
if [ -n "$USERPROFILE" ]; then
    # Convert Windows path to MSYS2 path
    PROFILE_DIR=$(cygpath -u "$USERPROFILE")/.conan2/profiles
fi

# Fallback: try common locations
if [ ! -d "$(dirname "$PROFILE_DIR")" ]; then
    PROFILE_DIR="/c/Users/runneradmin/.conan2/profiles"
fi

mkdir -p "$PROFILE_DIR"

cat > "$PROFILE_DIR/default" << EOF
[settings]
arch=x86_64
build_type=$BUILD_TYPE
compiler=gcc
compiler.cppstd=20
compiler.libcxx=libstdc++11
compiler.version=14
os=Windows
EOF

echo "Profile created at: $PROFILE_DIR/default"
echo "--- Profile content ---"
cat "$PROFILE_DIR/default"
echo "-----------------------"

# Also create at MSYS2 home (belt and suspenders)
mkdir -p ~/.conan2/profiles
cp "$PROFILE_DIR/default" ~/.conan2/profiles/default 2>/dev/null || true

# Step 3: Create build directory
echo ""
echo "[3/5] Creating build directory..."
mkdir -p build

# Step 4: Install dependencies
echo ""
echo "[4/5] Installing Conan dependencies..."
conan install . --output-folder=build --build=missing

# Step 5: Find toolchain file
echo ""
echo "[5/5] Finding toolchain file..."

# Debug: Show what Conan created
echo ""
echo "--- DEBUG: Directory structure ---"
find build -type f -name "*.cmake" 2>/dev/null | head -20
echo "----------------------------------"

# Find conan_toolchain.cmake
TOOLCHAIN_FILE=$(find build -name "conan_toolchain.cmake" -type f | head -1)

if [ -n "$TOOLCHAIN_FILE" ]; then
    echo ""
    echo "Toolchain file found: $TOOLCHAIN_FILE"
    
    # Export for next step
    echo "CONAN_TOOLCHAIN_FILE=$TOOLCHAIN_FILE"
    
    # For GitHub Actions
    if [ -n "$GITHUB_OUTPUT" ]; then
        echo "CONAN_TOOLCHAIN_FILE=$TOOLCHAIN_FILE" >> "$GITHUB_OUTPUT"
    fi
    
    # Also write to a file for easy access
    echo -n "$TOOLCHAIN_FILE" > build/toolchain_path.txt
else
    echo "ERROR: conan_toolchain.cmake not found!"
    exit 1
fi

echo ""
echo "========================================"
echo "  Setup completed successfully!"
echo "========================================"
