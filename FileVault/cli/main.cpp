#include "filevault/filevault.hpp"
#include <CLI/CLI.hpp>
#include <indicators/progress_bar.hpp>
#include <indicators/cursor_control.hpp>
#include <fmt/core.h>
#include <fmt/color.h>
#include <botan/hash.h>
#include <botan/exceptn.h>
#include <botan/auto_rng.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <ctime>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

// Set console to UTF-8 on Windows
#ifdef _WIN32
struct ConsoleUTF8 {
    ConsoleUTF8() {
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
    }
} console_utf8_init;
#endif

// Helper function to read password without echo
std::string read_password(const std::string& prompt) {
    std::cout << prompt;
    std::cout.flush();
    
    std::string password;
    
#ifdef _WIN32
    // Windows implementation using _getch()
    int ch_int;
    while ((ch_int = _getch()) != '\r') {  // \r is Enter on Windows
        char ch = static_cast<char>(ch_int);
        if (ch == '\b' && !password.empty()) {  // Backspace
            password.pop_back();
            std::cout << "\b \b";  // Erase character from screen
        } else if (ch != '\b') {
            password += ch;
            std::cout << '*';  // Show asterisk
        }
        std::cout.flush();
    }
#else
    // Linux/macOS implementation using termios
    termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~ECHO;  // Disable echo
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
    std::getline(std::cin, password);
    
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);  // Restore echo
#endif
    
    std::cout << "\n";
    return password;
}

// Format file size for display
std::string format_size(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unit_idx = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit_idx < 3) {
        size /= 1024.0;
        unit_idx++;
    }
    
    return fmt::format("{:.2f} {}", size, units[unit_idx]);
}

// Parse size string like "10MB", "500KB" to bytes
size_t parse_size(const std::string& size_str) {
    std::istringstream iss(size_str);
    double num_val;
    std::string unit;
    
    iss >> num_val;
    if (iss.fail() || num_val <= 0) {
        throw std::invalid_argument("Invalid size format: " + size_str);
    }
    
    std::getline(iss, unit);
    
    // Remove whitespace
    unit.erase(0, unit.find_first_not_of(" \t"));
    unit.erase(unit.find_last_not_of(" \t") + 1);
    
    // Convert to uppercase for comparison
    for (char& c : unit) c = std::toupper(c);
    
    if (unit.empty() || unit == "B") {
        return static_cast<size_t>(num_val);
    } else if (unit == "KB" || unit == "K") {
        return static_cast<size_t>(num_val * 1024);
    } else if (unit == "MB" || unit == "M") {
        return static_cast<size_t>(num_val * 1024 * 1024);
    } else if (unit == "GB" || unit == "G") {
        return static_cast<size_t>(num_val * 1024 * 1024 * 1024);
    } else {
        throw std::invalid_argument("Unknown size unit: " + unit);
    }
}

