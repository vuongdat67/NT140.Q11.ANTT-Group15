#include "filevault/cli/commands/hash_cmd.hpp"
#include "filevault/utils/console.hpp"
#include "filevault/utils/file_io.hpp"
#include "filevault/utils/crypto_utils.hpp"
#include "filevault/utils/progress.hpp"
#include <botan/hash.h>
#include <botan/hex.h>
#include <botan/mac.h>
#include <fmt/color.h>
#include <chrono>
#include <fstream>
#include <algorithm>

namespace filevault {
namespace cli {

HashCommand::HashCommand(core::CryptoEngine& engine)
    : engine_(engine) {
}

void HashCommand::setup(CLI::App& app) {
    auto* cmd = app.add_subcommand(name(), description());
    
    cmd->add_option("input", input_file_, "Input file to hash")
        ->required()
        ->check(CLI::ExistingFile);
    
    cmd->add_option("-a,--algorithm", algorithm_, 
                   "Hash algorithm: md5, sha1, sha224, sha256, sha384, sha512, "
                   "sha3-256, sha3-512, blake2b-512, blake2s-256")
        ->default_val("sha256");
    
    cmd->add_option("-o,--output", output_file_, 
                   "Output file for hash (default: stdout)");
    
    cmd->add_option("-v,--verify", verify_hash_, 
                   "Verify against expected hash");
    
    cmd->add_option("--hmac", hmac_key_, 
                   "Calculate HMAC with key (hex or string)");
    
    cmd->add_flag("--uppercase", uppercase_, 
                 "Output hash in uppercase");
    
    cmd->add_flag("--no-filename", no_filename_, 
                 "Don't include filename in output");
    
    cmd->add_flag("--verbose", verbose_, 
                 "Verbose output");
    
    cmd->add_flag("--benchmark", benchmark_, 
                 "Show performance metrics");
    
    cmd->callback([this]() { execute(); });
}

std::string HashCommand::get_botan_algorithm_name(const std::string& algo) {
    static const std::unordered_map<std::string, std::string> algo_map = {
        {"md5", "MD5"},
        {"sha1", "SHA-1"},
        {"sha224", "SHA-224"},
        {"sha256", "SHA-256"},
        {"sha384", "SHA-384"},
        {"sha512", "SHA-512"},
        {"sha512-256", "SHA-512-256"},
        {"sha3-224", "SHA-3(224)"},
        {"sha3-256", "SHA-3(256)"},
        {"sha3-384", "SHA-3(384)"},
        {"sha3-512", "SHA-3(512)"},
        {"blake2b-256", "BLAKE2b(256)"},
        {"blake2b-384", "BLAKE2b(384)"},
        {"blake2b-512", "BLAKE2b(512)"},
        {"blake2s-256", "Blake2s(256)"}
    };
    
    auto it = algo_map.find(algo);
    return (it != algo_map.end()) ? it->second : algo;
}

bool HashCommand::is_secure_algorithm(const std::string& algo) {
    return algo != "md5" && algo != "sha1";
}

int HashCommand::execute() {
    try {
        // Warn about insecure algorithms
        if (!is_secure_algorithm(algorithm_)) {
            utils::Console::warning(
                fmt::format("Algorithm '{}' is cryptographically BROKEN!", algorithm_)
            );
            fmt::print("  Use for compatibility only, not for security!\n\n");
        }
        
        // Get Botan algorithm name
        std::string botan_algo = get_botan_algorithm_name(algorithm_);
        
        if (verbose_) {
            utils::Console::info(fmt::format("Algorithm: {}", botan_algo));
            utils::Console::info(fmt::format("File: {}", input_file_));
        }
        
        // Calculate hash
        std::string hash_result;
        auto start_time = std::chrono::high_resolution_clock::now();
        
        if (hmac_key_.empty()) {
            hash_result = calculate_file_hash(input_file_, botan_algo);
        } else {
            hash_result = calculate_file_hmac(input_file_, botan_algo, hmac_key_);
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time).count();
        
        // Format output
        if (uppercase_) {
            std::transform(hash_result.begin(), hash_result.end(), 
                          hash_result.begin(), ::toupper);
        }
        
        // Verify mode
        if (!verify_hash_.empty()) {
            return verify_mode(hash_result);
        }
        
        // Normal output mode
        std::string output;
        if (!no_filename_) {
            output = fmt::format("{}  {}", hash_result, input_file_);
        } else {
            output = hash_result;
        }
        
        // Write output
        if (output_file_.empty()) {
            fmt::print("{}\n", output);
        } else {
            std::ofstream out(output_file_);
            out << output << '\n';
            utils::Console::success(fmt::format("Hash written to: {}", output_file_));
        }
        
        // Benchmark info
        if (benchmark_ || verbose_) {
            auto file_size = utils::FileIO::file_size(input_file_);
            double throughput_mbps = (file_size / 1024.0 / 1024.0) / (duration_ms / 1000.0);
            
            fmt::print("\n");
            utils::Console::info(fmt::format("File size: {} bytes", file_size));
            utils::Console::info(fmt::format("Time: {} ms", duration_ms));
            utils::Console::info(fmt::format("Throughput: {:.2f} MB/s", throughput_mbps));
        }
        
        return 0;
        
    } catch (const Botan::Exception& e) {
        utils::Console::error(fmt::format("Botan error: {}", e.what()));
        return 1;
    } catch (const std::exception& e) {
        utils::Console::error(fmt::format("Error: {}", e.what()));
        return 1;
    }
}

std::string HashCommand::calculate_file_hash(
    const std::string& filepath,
    const std::string& algorithm
) {
    auto hash_func = Botan::HashFunction::create(algorithm);
    if (!hash_func) {
        throw std::runtime_error("Hash algorithm not available: " + algorithm);
    }
    
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + filepath);
    }
    
