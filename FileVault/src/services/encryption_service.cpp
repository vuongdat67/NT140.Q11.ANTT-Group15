#include "filevault/filevault.hpp"
#include <fstream>
#include <ctime>
#include <botan/auto_rng.h>

namespace filevault {

EncryptionService::EncryptionService(SecurityMode mode) {
    // Configure based on security mode
    switch (mode) {
        case SecurityMode::BASIC:
            // Educational: DES-CBC + PBKDF2
            cipher_ = crypto::CipherFactory::create(CipherType::DES, CipherMode::CBC);
            kdf_ = crypto::KDFFactory::create(KDFType::PBKDF2);
            compressor_ = nullptr;  // No compression
            break;

        case SecurityMode::STANDARD:
            // Recommended: AES-256-GCM + Argon2id
            cipher_ = crypto::CipherFactory::create(CipherType::AES256, CipherMode::GCM);
            kdf_ = crypto::KDFFactory::create(KDFType::ARGON2ID);
            compressor_ = nullptr;  // Optional compression
            break;

        case SecurityMode::ADVANCED:
            // Maximum security: AES-256-GCM + Argon2id + Zstd
            cipher_ = crypto::CipherFactory::create(CipherType::AES256, CipherMode::GCM);
            kdf_ = crypto::KDFFactory::create(KDFType::ARGON2ID);
            compressor_ = compression::CompressorFactory::create(CompressionType::ZSTD);
            break;
    }
}

EncryptionService::EncryptionService(
    std::unique_ptr<crypto::ICipherEngine> cipher,
    std::unique_ptr<crypto::IKDFEngine> kdf,
    std::unique_ptr<compression::ICompressor> compressor
) : cipher_(std::move(cipher)),
    kdf_(std::move(kdf)),
    compressor_(std::move(compressor)) {}

void EncryptionService::encrypt_file(
    const std::filesystem::path& input_path,
    const std::filesystem::path& output_path,
    const std::string& password,
    ProgressCallback callback
) {
    // Step 1: Validate input file exists
    if (!std::filesystem::exists(input_path)) {
        throw FileNotFoundException(input_path.string());
    }
    
    auto report_progress = [&callback](double percent, const std::string& msg) {
        if (callback) callback(percent, msg);
    };
    
    report_progress(0.0, "Reading file...");
    
    // Step 2: Read input file into memory
    std::ifstream input(input_path, std::ios::binary);
    if (!input) {
        throw FileIOException("Cannot open input file: " + input_path.string());
    }
    
    Bytes plaintext((std::istreambuf_iterator<char>(input)),
                    std::istreambuf_iterator<char>());
    input.close();
    
    size_t original_size = plaintext.size();
    report_progress(20.0, "File read complete");
    
    // Step 3: Compress data if compressor is configured
    Bytes data_to_encrypt = plaintext;
    size_t compressed_size = original_size;
    CompressionType compression_type = CompressionType::NONE;
    
    if (compressor_) {
        report_progress(25.0, "Compressing...");
        data_to_encrypt = compressor_->compress(plaintext);
        compressed_size = data_to_encrypt.size();
        compression_type = compressor_->get_type();
        report_progress(40.0, "Compression complete");
    } else {
        report_progress(40.0, "Skipping compression");
    }
    
    // Step 4: Generate cryptographic materials
    report_progress(45.0, "Generating salt and IV...");
    Bytes salt = generate_salt();
    Bytes iv = generate_iv();
    
    // Step 5: Derive encryption key from password
    report_progress(50.0, "Deriving encryption key...");
    SecureBytes key = derive_key(password, salt);
    
    // Step 6: Encrypt data
    report_progress(60.0, "Encrypting...");
    Bytes ciphertext = cipher_->encrypt(data_to_encrypt, key, iv);
    report_progress(75.0, "Encryption complete");
    
    // Step 7: Build file header
    report_progress(80.0, "Building file format...");
    
    // Validate filename length (max 255 bytes)
    std::string filename = input_path.filename().string();
    if (filename.size() > 255) {
        throw InvalidArgumentException("Filename too long (max 255 bytes): " + filename);
    }
    
    // Build header structure manually for full control
    core::FileHeader header{};
    header.major_version = 1;
    header.minor_version = 0;
    header.cipher_type = cipher_->get_type();
    header.cipher_mode = cipher_->get_mode();
    header.iv_or_nonce = iv;
    header.salt = salt;
    header.kdf_type = kdf_ ? kdf_->get_type() : KDFType::PBKDF2;
    header.kdf_params = kdf_ ? kdf_->get_params() : crypto::KDFParams{};
    header.kdf_params.salt = salt;
    header.compression_type = compression_type;
    header.original_size = static_cast<uint32_t>(original_size);
    header.compressed_size = static_cast<uint32_t>(compressed_size);
    header.timestamp = static_cast<uint64_t>(std::time(nullptr));
    header.filename = filename;
    
    // Serialize header
    Bytes header_data = core::FileFormatHandler::serialize_header(header);
    
    report_progress(85.0, "Writing output file...");
    
    // Step 8: Write output file (header + ciphertext)
    std::ofstream output(output_path, std::ios::binary);
    if (!output) {
        throw FileIOException("Cannot create output file: " + output_path.string());
    }
    
    // Write header
    output.write(reinterpret_cast<const char*>(header_data.data()), header_data.size());
    
    // Write encrypted data
    output.write(reinterpret_cast<const char*>(ciphertext.data()), ciphertext.size());
    
    output.close();
    
    if (!output) {
        throw FileIOException("Error writing to output file: " + output_path.string());
    }
    
    report_progress(100.0, "Encryption complete!");
}

void EncryptionService::decrypt_file(
    const std::filesystem::path& input_path,
    const std::filesystem::path& output_path,
    const std::string& password,
    ProgressCallback callback
) {
    // Step 1: Validate input file exists
    if (!std::filesystem::exists(input_path)) {
        throw FileNotFoundException(input_path.string());
    }
    
    auto report_progress = [&callback](double percent, const std::string& msg) {
        if (callback) callback(percent, msg);
    };
    
    report_progress(0.0, "Reading encrypted file...");
    
    // Step 2: Read entire encrypted file into memory
    std::ifstream input(input_path, std::ios::binary);
    if (!input) {
        throw FileIOException("Cannot open encrypted file: " + input_path.string());
    }
    
    Bytes encrypted_data((std::istreambuf_iterator<char>(input)),
                         std::istreambuf_iterator<char>());
    input.close();
    
    report_progress(10.0, "Parsing header...");
    
    // Step 3: Parse header to get encryption parameters
    core::FileHeader header;
    try {
        header = core::FileFormatHandler::parse_header(encrypted_data);
    } catch (const InvalidFormatException& e) {
        throw InvalidFormatException(std::string("Invalid encrypted file format: ") + e.what());
    }
    
    report_progress(20.0, "Header parsed");
    
    // Step 4: Validate header and create appropriate cipher/KDF
    // Recreate cipher based on header info
    auto cipher = crypto::CipherFactory::create(header.cipher_type, header.cipher_mode);
    
    // Recreate KDF with parameters from header
    std::unique_ptr<crypto::IKDFEngine> kdf;
    if (header.kdf_type != KDFType::PBKDF2 || header.salt.size() > 0) {
        kdf = crypto::KDFFactory::create_from_params(header.kdf_params);
    }
    
    report_progress(30.0, "Deriving decryption key...");
    
    // Step 5: Derive decryption key from password using salt from header
    SecureBytes key;
    if (kdf) {
        key = kdf->derive_key(password, header.salt, cipher->get_key_size());
    } else {
        // For classical ciphers without KDF
        key = SecureBytes(password.begin(), password.end());
    }
    
    report_progress(45.0, "Key derived");
    
    // Step 6: Extract ciphertext (everything after header)
    report_progress(50.0, "Extracting ciphertext...");
    
    // Calculate actual header size by re-serializing
    // This ensures we get the exact size that was written
    Bytes header_bytes = core::FileFormatHandler::serialize_header(header);
    size_t header_size = header_bytes.size();
    
    if (encrypted_data.size() < header_size) {
        throw InvalidFormatException("Encrypted file truncated - header incomplete");
    }
    
    Bytes ciphertext(encrypted_data.begin() + header_size, encrypted_data.end());
    
    if (ciphertext.empty() && header.original_size > 0) {
        throw InvalidFormatException("Encrypted file truncated - no ciphertext found");
    }
    
    report_progress(60.0, "Decrypting...");
    
    // Step 7: Decrypt the data
    Bytes decrypted_data;
    try {
        decrypted_data = cipher->decrypt(ciphertext, key, header.iv_or_nonce);
    } catch (const AuthenticationFailedException&) {
        // Re-throw with more helpful message
        throw AuthenticationFailedException();
    } catch (const CryptoException& e) {
        throw CryptoException(std::string("Decryption failed: ") + e.what());
    }
    
    report_progress(75.0, "Decryption complete");
    
    // Step 8: Decompress if compression was used
    Bytes plaintext = decrypted_data;
    if (header.compression_type != CompressionType::NONE) {
        report_progress(80.0, "Decompressing...");
        
        auto decompressor = compression::CompressorFactory::create(header.compression_type);
        try {
            plaintext = decompressor->decompress(decrypted_data);
        } catch (const CompressionException& e) {
            throw CompressionException(std::string("Decompression failed: ") + e.what());
        }
        
        report_progress(90.0, "Decompression complete");
    } else {
        report_progress(85.0, "No decompression needed");
    }
    
    // Step 9: Verify size matches expected
    if (plaintext.size() != header.original_size) {
        throw InvalidFormatException(
            "Size mismatch after decryption - expected " + 
            std::to_string(header.original_size) + " bytes, got " + 
            std::to_string(plaintext.size()) + " bytes"
        );
    }
    
    report_progress(92.0, "Writing decrypted file...");
    
    // Step 10: Write decrypted data to output file
    std::ofstream output(output_path, std::ios::binary);
    if (!output) {
        throw FileIOException("Cannot create output file: " + output_path.string());
    }
    
    if (!plaintext.empty()) {
        output.write(reinterpret_cast<const char*>(plaintext.data()), plaintext.size());
    }
    
    output.close();
    
    if (!output) {
        throw FileIOException("Error writing to output file: " + output_path.string());
    }
    
    report_progress(100.0, "Decryption complete!");
}

core::FileHeader EncryptionService::get_file_info(const std::filesystem::path& file_path) {
    return core::FileFormatHandler::parse_header(file_path.string());
}

SecureBytes EncryptionService::derive_key(const std::string& password, const Bytes& salt) {
    if (!kdf_) {
        // For ciphers that don't use KDF (like classical ciphers)
        return SecureBytes(password.begin(), password.end());
    }
    
    return kdf_->derive_key(password, salt, cipher_->get_key_size());
}

Bytes EncryptionService::generate_salt() {
    if (!kdf_) return {};
    
    Botan::AutoSeeded_RNG rng;
    Bytes salt(kdf_->get_salt_size());
    rng.randomize(salt.data(), salt.size());
    return salt;
}

Bytes EncryptionService::generate_iv() {
    size_t iv_size = cipher_->get_iv_size();
    if (iv_size == 0) return {};
    
    Botan::AutoSeeded_RNG rng;
    Bytes iv(iv_size);
    rng.randomize(iv.data(), iv.size());
    return iv;
}

} // namespace filevault
