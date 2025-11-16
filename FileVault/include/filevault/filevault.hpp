#pragma once

#include "crypto/cipher.hpp"
#include "crypto/kdf.hpp"
#include "compression/compressor.hpp"
#include "core/file_format.hpp"
#include "core/secure_memory.hpp"
#include "types.hpp"
#include "exceptions.hpp"
#include <filesystem>
#include <memory>
#include <functional>
#include <string>

namespace filevault {

// Progress callback type
using ProgressCallback = std::function<void(double percent, const std::string& message)>;

// Security mode presets
enum class SecurityMode {
    BASIC,      // DES + PBKDF2 (educational)
    STANDARD,   // AES-256-GCM + Argon2id (recommended)
    ADVANCED    // AES-256-GCM + Argon2id + Zstd (maximum security)
};

/**
 * Main encryption service
 * Coordinates cipher, KDF, compression, and file operations
 */
class EncryptionService {
private:
    std::unique_ptr<crypto::ICipherEngine> cipher_;
    std::unique_ptr<crypto::IKDFEngine> kdf_;
    std::unique_ptr<compression::ICompressor> compressor_;

public:
    /**
     * Configure encryption service
     * @param mode Security mode preset (basic/standard/advanced)
     */
    explicit EncryptionService(SecurityMode mode = SecurityMode::STANDARD);

    /**
     * Custom configuration
     */
    EncryptionService(
        std::unique_ptr<crypto::ICipherEngine> cipher,
        std::unique_ptr<crypto::IKDFEngine> kdf,
        std::unique_ptr<compression::ICompressor> compressor = nullptr
    );

    /**
     * Encrypt file
     * @param input_path Path to plaintext file
     * @param output_path Path for encrypted output
     * @param password User password
     * @param callback Progress callback (optional)
     */
    void encrypt_file(
        const std::filesystem::path& input_path,
        const std::filesystem::path& output_path,
        const std::string& password,
        ProgressCallback callback = nullptr
    );

    /**
     * Decrypt file
     * @param input_path Path to encrypted file
     * @param output_path Path for decrypted output
     * @param password User password
     * @param callback Progress callback (optional)
     */
    void decrypt_file(
        const std::filesystem::path& input_path,
        const std::filesystem::path& output_path,
        const std::string& password,
        ProgressCallback callback = nullptr
    );

    /**
     * Get file information without decrypting
     * @param file_path Path to encrypted file
     * @return Header information
     */
    static core::FileHeader get_file_info(const std::filesystem::path& file_path);

private:
    SecureBytes derive_key(const std::string& password, const Bytes& salt);
    Bytes generate_salt();
    Bytes generate_iv();
};

} // namespace filevault
