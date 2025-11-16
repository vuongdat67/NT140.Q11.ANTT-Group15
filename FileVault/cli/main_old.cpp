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

int main(int argc, char** argv) {
    CLI::App app{"FileVault - Cross-platform file encryption tool"};
    app.require_subcommand(1);

    // Encrypt command
    auto encrypt_cmd = app.add_subcommand("encrypt", "Encrypt a file");
    std::string encrypt_input, encrypt_output, encrypt_password;
    std::string mode_str = "standard";
    
    encrypt_cmd->add_option("input", encrypt_input, "Input file path")->required();
    encrypt_cmd->add_option("-o,--output", encrypt_output, "Output file path (default: input.fv)");
    encrypt_cmd->add_option("-m,--mode", mode_str, "Security mode: basic|standard|advanced")
        ->check(CLI::IsMember({"basic", "standard", "advanced"}));
    encrypt_cmd->add_option("-p,--password", encrypt_password, "Password (prompt if not provided)");

    encrypt_cmd->callback([&]() {
        using namespace indicators;
        
        try {
            // Step 1: Validate input file exists
            if (!fs::exists(encrypt_input)) {
                fmt::print(fg(fmt::color::red), "[FAIL] Error: Input file not found: {}\n", encrypt_input);
                std::exit(1);
            }
            
            // Step 2: Prompt for password if not provided
            std::string password = encrypt_password;
            if (password.empty()) {
                password = read_password("Enter encryption password: ");
                
                if (password.empty()) {
                    fmt::print(fg(fmt::color::red), "[FAIL] Error: Password cannot be empty\n");
                    std::exit(1);
                }
                
                // Confirm password
                std::string confirm = read_password("Confirm password: ");
                if (password != confirm) {
                    fmt::print(fg(fmt::color::red), "[FAIL] Error: Passwords do not match\n");
                    std::exit(1);
                }
            }
            
            // Step 3: Determine output path if not specified
            if (encrypt_output.empty()) {
                encrypt_output = encrypt_input + ".fv";
            }
            
            // Check if output file already exists
            if (fs::exists(encrypt_output)) {
                std::string response;
                while (true) {
                    std::cout << fmt::format("[WARNING] Output file '{}' already exists. Overwrite? (y/n): ", encrypt_output);
                    std::cin.clear();  // Clear any error flags
                    std::getline(std::cin, response);
                    
                    if (!response.empty() && (response[0] == 'y' || response[0] == 'Y')) {
                        break;  // Continue with encryption
                    } else if (!response.empty() && (response[0] == 'n' || response[0] == 'N')) {
                        fmt::print("[INFO] Encryption cancelled\n");
                        return;
                    }
                    // Empty or invalid input - ask again
                }
            }
            
            // Step 4: Parse security mode
            filevault::SecurityMode mode = filevault::SecurityMode::STANDARD;
            if (mode_str == "basic") {
                mode = filevault::SecurityMode::BASIC;
            } else if (mode_str == "advanced") {
                mode = filevault::SecurityMode::ADVANCED;
            }
            
            // Step 5: Create encryption service
            filevault::EncryptionService service(mode);
            
            // Step 6: Create progress bar
            ProgressBar progress_bar{
                option::BarWidth{50},
                option::Start{"["},
                option::Fill{"="},
                option::Lead{">"},
                option::Remainder{"-"},
                option::End{"]"},
                option::ForegroundColor{Color::green},
                option::ShowElapsedTime{true},
                option::ShowRemainingTime{true},
                option::FontStyles{std::vector<FontStyle>{FontStyle::bold}}
            };
            
            // Step 7: Encrypt file with progress callback
            fmt::print(fg(fmt::color::cyan), "🔒 Encrypting: {}\n", encrypt_input);
            fmt::print("   Mode: {}\n", mode_str);
            fmt::print("   Output: {}\n\n", encrypt_output);
            
            service.encrypt_file(
                encrypt_input,
                encrypt_output,
                password,
                [&progress_bar](double percent, const std::string& message) {
                    progress_bar.set_progress(static_cast<size_t>(percent));
                    progress_bar.set_option(option::PostfixText{message});
                    
                    if (percent >= 100.0) {
                        progress_bar.mark_as_completed();
                    }
                }
            );
            
            // Step 8: Display success message
            fmt::print("\n");
            fmt::print(fg(fmt::color::green), "[SUCCESS] Encryption successful!\n");
            
            auto input_size = fs::file_size(encrypt_input);
            auto output_size = fs::file_size(encrypt_output);
            double ratio = static_cast<double>(output_size) / input_size;
            
            fmt::print("  Input:  {} bytes\n", input_size);
            fmt::print("  Output: {} bytes ({:.1f}% of original)\n", output_size, ratio * 100);
            
        } catch (const filevault::FileNotFoundException& e) {
            fmt::print(fg(fmt::color::red), "[ERROR] File not found - {}\n", e.what());
            std::exit(1);
        } catch (const filevault::FileIOException& e) {
            fmt::print(fg(fmt::color::red), "[FAIL] Error: File I/O error - {}\n", e.what());
            std::exit(1);
        } catch (const filevault::CryptoException& e) {
            fmt::print(fg(fmt::color::red), "[FAIL] Error: Cryptographic error - {}\n", e.what());
            std::exit(1);
        } catch (const std::exception& e) {
            fmt::print(fg(fmt::color::red), "[FAIL] Error: {}\n", e.what());
            std::exit(1);
        }
    });

    // Decrypt command
    auto decrypt_cmd = app.add_subcommand("decrypt", "Decrypt a file");
    std::string decrypt_input, decrypt_output, decrypt_password;
    
    decrypt_cmd->add_option("input", decrypt_input, "Encrypted file path")->required();
    decrypt_cmd->add_option("-o,--output", decrypt_output, "Output file path");
    decrypt_cmd->add_option("-p,--password", decrypt_password, "Password (prompt if not provided)");

    decrypt_cmd->callback([&]() {
        try {
            // Validate input file exists
            if (!fs::exists(decrypt_input)) {
                std::cerr << fmt::format(fmt::fg(fmt::color::red), 
                    "[FAIL] Error: Input file '{}' not found\n", decrypt_input);
                return;
            }
            
            // Check if it's a .fv file
            if (fs::path(decrypt_input).extension() != ".fv") {
                std::cerr << fmt::format(fmt::fg(fmt::color::yellow), 
                    "[WARNING] Warning: Input file doesn't have .fv extension\n");
            }
            
            // Prompt for password if not provided
            if (decrypt_password.empty()) {
                decrypt_password = read_password("Enter decryption password: ");
                if (decrypt_password.empty()) {
                    std::cerr << fmt::format(fmt::fg(fmt::color::red), 
                        "[FAIL] Error: Password cannot be empty\n");
                    return;
                }
            }
            
            // Determine output path (remove .fv extension by default)
            if (decrypt_output.empty()) {
                fs::path input_path(decrypt_input);
                if (input_path.extension() == ".fv") {
                    decrypt_output = input_path.stem().string();
                } else {
                    decrypt_output = decrypt_input + ".decrypted";
                }
            }
            
            // Check if output file already exists
            if (fs::exists(decrypt_output)) {
                std::string response;
                while (true) {
                    std::cout << fmt::format("[WARNING] Output file '{}' already exists. Overwrite? (y/n): ", decrypt_output);
                    std::cin.clear();  // Clear any error flags
                    std::getline(std::cin, response);
                    
                    if (!response.empty() && (response[0] == 'y' || response[0] == 'Y')) {
                        break;  // Continue with decryption
                    } else if (!response.empty() && (response[0] == 'n' || response[0] == 'N')) {
                        fmt::print("[INFO] Decryption cancelled\n");
                        return;
                    }
                    // Empty or invalid input - ask again
                }
            }
            
            // Display decryption info
            std::cout << fmt::format(fmt::fg(fmt::color::cyan), "🔓 Decrypting: {}\n", decrypt_input);
            std::cout << fmt::format("   Output: {}\n\n", decrypt_output);
            
            // Create progress bar
            indicators::ProgressBar bar{
                indicators::option::BarWidth{40},
                indicators::option::Start{"["},
                indicators::option::Fill{"="},
                indicators::option::Lead{">"},
                indicators::option::Remainder{"-"},
                indicators::option::End{"]"},
                indicators::option::PostfixText{"Initializing..."},
                indicators::option::ForegroundColor{indicators::Color::cyan},
                indicators::option::FontStyles{std::vector<indicators::FontStyle>{indicators::FontStyle::bold}},
                indicators::option::ShowElapsedTime{true},
                indicators::option::ShowRemainingTime{true}
            };
            
            // Decrypt with progress callback
            filevault::EncryptionService service;
            service.decrypt_file(
                decrypt_input,
                decrypt_output,
                decrypt_password,
                [&bar](double progress, const std::string& status) {
                    bar.set_progress(static_cast<size_t>(progress * 100));
                    bar.set_option(indicators::option::PostfixText{status});
                }
            );
            
            bar.set_progress(100);
            bar.set_option(indicators::option::PostfixText{"Decryption complete!"});
            bar.mark_as_completed();
            
            indicators::show_console_cursor(true);
            
            // Display success message
            auto input_size = fs::file_size(decrypt_input);
            auto output_size = fs::file_size(decrypt_output);
            
            std::cout << "\n" << fmt::format(fmt::fg(fmt::color::green), "[SUCCESS] Decryption successful!\n");
            std::cout << fmt::format("  Input:  {} bytes\n", input_size);
            std::cout << fmt::format("  Output: {} bytes (original file)\n", output_size);
            
        } catch (const filevault::FileNotFoundException& e) {
            indicators::show_console_cursor(true);
            std::cerr << "\n" << fmt::format(fmt::fg(fmt::color::red), 
                "[FAIL] File Error: {}\n", e.what());
        } catch (const filevault::FileIOException& e) {
            indicators::show_console_cursor(true);
            std::cerr << "\n" << fmt::format(fmt::fg(fmt::color::red), 
                "[FAIL] I/O Error: {}\n", e.what());
        } catch (const filevault::CryptoException& e) {
            indicators::show_console_cursor(true);
            std::cerr << "\n" << fmt::format(fmt::fg(fmt::color::red), 
                "[FAIL] Decryption Failed: {}\n", e.what());
            std::cerr << fmt::format(fmt::fg(fmt::color::yellow), 
                "  --> Possible causes:\n");
            std::cerr << "    - Incorrect password\n";
            std::cerr << "    - File has been modified or corrupted\n";
            std::cerr << "    - File was encrypted with a different tool\n";
        } catch (const std::exception& e) {
            indicators::show_console_cursor(true);
            std::cerr << "\n" << fmt::format(fmt::fg(fmt::color::red), 
                "[FAIL] Unexpected Error: {}\n", e.what());
        }
    });

    // Info command
    auto info_cmd = app.add_subcommand("info", "Display file metadata");
    std::string info_input;
    
    info_cmd->add_option("input", info_input, "Encrypted file path")->required();

    info_cmd->callback([&]() {
        try {
            // Validate input file exists
            if (!fs::exists(info_input)) {
                std::cerr << fmt::format(fmt::fg(fmt::color::red), 
                    "[FAIL] Error: File '{}' not found\n", info_input);
                return;
            }
            
            // Get file info without decrypting
            auto header = filevault::EncryptionService::get_file_info(info_input);
            
            // Display formatted information
            std::cout << fmt::format(fmt::fg(fmt::color::cyan) | fmt::emphasis::bold, 
                "\n📋 FileVault Encrypted File Information\n");
            std::cout << fmt::format("{}\n\n", std::string(50, '='));
            
            // File info
            std::cout << fmt::format(fmt::emphasis::bold, "File: ") << info_input << "\n";
            std::cout << fmt::format(fmt::emphasis::bold, "Size: ") 
                << fs::file_size(info_input) << " bytes\n\n";
            
            // Format version
            std::cout << fmt::format(fmt::fg(fmt::color::yellow) | fmt::emphasis::bold, "Format:\n");
            std::cout << fmt::format("  Version: {}.{}\n", 
                static_cast<int>(header.major_version), 
                static_cast<int>(header.minor_version));
            
            // Cipher info
            std::cout << fmt::format(fmt::fg(fmt::color::yellow) | fmt::emphasis::bold, "\nCipher:\n");
            std::string cipher_name = "Unknown";
            switch (header.cipher_type) {
                case filevault::CipherType::AES256: cipher_name = "AES-256"; break;
                case filevault::CipherType::DES: cipher_name = "DES"; break;
                case filevault::CipherType::CHACHA20: cipher_name = "ChaCha20"; break;
                default: break;
            }
            
            std::string mode_name = "Unknown";
            switch (header.cipher_mode) {
                case filevault::CipherMode::GCM: mode_name = "GCM (Authenticated)"; break;
                case filevault::CipherMode::CBC: mode_name = "CBC"; break;
                case filevault::CipherMode::CTR: mode_name = "CTR"; break;
                default: break;
            }
            
            std::cout << fmt::format("  Algorithm: {}\n", cipher_name);
            std::cout << fmt::format("  Mode: {}\n", mode_name);
            std::cout << fmt::format("  IV/Nonce Size: {} bytes\n", header.iv_or_nonce.size());
            
            // KDF info
            std::cout << fmt::format(fmt::fg(fmt::color::yellow) | fmt::emphasis::bold, "\nKey Derivation:\n");
            std::string kdf_name = "Unknown";
            switch (header.kdf_type) {
                case filevault::KDFType::ARGON2ID: kdf_name = "Argon2id"; break;
                case filevault::KDFType::PBKDF2: kdf_name = "PBKDF2-SHA256"; break;
                default: break;
            }
            
            std::cout << fmt::format("  Algorithm: {}\n", kdf_name);
            std::cout << fmt::format("  Salt Size: {} bytes\n", header.salt.size());
            
            if (header.kdf_type == filevault::KDFType::ARGON2ID) {
                std::cout << fmt::format("  Memory: {} KB\n", header.kdf_params.memory_kb);
                std::cout << fmt::format("  Iterations: {}\n", header.kdf_params.iterations);
                std::cout << fmt::format("  Parallelism: {}\n", header.kdf_params.parallelism);
            } else if (header.kdf_type == filevault::KDFType::PBKDF2) {
                std::cout << fmt::format("  Iterations: {}\n", header.kdf_params.iterations);
            }
            
            // Compression info
            std::cout << fmt::format(fmt::fg(fmt::color::yellow) | fmt::emphasis::bold, "\nCompression:\n");
            std::string comp_name = "None";
            switch (header.compression_type) {
                case filevault::CompressionType::NONE: comp_name = "None"; break;
                case filevault::CompressionType::ZLIB: comp_name = "Zlib"; break;
                case filevault::CompressionType::ZSTD: comp_name = "Zstandard"; break;
                default: break;
            }
            
            std::cout << fmt::format("  Algorithm: {}\n", comp_name);
            std::cout << fmt::format("  Original Size: {} bytes\n", header.original_size);
            std::cout << fmt::format("  Compressed Size: {} bytes", header.compressed_size);
            
            if (header.compressed_size < header.original_size) {
                double ratio = 100.0 * (1.0 - static_cast<double>(header.compressed_size) / header.original_size);
                std::cout << fmt::format(fmt::fg(fmt::color::green), " ({:.1f}% reduction)", ratio);
            }
            std::cout << "\n";
            
            // Metadata
            std::cout << fmt::format(fmt::fg(fmt::color::yellow) | fmt::emphasis::bold, "\nMetadata:\n");
            std::cout << fmt::format("  Original Filename: {}\n", 
                header.filename.empty() ? "<unknown>" : header.filename);
            
            // Format timestamp
            std::time_t time = static_cast<std::time_t>(header.timestamp);
            char time_buf[100];
            std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", std::localtime(&time));
            std::cout << fmt::format("  Encrypted: {}\n", time_buf);
            
            std::cout << "\n";
            
        } catch (const filevault::FileNotFoundException& e) {
            std::cerr << fmt::format(fmt::fg(fmt::color::red), 
                "[FAIL] File Error: {}\n", e.what());
        } catch (const filevault::InvalidFormatException& e) {
            std::cerr << fmt::format(fmt::fg(fmt::color::red), 
                "[FAIL] Format Error: {}\n", e.what());
            std::cerr << "  → This file is not a valid FileVault encrypted file\n";
        } catch (const std::exception& e) {
            std::cerr << fmt::format(fmt::fg(fmt::color::red), 
                "[FAIL] Error: {}\n", e.what());
        }
    });

    // Hash command
    auto hash_cmd = app.add_subcommand("hash", "Compute file hash");
    std::string hash_input, hash_algo = "sha256";
    
    hash_cmd->add_option("input", hash_input, "File path")->required();
    hash_cmd->add_option("-a,--algorithm", hash_algo, "Hash algorithm: sha256|sha512|blake2b")
        ->check(CLI::IsMember({"sha256", "sha512", "blake2b"}));

    hash_cmd->callback([&]() {
        try {
            // Validate input file exists
            if (!fs::exists(hash_input)) {
                fmt::print(fg(fmt::color::red), "[ERROR] File not found: {}\n", hash_input);
                return;
            }
            
            // Map algorithm names to Botan hash function names
            std::string botan_algo;
            if (hash_algo == "sha256") {
                botan_algo = "SHA-256";
            } else if (hash_algo == "sha512") {
                botan_algo = "SHA-512";
            } else if (hash_algo == "blake2b") {
                botan_algo = "Blake2b";
            } else {
                fmt::print(fg(fmt::color::red), "[ERROR] Unknown algorithm: {}\n", hash_algo);
                return;
            }
            
            // Create hash function
            auto hash = Botan::HashFunction::create(botan_algo);
            if (!hash) {
                fmt::print(fg(fmt::color::red), "[ERROR] Failed to create hash function\n");
                return;
            }
            
            // Read and hash file
            std::ifstream file(hash_input, std::ios::binary);
            if (!file) {
                fmt::print(fg(fmt::color::red), "[ERROR] Failed to open file\n");
                return;
            }
            
            // Get file size for progress
            auto file_size = fs::file_size(hash_input);
            size_t bytes_read = 0;
            
            fmt::print(fg(fmt::color::cyan), "Computing {} hash of {}...\n", hash_algo, hash_input);
            
            // Hash in chunks
            const size_t buffer_size = 4096;
            std::vector<uint8_t> buffer(buffer_size);
            
            while (file.read(reinterpret_cast<char*>(buffer.data()), buffer_size) || file.gcount() > 0) {
                size_t count = file.gcount();
                hash->update(buffer.data(), count);
                bytes_read += count;
                
                // Show progress for large files
                if (file_size > 1024 * 1024) {  // > 1 MB
                    double progress = 100.0 * bytes_read / file_size;
                    fmt::print("\rProgress: {:.1f}%", progress);
                    std::cout.flush();
                }
            }
            
            if (file_size > 1024 * 1024) {
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
            fmt::print("File: {}\n", hash_input);
            fmt::print("Size: {} bytes\n", file_size);
            fmt::print("Algorithm: {}\n", hash_algo);
            
        } catch (const Botan::Exception& e) {
            fmt::print(fg(fmt::color::red), "[ERROR] Botan error: {}\n", e.what());
        } catch (const std::exception& e) {
            fmt::print(fg(fmt::color::red), "[ERROR] {}\n", e.what());
        }
    });

    // Benchmark command
    auto bench_cmd = app.add_subcommand("benchmark", "Run performance benchmarks");
    std::vector<std::string> algorithms;
    std::vector<std::string> sizes;
    
    bench_cmd->add_option("--algorithms", algorithms, "Algorithms to test (comma-separated)")->delimiter(',');
    bench_cmd->add_option("--sizes", sizes, "File sizes to test (e.g., 10MB,100MB)")->delimiter(',');

    bench_cmd->callback([&]() {
        try {
            fmt::print(fg(fmt::color::cyan) | fmt::emphasis::bold, "\n📊 FileVault Performance Benchmark\n");
            fmt::print("{}\n\n", std::string(50, '='));
            
            // Default algorithms if not specified
            std::vector<std::string> test_algos = algorithms.empty() ? 
                std::vector<std::string>{"aes256"} : algorithms;
            
            // Default sizes if not specified (in bytes)
            std::vector<size_t> test_sizes;
            if (sizes.empty()) {
                test_sizes = {1024 * 1024};  // 1 MB
            } else {
                for (const auto& size_str : sizes) {
                    // Parse number and unit
                    std::istringstream iss(size_str);
                    int num_val;
                    std::string unit;
                    
                    iss >> num_val;
                    if (iss.fail()) {
                        fmt::print(fg(fmt::color::red), "[WARNING] Invalid size format: {}, using 1MB\n", size_str);
                        test_sizes.push_back(1024 * 1024);
                        continue;
                    }
                    
                    // Get unit (rest of string)
                    std::getline(iss, unit);
                    
                    // Determine multiplier based on unit
                    size_t size = 0;
                    if (unit.find("KB") != std::string::npos || unit.find("kb") != std::string::npos) {
                        size = static_cast<size_t>(num_val) * 1024;
                    } else if (unit.find("MB") != std::string::npos || unit.find("mb") != std::string::npos) {
                        size = static_cast<size_t>(num_val) * 1024 * 1024;
                    } else if (unit.find("GB") != std::string::npos || unit.find("gb") != std::string::npos) {
                        size = static_cast<size_t>(num_val) * 1024 * 1024 * 1024;
                    } else {
                        // No suffix or unrecognized, assume MB
                        size = static_cast<size_t>(num_val) * 1024 * 1024;
                    }
                    test_sizes.push_back(size);
                }
            }
            
            // Create temporary test file
            std::string temp_file = "benchmark_temp.dat";
            std::string encrypted_file = temp_file + ".fv";
            std::string decrypted_file = temp_file + ".dec";
            std::string password = "BenchmarkPassword123";
            
            fmt::print("Configuration:\n");
            fmt::print("  Algorithms: ");
            for (const auto& algo : test_algos) {
                fmt::print("{} ", algo);
            }
            fmt::print("\n  Sizes: ");
            for (auto size : test_sizes) {
                if (size >= 1024 * 1024) {
                    fmt::print("{}MB ", size / (1024 * 1024));
                } else {
                    fmt::print("{}KB ", size / 1024);
                }
            }
            fmt::print("\n\n");
            
            // Run benchmarks
            for (size_t test_size : test_sizes) {
                fmt::print(fg(fmt::color::yellow), "Testing with {} data:\n", 
                    test_size >= 1024*1024 ? 
                    fmt::format("{} MB", test_size/(1024*1024)) :
                    fmt::format("{} KB", test_size/1024));
                
                // Create test file
                {
                    std::ofstream temp(temp_file, std::ios::binary);
                    Botan::AutoSeeded_RNG rng;
                    std::vector<uint8_t> data(4096);
                    
                    for (size_t written = 0; written < test_size; written += data.size()) {
                        size_t to_write = std::min(data.size(), test_size - written);
                        rng.randomize(data.data(), to_write);
                        temp.write(reinterpret_cast<char*>(data.data()), to_write);
                    }
                }
                
                // Benchmark encryption
                auto encrypt_start = std::chrono::high_resolution_clock::now();
                
                filevault::EncryptionService service(filevault::SecurityMode::STANDARD);
                service.encrypt_file(temp_file, encrypted_file, password);
                
                auto encrypt_end = std::chrono::high_resolution_clock::now();
                auto encrypt_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    encrypt_end - encrypt_start).count();
                
                // Benchmark decryption
                auto decrypt_start = std::chrono::high_resolution_clock::now();
                
                service.decrypt_file(encrypted_file, decrypted_file, password);
                
                auto decrypt_end = std::chrono::high_resolution_clock::now();
                auto decrypt_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    decrypt_end - decrypt_start).count();
                
                // Calculate throughput
                double encrypt_mbps = (test_size / (1024.0 * 1024.0)) / (encrypt_ms / 1000.0);
                double decrypt_mbps = (test_size / (1024.0 * 1024.0)) / (decrypt_ms / 1000.0);
                
                // Display results
                fmt::print("  Encryption: {} ms ({:.2f} MB/s)\n", encrypt_ms, encrypt_mbps);
                fmt::print("  Decryption: {} ms ({:.2f} MB/s)\n", decrypt_ms, decrypt_mbps);
                fmt::print("  Total:      {} ms\n\n", encrypt_ms + decrypt_ms);
                
                // Cleanup
                fs::remove(temp_file);
                fs::remove(encrypted_file);
                fs::remove(decrypted_file);
            }
            
            fmt::print(fg(fmt::color::green), "Benchmark complete!\n\n");
            
        } catch (const std::exception& e) {
            fmt::print(fg(fmt::color::red), "[ERROR] Benchmark failed: {}\n", e.what());
            
            // Cleanup on error
            fs::remove("benchmark_temp.dat");
            fs::remove("benchmark_temp.dat.fv");
            fs::remove("benchmark_temp.dat.dec");
        }
    });

    // Parse and run
    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return app.exit(e);
    }

    return 0;
}
