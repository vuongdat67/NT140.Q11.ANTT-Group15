# Unit tests
add_executable(filevault_tests
    # Crypto tests
    unit/crypto/test_aes.cpp
    unit/crypto/test_aes_nist_vectors.cpp
    unit/crypto/test_kdf.cpp
    # unit/crypto/test_classical.cpp
    
    # Core tests
    unit/core/test_file_format.cpp
    # unit/core/test_secure_memory.cpp
    
    # Compression tests
    unit/compression/test_compression.cpp
    
    # Integration tests
    integration/test_encrypt_decrypt.cpp
)

target_link_libraries(filevault_tests
    PRIVATE
        filevault
        Catch2::Catch2WithMain
)

# Catch2 integration
include(Catch)
catch_discover_tests(filevault_tests)

# Add test data directory (if exists)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/fixtures)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/fixtures
         DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

# Custom test target (use run_tests instead of 'test' to avoid conflict)
add_custom_target(run_tests
    COMMAND filevault_tests
    DEPENDS filevault_tests
    COMMENT "Running unit tests..."
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
