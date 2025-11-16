# FileVault library
add_library(filevault STATIC
    # Core
    core/file_format.cpp
    
    # Crypto
    crypto/cipher/cipher_factory.cpp
    crypto/cipher/classical/classical_ciphers.cpp
    crypto/kdf/kdf_factory.cpp
    
    # Compression
    compression/compression_factory.cpp
    
    # Services
    services/encryption_service.cpp
)

target_include_directories(filevault
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies from Conan
target_link_libraries(filevault
    PUBLIC
        botan::botan
    PRIVATE
        spdlog::spdlog
        ZLIB::ZLIB
        # zstd::zstd  # Optional
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(filevault PRIVATE bcrypt)
endif()

# Export symbols for shared library (if needed)
if(BUILD_SHARED_LIBS)
    target_compile_definitions(filevault
        PUBLIC FILEVAULT_SHARED
        PRIVATE FILEVAULT_EXPORTS
    )
endif()