int main(int argc, char** argv) {
    CLI::App app{"FileVault - Secure file encryption tool"};
    app.require_subcommand(1);

    // ============================================================================
    // ENCRYPT COMMAND
    // ============================================================================
    auto encrypt_cmd = app.add_subcommand("encrypt", "Encrypt a file");
    std::string encrypt_input, encrypt_output, encrypt_password;
    std::string encrypt_algorithm = "aes256";
    std::string encrypt_mode = "gcm";
    std::string encrypt_kdf = "argon2id";
    std::string encrypt_compression = "";
    std::string encrypt_preset = "";
    bool compress = false;
    bool verbose = false;
    
    encrypt_cmd->add_option("input", encrypt_input, "Input file to encrypt")->required()->check(CLI::ExistingFile);
    encrypt_cmd->add_option("-o,--output", encrypt_output, "Output file (default: input.fv)");
    encrypt_cmd->add_option("-p,--password", encrypt_password, "Encryption password (prompt if not provided)");
    encrypt_cmd->add_option("-a,--algorithm", encrypt_algorithm, "Encryption algorithm: aes256, aes192, aes128")
        ->check(CLI::IsMember({"aes256", "aes192", "aes128"}));
    encrypt_cmd->add_option("-m,--mode", encrypt_mode, "Cipher mode: gcm, cbc, ctr")
        ->check(CLI::IsMember({"gcm", "cbc", "ctr"}));
    encrypt_cmd->add_option("-k,--kdf", encrypt_kdf, "Key derivation: argon2id, pbkdf2")
        ->check(CLI::IsMember({"argon2id", "pbkdf2"}));
    encrypt_cmd->add_option("--compression", encrypt_compression, "Compression: zlib, zstd, none")
        ->check(CLI::IsMember({"zlib", "zstd", "none", ""}));
    encrypt_cmd->add_option("--preset", encrypt_preset, "Preset configuration: basic, standard, advanced")
        ->check(CLI::IsMember({"basic", "standard", "advanced", ""}));
    encrypt_cmd->add_flag("-c,--compress", compress, "Enable compression (auto-select type)");
    encrypt_cmd->add_flag("-v,--verbose", verbose, "Verbose output");

    encrypt_cmd->callback([&]() {
        using namespace indicators;
        
        // Helper functions for parsing enums
        auto parse_cipher_type = [](const std::string& algo) -> filevault::CipherType {
            if (algo == "aes256") return filevault::CipherType::AES256;
            if (algo == "aes192") return filevault::CipherType::AES192;
            if (algo == "aes128") return filevault::CipherType::AES128;
            throw std::invalid_argument("Unknown algorithm: " + algo);
        };
        
        auto parse_cipher_mode = [](const std::string& mode) -> filevault::CipherMode {
            if (mode == "gcm") return filevault::CipherMode::GCM;
            if (mode == "cbc") return filevault::CipherMode::CBC;
            if (mode == "ctr") return filevault::CipherMode::CTR;
            throw std::invalid_argument("Unknown mode: " + mode);
        };
        
        auto parse_kdf_type = [](const std::string& kdf) -> filevault::KDFType {
            if (kdf == "argon2id") return filevault::KDFType::ARGON2ID;
            if (kdf == "pbkdf2") return filevault::KDFType::PBKDF2;
            throw std::invalid_argument("Unknown KDF: " + kdf);
        };
        
        auto parse_compression = [](const std::string& comp) -> filevault::CompressionType {
            if (comp.empty() || comp == "none") return filevault::CompressionType::NONE;
            if (comp == "zlib") return filevault::CompressionType::ZLIB;
            if (comp == "zstd") return filevault::CompressionType::ZSTD;
            throw std::invalid_argument("Unknown compression: " + comp);
        };
        
        try {
            // Get password
            std::string password = encrypt_password;
            if (password.empty()) {
                password = read_password("Enter password: ");
                if (password.empty()) {
                    fmt::print(fg(fmt::color::red), "[ERROR] Password cannot be empty\n");
                    std::exit(1);
                }
                
                std::string confirm = read_password("Confirm password: ");
                if (password != confirm) {
                    fmt::print(fg(fmt::color::red), "[ERROR] Passwords do not match\n");
                    std::exit(1);
                }
            }
            
            // Determine output path
            if (encrypt_output.empty()) {
                encrypt_output = encrypt_input + ".fv";
            }
            
            // Check if output exists
            if (fs::exists(encrypt_output)) {
                std::string response;
                while (true) {
                    fmt::print(fg(fmt::color::yellow), "[WARNING] '{}' exists. Overwrite? (y/n): ", encrypt_output);
                    std::getline(std::cin, response);
                    
                    if (!response.empty() && (response[0] == 'y' || response[0] == 'Y')) {
                        break;
                    } else if (!response.empty() && (response[0] == 'n' || response[0] == 'N')) {
                        fmt::print("[INFO] Cancelled\n");
                        return;
                    }
                }
            }
            
            // Create service based on preset or custom configuration
            filevault::EncryptionService service(filevault::SecurityMode::STANDARD);
            std::string config_description;
            
            if (!encrypt_preset.empty()) {
                // Use preset configuration
                if (encrypt_preset == "basic") {
                    service = filevault::EncryptionService(filevault::SecurityMode::BASIC);
                    config_description = "Basic (DES-CBC + PBKDF2)";
                } else if (encrypt_preset == "standard") {
                    service = filevault::EncryptionService(filevault::SecurityMode::STANDARD);
                    config_description = "Standard (AES-256-GCM + Argon2id)";
                } else if (encrypt_preset == "advanced") {
                    service = filevault::EncryptionService(filevault::SecurityMode::ADVANCED);
                    config_description = "Advanced (AES-256-GCM + Argon2id + Zstd)";
                }
            } else {
                // Custom configuration
                auto cipher = filevault::crypto::CipherFactory::create(
                    parse_cipher_type(encrypt_algorithm),
                    parse_cipher_mode(encrypt_mode)
                );
                
                auto kdf = filevault::crypto::KDFFactory::create(
                    parse_kdf_type(encrypt_kdf)
                );
                
                // Determine compression
                std::unique_ptr<filevault::compression::ICompressor> compressor = nullptr;
                std::string comp_type = encrypt_compression;
                if (comp_type.empty() && compress) {
                    comp_type = "zlib";  // Default when --compress flag used
                }
                
                if (!comp_type.empty() && comp_type != "none") {
                    compressor = filevault::compression::CompressorFactory::create(
                        parse_compression(comp_type)
                    );
                }
                
                service = filevault::EncryptionService(
                    std::move(cipher),
                    std::move(kdf),
                    std::move(compressor)
                );
                
                // Build description
                config_description = encrypt_algorithm + "-" + encrypt_mode + " + " + encrypt_kdf;
                if (!comp_type.empty() && comp_type != "none") {
                    config_description += " + " + comp_type;
                }
            }
            
            // Setup progress bar
            ProgressBar progress_bar{
                option::BarWidth{50},
                option::Start{"["},
                option::Fill{"="},
                option::Lead{">"},
                option::Remainder{" "},
                option::End{"]"},
                option::ForegroundColor{Color::green},
                option::ShowElapsedTime{true},
                option::ShowRemainingTime{true}
            };
            
            // Display info
            auto input_size = fs::file_size(encrypt_input);
            fmt::print(fg(fmt::color::cyan) | fmt::emphasis::bold, "\n=== FileVault Encryption ===\n");
            fmt::print("Input:        {}\n", encrypt_input);
            fmt::print("Output:       {}\n", encrypt_output);
            fmt::print("Size:         {}\n", format_size(input_size));
            
            if (!encrypt_preset.empty()) {
                fmt::print("Preset:       {}\n", config_description);
            } else {
                fmt::print("Algorithm:    {}-{}\n", encrypt_algorithm, encrypt_mode);
                fmt::print("KDF:          {}\n", encrypt_kdf);
                std::string comp_display = encrypt_compression.empty() && compress ? "zlib" : encrypt_compression;
                if (!comp_display.empty() && comp_display != "none") {
                    fmt::print("Compression:  {} (enabled)\n", comp_display);
                } else {
                    fmt::print("Compression:  None\n");
                }
            }
            fmt::print("\n");
            
            // Encrypt
            auto start_time = std::chrono::high_resolution_clock::now();
            
            service.encrypt_file(
                encrypt_input,
                encrypt_output,
                password,
                [&](double percent, const std::string& message) {
                    progress_bar.set_progress(static_cast<size_t>(percent));
                    if (verbose) {
                        progress_bar.set_option(option::PostfixText{message});
                    }
                    if (percent >= 100.0) {
                        progress_bar.mark_as_completed();
                    }
                }
            );
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            
            // Success message
            fmt::print("\n");
            fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, "[SUCCESS] ");
            fmt::print("Encrypted successfully!\n\n");
            
            auto output_size = fs::file_size(encrypt_output);
            double ratio = (static_cast<double>(output_size) / input_size) * 100.0;
            double throughput = (input_size / 1024.0 / 1024.0) / (duration.count() / 1000.0);
            
            fmt::print("Results:\n");
            fmt::print("  Original size:  {}\n", format_size(input_size));
            fmt::print("  Encrypted size: {} ({:.1f}% of original)\n", format_size(output_size), ratio);
            fmt::print("  Time:           {:.2f}s\n", duration.count() / 1000.0);
            fmt::print("  Throughput:     {:.2f} MB/s\n", throughput);
            fmt::print("\n");
            
        } catch (const std::exception& e) {
            fmt::print(fg(fmt::color::red), "[ERROR] {}\n", e.what());
            std::exit(1);
        }
    });

    // ============================================================================
    // DECRYPT COMMAND
    // ============================================================================
    auto decrypt_cmd = app.add_subcommand("decrypt", "Decrypt a file");
    std::string decrypt_input, decrypt_output, decrypt_password;
    bool decrypt_verbose = false;
    
    decrypt_cmd->add_option("input", decrypt_input, "Encrypted file (.fv)")->required()->check(CLI::ExistingFile);
    decrypt_cmd->add_option("-o,--output", decrypt_output, "Output file (default: remove .fv extension)");
    decrypt_cmd->add_option("-p,--password", decrypt_password, "Decryption password (prompt if not provided)");
    decrypt_cmd->add_flag("-v,--verbose", decrypt_verbose, "Verbose output");

    decrypt_cmd->callback([&]() {
        using namespace indicators;
        
        try {
            // Get password
            std::string password = decrypt_password;
            if (password.empty()) {
                password = read_password("Enter password: ");
                if (password.empty()) {
                    fmt::print(fg(fmt::color::red), "[ERROR] Password cannot be empty\n");
                    std::exit(1);
                }
            }
            
            // Determine output path
            if (decrypt_output.empty()) {
                fs::path input_path(decrypt_input);
                if (input_path.extension() == ".fv") {
                    decrypt_output = input_path.stem().string();
                } else {
                    decrypt_output = decrypt_input + ".dec";
                }
            }
            
            // Check if output exists
            if (fs::exists(decrypt_output)) {
                std::string response;
                while (true) {
                    fmt::print(fg(fmt::color::yellow), "[WARNING] '{}' exists. Overwrite? (y/n): ", decrypt_output);
                    std::getline(std::cin, response);
                    
                    if (!response.empty() && (response[0] == 'y' || response[0] == 'Y')) {
                        break;
                    } else if (!response.empty() && (response[0] == 'n' || response[0] == 'N')) {
                        fmt::print("[INFO] Cancelled\n");
                        return;
                    }
                }
            }
            
            // Create service
            filevault::EncryptionService service(filevault::SecurityMode::STANDARD);
            
            // Setup progress bar
            ProgressBar progress_bar{
                option::BarWidth{50},
                option::Start{"["},
                option::Fill{"="},
                option::Lead{">"},
                option::Remainder{" "},
                option::End{"]"},
                option::ForegroundColor{Color::blue},
                option::ShowElapsedTime{true},
                option::ShowRemainingTime{true}
            };
            
            // Display info
            auto input_size = fs::file_size(decrypt_input);
            fmt::print(fg(fmt::color::cyan) | fmt::emphasis::bold, "\n=== FileVault Decryption ===\n");
            fmt::print("Input:  {}\n", decrypt_input);
            fmt::print("Output: {}\n", decrypt_output);
            fmt::print("Size:   {}\n", format_size(input_size));
            fmt::print("\n");
            
            // Decrypt
            auto start_time = std::chrono::high_resolution_clock::now();
            
            service.decrypt_file(
                decrypt_input,
                decrypt_output,
                password,
                [&](double percent, const std::string& message) {
                    progress_bar.set_progress(static_cast<size_t>(percent));
                    if (decrypt_verbose) {
                        progress_bar.set_option(option::PostfixText{message});
                    }
                    if (percent >= 100.0) {
                        progress_bar.mark_as_completed();
                    }
                }
            );
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            
            // Success message
            fmt::print("\n");
            fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, "[SUCCESS] ");
            fmt::print("Decrypted successfully!\n\n");
            
            auto output_size = fs::file_size(decrypt_output);
            double throughput = (output_size / 1024.0 / 1024.0) / (duration.count() / 1000.0);
            
            fmt::print("Results:\n");
            fmt::print("  Decrypted size: {}\n", format_size(output_size));
            fmt::print("  Time:           {:.2f}s\n", duration.count() / 1000.0);
            fmt::print("  Throughput:     {:.2f} MB/s\n", throughput);
            fmt::print("\n");
            
        } catch (const filevault::AuthenticationFailedException& e) {
            fmt::print(fg(fmt::color::red), "[ERROR] Authentication failed - wrong password or corrupted file\n");
            std::exit(1);
        } catch (const std::exception& e) {
            fmt::print(fg(fmt::color::red), "[ERROR] {}\n", e.what());
            std::exit(1);
        }
    });

    // ============================================================================
    // INFO COMMAND
    // ============================================================================
    auto info_cmd = app.add_subcommand("info", "Display encrypted file information");
    std::string info_input;
    
    info_cmd->add_option("input", info_input, "Encrypted file (.fv)")->required()->check(CLI::ExistingFile);

    info_cmd->callback([&]() {
        try {
            filevault::EncryptionService service(filevault::SecurityMode::STANDARD);
            auto metadata = service.get_file_metadata(info_input);
            
            fmt::print(fg(fmt::color::cyan) | fmt::emphasis::bold, "\n=== FileVault File Information ===\n");
            fmt::print("File: {}\n\n", info_input);
            
            fmt::print(fmt::emphasis::bold, "Header:\n");
            fmt::print("  Version:     {}.{}\n", metadata.major_version, metadata.minor_version);
            fmt::print("  File Size:   {}\n", format_size(fs::file_size(info_input)));
            
            fmt::print(fmt::emphasis::bold, "\nCryptography:\n");
            fmt::print("  Algorithm:   ");
            if (metadata.cipher_type == filevault::CipherType::AES256) {
                fmt::print("AES-256");
            } else if (metadata.cipher_type == filevault::CipherType::AES128) {
                fmt::print("AES-128");
            }
            
            fmt::print(" / ");
            if (metadata.cipher_mode == filevault::CipherMode::GCM) {
                fmt::print("GCM (Authenticated)\n");
            } else if (metadata.cipher_mode == filevault::CipherMode::CBC) {
                fmt::print("CBC\n");
            }
            
            fmt::print("  KDF:         ");
            if (metadata.kdf_type == filevault::KDFType::Argon2id) {
                fmt::print("Argon2id\n");
                fmt::print("    Memory:      {} KB\n", metadata.kdf_memory_kb);
                fmt::print("    Iterations:  {}\n", metadata.kdf_iterations);
                fmt::print("    Parallelism: {}\n", metadata.kdf_parallelism);
            } else if (metadata.kdf_type == filevault::KDFType::PBKDF2) {
                fmt::print("PBKDF2-HMAC-SHA256\n");
                fmt::print("    Iterations: {}\n", metadata.kdf_iterations);
            }
            
            fmt::print(fmt::emphasis::bold, "\nCompression:\n");
            if (metadata.compression_type == filevault::CompressionType::ZLIB) {
                fmt::print("  Algorithm:   Zlib (enabled)\n");
            } else {
                fmt::print("  Algorithm:   None\n");
            }
            
            // Timestamp
            if (metadata.timestamp > 0) {
                std::time_t time = metadata.timestamp;
                std::tm* tm_info = std::localtime(&time);
                char buffer[80];
                std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
                fmt::print(fmt::emphasis::bold, "\nMetadata:\n");
                fmt::print("  Created:     {}\n", buffer);
            }
            
            fmt::print("\n");
            
        } catch (const std::exception& e) {
            fmt::print(fg(fmt::color::red), "[ERROR] {}\n", e.what());
            std::exit(1);
        }
    });

    // ============================================================================
    // HASH COMMAND
    // ============================================================================
    auto hash_cmd = app.add_subcommand("hash", "Compute file hash (checksum)");
    std::string hash_input;
    std::string hash_algo = "sha256";  // Default
    
    hash_cmd->add_option("input", hash_input, "File to hash")->required()->check(CLI::ExistingFile);
    hash_cmd->add_option("-a,--algorithm", hash_algo, "Hash algorithm: sha256, sha512, blake2b (default: sha256)")
        ->check(CLI::IsMember({"sha256", "sha512", "blake2b"}));

    hash_cmd->callback([&]() {
        try {
            // Map algorithm names
            std::string botan_algo;
            if (hash_algo == "sha256") {
                botan_algo = "SHA-256";
            } else if (hash_algo == "sha512") {
                botan_algo = "SHA-512";
            } else if (hash_algo == "blake2b") {
                botan_algo = "Blake2b";
            }
            
            auto hash = Botan::HashFunction::create(botan_algo);
            if (!hash) {
                fmt::print(fg(fmt::color::red), "[ERROR] Hash algorithm not available: {}\n", hash_algo);
                std::exit(1);
            }
            
            // Open file
            std::ifstream file(hash_input, std::ios::binary);
            if (!file) {
                fmt::print(fg(fmt::color::red), "[ERROR] Failed to open file\n");
                std::exit(1);
            }
            
            // Get file size
            auto file_size = fs::file_size(hash_input);
            size_t bytes_read = 0;
            
            fmt::print(fg(fmt::color::cyan), "Computing {} hash...\n", botan_algo);
            
            // Hash in chunks with progress for large files
            const size_t buffer_size = 64 * 1024;  // 64 KB chunks
            std::vector<uint8_t> buffer(buffer_size);
            
            while (file.read(reinterpret_cast<char*>(buffer.data()), buffer_size) || file.gcount() > 0) {
                size_t count = file.gcount();
                hash->update(buffer.data(), count);
                bytes_read += count;
                
                // Show progress for large files
                if (file_size > 10 * 1024 * 1024) {  // > 10 MB
                    double progress = 100.0 * bytes_read / file_size;
                    fmt::print("\rProgress: {:.1f}%", progress);
                    std::cout.flush();
                }
            }
            
            if (file_size > 10 * 1024 * 1024) {
                fmt::print("\n");
            }
            
            // Get final hash
            auto digest = hash->final();
            
            // Display result
            fmt::print("\n");
            fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, "{} Hash:\n", botan_algo);
            fmt::print(fg(fmt::color::yellow), "  ");
            for (auto byte : digest) {
                fmt::print("{:02x}", byte);
            }
            fmt::print("\n\n");
            fmt::print("File: {} ({})\n", hash_input, format_size(file_size));
            fmt::print("\n");
            
        } catch (const Botan::Exception& e) {
            fmt::print(fg(fmt::color::red), "[ERROR] Cryptography error: {}\n", e.what());
            std::exit(1);
        } catch (const std::exception& e) {
            fmt::print(fg(fmt::color::red), "[ERROR] {}\n", e.what());
            std::exit(1);
        }
    });

    // ============================================================================
    // BENCHMARK COMMAND
    // ============================================================================
    auto bench_cmd = app.add_subcommand("benchmark", "Performance benchmark");
    std::string bench_output = "benchmark_results.txt";
    std::vector<std::string> bench_sizes;
    
    bench_cmd->add_option("-o,--output", bench_output, "Output file for results (default: benchmark_results.txt)");
    bench_cmd->add_option("-s,--sizes", bench_sizes, "Test sizes: 1MB, 10MB, 100MB, etc.")
        ->delimiter(',');

    bench_cmd->callback([&]() {
        try {
            fmt::print(fg(fmt::color::cyan) | fmt::emphasis::bold, "\n=== FileVault Performance Benchmark ===\n\n");
            
            // Default sizes if not specified
            std::vector<size_t> test_sizes;
            if (bench_sizes.empty()) {
                test_sizes = {1024 * 1024, 10 * 1024 * 1024};  // 1MB, 10MB
            } else {
                for (const auto& size_str : bench_sizes) {
                    try {
                        test_sizes.push_back(parse_size(size_str));
                    } catch (const std::exception& e) {
                        fmt::print(fg(fmt::color::red), "[ERROR] Invalid size '{}': {}\n", size_str, e.what());
                        std::exit(1);
                    }
                }
            }
            
            fmt::print("Configuration:\n");
            fmt::print("  Algorithm:    AES-256-GCM + Argon2id\n");
            fmt::print("  Test sizes:   ");
            for (auto size : test_sizes) {
                fmt::print("{} ", format_size(size));
            }
            fmt::print("\n  Output:       {}\n\n", bench_output);
            
            // Open output file
            std::ofstream result_file(bench_output);
            result_file << "FileVault Performance Benchmark Results\n";
            result_file << "========================================\n\n";
            result_file << "Date: ";
            auto now = std::time(nullptr);
            result_file << std::ctime(&now);
            result_file << "Algorithm: AES-256-GCM + Argon2id\n\n";
            
            // Run benchmarks
            std::string temp_file = "benchmark_temp_" + std::to_string(std::time(nullptr)) + ".dat";
            std::string encrypted_file = temp_file + ".fv";
            std::string decrypted_file = temp_file + ".dec";
            std::string password = "BenchmarkTestPassword123!@#";
            
            for (size_t test_size : test_sizes) {
                fmt::print(fg(fmt::color::yellow), "Testing {} data...\n", format_size(test_size));
                
                // Create test file with random data
                {
                    std::ofstream temp(temp_file, std::ios::binary);
                    Botan::AutoSeeded_RNG rng;
                    const size_t chunk_size = 64 * 1024;  // 64 KB chunks
                    std::vector<uint8_t> data(chunk_size);
                    
                    for (size_t written = 0; written < test_size; written += chunk_size) {
                        size_t to_write = std::min(chunk_size, test_size - written);
                        rng.randomize(data.data(), to_write);
                        temp.write(reinterpret_cast<char*>(data.data()), to_write);
                    }
                }
                
                // Benchmark encryption
                filevault::EncryptionService service(filevault::SecurityMode::STANDARD);
                
                auto encrypt_start = std::chrono::high_resolution_clock::now();
                service.encrypt_file(temp_file, encrypted_file, password);
                auto encrypt_end = std::chrono::high_resolution_clock::now();
                
                auto encrypt_ms = std::chrono::duration_cast<std::chrono::milliseconds>(encrypt_end - encrypt_start).count();
                double encrypt_mbps = (test_size / 1024.0 / 1024.0) / (encrypt_ms / 1000.0);
                
                // Benchmark decryption
                auto decrypt_start = std::chrono::high_resolution_clock::now();
                service.decrypt_file(encrypted_file, decrypted_file, password);
                auto decrypt_end = std::chrono::high_resolution_clock::now();
                
                auto decrypt_ms = std::chrono::duration_cast<std::chrono::milliseconds>(decrypt_end - decrypt_start).count();
                double decrypt_mbps = (test_size / 1024.0 / 1024.0) / (decrypt_ms / 1000.0);
                
                // Display results
                fmt::print("  Encryption: {:6} ms  ({:6.2f} MB/s)\n", encrypt_ms, encrypt_mbps);
                fmt::print("  Decryption: {:6} ms  ({:6.2f} MB/s)\n", decrypt_ms, decrypt_mbps);
                fmt::print("  Total:      {:6} ms\n\n", encrypt_ms + decrypt_ms);
                
                // Write to file
                result_file << format_size(test_size) << " test:\n";
                result_file << "  Encryption: " << encrypt_ms << " ms (" << encrypt_mbps << " MB/s)\n";
                result_file << "  Decryption: " << decrypt_ms << " ms (" << decrypt_mbps << " MB/s)\n";
                result_file << "  Total: " << (encrypt_ms + decrypt_ms) << " ms\n\n";
                
                // Cleanup
                fs::remove(temp_file);
                fs::remove(encrypted_file);
                fs::remove(decrypted_file);
            }
            
            result_file.close();
            
            fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, "[SUCCESS] ");
            fmt::print("Benchmark complete! Results saved to: {}\n\n", bench_output);
            
        } catch (const std::exception& e) {
            fmt::print(fg(fmt::color::red), "[ERROR] Benchmark failed: {}\n", e.what());
            
            // Cleanup on error
            try {
                fs::remove("benchmark_temp.dat");
                fs::remove("benchmark_temp.dat.fv");
                fs::remove("benchmark_temp.dat.dec");
            } catch (...) {}
            
            std::exit(1);
        }
    });

    // Add usage examples to help
    app.footer(R"(
Examples:
  # Standard encryption (default: AES-256-GCM + Argon2id)
  filevault encrypt secret.txt

  # AES-128 with CBC mode
  filevault encrypt file.txt -a aes128 -m cbc

  # Custom: AES-256-CTR with PBKDF2 and Zlib compression
  filevault encrypt file.txt -a aes256 -m ctr -k pbkdf2 --compression zlib

  # Use preset configurations
  filevault encrypt data.zip --preset advanced    # Maximum security + Zstd
  filevault encrypt demo.txt --preset basic       # DES-CBC + PBKDF2 (educational)

  # Decrypt with automatic detection
  filevault decrypt encrypted.fv

  # View file information
  filevault info encrypted.fv

  # Compute file hash
  filevault hash document.pdf --algorithm sha256

  # Benchmark performance
  filevault benchmark --sizes 1MB,10MB,100MB
)");

    // Parse and run
    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return app.exit(e);
    }

    return 0;
}
