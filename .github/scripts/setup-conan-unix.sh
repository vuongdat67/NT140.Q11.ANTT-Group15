#!/bin/bash
# Setup Conan for Linux/macOS build
# Usage: ./setup-conan-unix.sh [COMPILER] [VERSION] [CONAN_VERSION]
# Example: ./setup-conan-unix.sh gcc 13 2.22.2
#          ./setup-conan-unix.sh clang 17 2.22.2
#          ./setup-conan-unix.sh apple-clang 16 2.22.2

set -e

COMPILER="${1:-gcc}"
COMPILER_VERSION="${2:-13}"
CONAN_VERSION="${3:-2.22.2}"
BUILD_TYPE="${4:-Release}"

echo "========================================"
echo "  Unix Conan Setup Script"
echo "  Compiler: $COMPILER $COMPILER_VERSION"
echo "========================================"

# Determine OS and libcxx
OS="Linux"
LIBCXX="libstdc++11"
ARCH="x86_64"

if [[ "$OSTYPE" == "darwin"* ]]; then
    OS="Macos"
    LIBCXX="libc++"
    # Check if ARM
    if [[ $(uname -m) == "arm64" ]]; then
        ARCH="armv8"
    fi
fi

if [[ "$COMPILER" == "clang" ]]; then
    LIBCXX="libc++"
fi

if [[ "$COMPILER" == "apple-clang" ]]; then
    LIBCXX="libc++"
fi

# Step 1: Install Conan
echo ""
echo "[1/5] Installing Conan $CONAN_VERSION..."
pip install "conan==$CONAN_VERSION"

# Step 2: Create Conan profile
echo ""
echo "[2/5] Creating Conan profile..."
mkdir -p ~/.conan2/profiles

cat > ~/.conan2/profiles/default << EOF
[settings]
arch=$ARCH
build_type=$BUILD_TYPE
compiler=$COMPILER
compiler.cppstd=20
compiler.libcxx=$LIBCXX
compiler.version=$COMPILER_VERSION
os=$OS
EOF

echo "Profile created at: ~/.conan2/profiles/default"
echo "--- Profile content ---"
cat ~/.conan2/profiles/default
echo "-----------------------"

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