    // Get file size for progress
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Read and hash in chunks
    const size_t CHUNK_SIZE = 64 * 1024;  // 64KB chunks
    std::vector<uint8_t> buffer(CHUNK_SIZE);
    size_t total_read = 0;
    
    // Progress bar for large files
    std::unique_ptr<utils::ProgressBar> progress;
    if (verbose_ && file_size > 1024 * 1024) {  // > 1MB
        progress = std::make_unique<utils::ProgressBar>(
            fmt::format("Hashing with {}", algorithm),
            100  // Use percentage
        );
    }
    
    while (file) {
        file.read(reinterpret_cast<char*>(buffer.data()), CHUNK_SIZE);
        std::streamsize bytes_read = file.gcount();
        
        if (bytes_read > 0) {
            hash_func->update(buffer.data(), bytes_read);
            total_read += bytes_read;
            
            if (progress) {
                size_t percentage = (total_read * 100) / file_size;
                progress->set_progress(percentage);
            }
        }
    }
    
    if (progress) {
        progress->mark_as_completed();
    }
    
    auto result = hash_func->final();
    return Botan::hex_encode(result);
}

std::string HashCommand::calculate_file_hmac(
    const std::string& filepath,
    const std::string& hash_algorithm,
    const std::string& key_str
) {
    // Parse key (try hex first, then as string)
    std::vector<uint8_t> key;
    try {
        key = Botan::hex_decode(key_str);
    } catch (...) {
        key.assign(key_str.begin(), key_str.end());
    }
    
    // Create HMAC
    auto hmac = Botan::MessageAuthenticationCode::create(
        fmt::format("HMAC({})", hash_algorithm)
    );
    
    if (!hmac) {
        throw std::runtime_error("HMAC not available for: " + hash_algorithm);
    }
    
    hmac->set_key(key);
    
    // Read and process file
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + filepath);
    }
    
    const size_t CHUNK_SIZE = 64 * 1024;
    std::vector<uint8_t> buffer(CHUNK_SIZE);
    
    while (file) {
        file.read(reinterpret_cast<char*>(buffer.data()), CHUNK_SIZE);
        std::streamsize bytes_read = file.gcount();
        
        if (bytes_read > 0) {
            hmac->update(buffer.data(), bytes_read);
        }
    }
    
    auto result = hmac->final();
    return Botan::hex_encode(result);
}

int HashCommand::verify_mode(const std::string& calculated_hash) {
    // Normalize hashes for comparison
    std::string expected = verify_hash_;
    std::string actual = calculated_hash;
    
    std::transform(expected.begin(), expected.end(), expected.begin(), ::tolower);
    std::transform(actual.begin(), actual.end(), actual.begin(), ::tolower);
    
    // Remove whitespace
    expected.erase(std::remove_if(expected.begin(), expected.end(), ::isspace), 
                   expected.end());
    actual.erase(std::remove_if(actual.begin(), actual.end(), ::isspace), 
                 actual.end());
    
    if (expected == actual) {
        utils::Console::success(
            fmt::format("{}: [PASS] Hash verification successful", input_file_)
        );
        return 0;
    } else {
        utils::Console::error(
            fmt::format("{}: [FAIL] Hash verification failed", input_file_)
        );
        
        if (verbose_) {
            fmt::print("  Expected: {}\n", verify_hash_);
            fmt::print("  Actual:   {}\n", calculated_hash);
        }
        
        return 1;
    }
}

} // namespace cli
} // namespace filevault
