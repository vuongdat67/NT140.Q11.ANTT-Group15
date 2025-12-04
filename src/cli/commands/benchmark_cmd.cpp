/**
 * @file benchmark_cmd.cpp
 * @brief Enhanced benchmark command with all algorithms and tabulate tables
 */

#include "filevault/cli/commands/benchmark_cmd.hpp"
#include "filevault/utils/console.hpp"
#include "filevault/utils/crypto_utils.hpp"
#include "filevault/compression/compressor.hpp"
#include "filevault/algorithms/pqc/post_quantum.hpp"
#include "filevault/algorithms/asymmetric/rsa.hpp"
#include "filevault/algorithms/asymmetric/ecc.hpp"
#include <spdlog/spdlog.h>
#include <tabulate/table.hpp>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <numeric>
#include <algorithm>

namespace filevault {
namespace cli {

namespace {

tabulate::Table create_benchmark_table(const std::vector<std::string>& headers) {
    tabulate::Table table;
    tabulate::Table::Row_t header_row;
    for (const auto& h : headers) {
        header_row.push_back(h);
    }
    table.add_row(header_row);
    
    table[0].format()
        .font_color(tabulate::Color::cyan)
        .font_align(tabulate::FontAlign::center)
        .font_style({tabulate::FontStyle::bold});
    
    return table;
}

void print_benchmark_section(const std::string& title, const std::string& emoji = "") {
    fmt::print("\n");
    fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::cyan), 
              "{} {}\n", emoji, title);
}

std::string format_mbps(double mbps) {
    return fmt::format("{:.2f} MB/s", mbps);
}

std::string format_ms(double ms) {
    return fmt::format("{:.2f} ms", ms);
}

} // anonymous namespace

BenchmarkCommand::BenchmarkCommand(core::CryptoEngine& engine)
    : engine_(engine) {
}

void BenchmarkCommand::setup(CLI::App& app) {
    auto* cmd = app.add_subcommand(name(), description());
    
    cmd->add_option("-a,--algorithm", algorithm_, "Algorithm to benchmark (or 'all')");
    cmd->add_flag("--all", all_, "Benchmark all algorithms");
    cmd->add_option("-o,--output", output_file_, "Output JSON results to file");
    cmd->add_flag("--json", json_output_, "Output results in JSON format");
    cmd->add_option("-s,--size", data_size_, "Data size in bytes (default: 1MB)")->default_val(1048576);
    cmd->add_option("-i,--iterations", iterations_, "Number of iterations (default: 5)")->default_val(5);
    cmd->add_flag("--pqc", pqc_only_, "Only benchmark Post-Quantum algorithms");
    cmd->add_flag("--symmetric", symmetric_only_, "Only benchmark symmetric algorithms");
    cmd->add_flag("--asymmetric", asymmetric_only_, "Only benchmark asymmetric algorithms");
    cmd->add_flag("--hash", hash_only_, "Only benchmark hash functions");
    cmd->add_flag("--kdf", kdf_only_, "Only benchmark key derivation functions");
    cmd->add_flag("--compression", compression_only_, "Only benchmark compression algorithms");
    
    cmd->callback([this]() { execute(); });
}

int BenchmarkCommand::execute() {
    try {
        if (!json_output_) {
            utils::Console::header("FileVault Performance Benchmark");
            fmt::print("Data size: {}, Iterations: {}\n\n", 
                       utils::CryptoUtils::format_bytes(data_size_), iterations_);
        }
        
        nlohmann::json json_results;
        json_results["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
        json_results["platform"] = get_platform_info();
        json_results["data_size"] = data_size_;
        json_results["iterations"] = iterations_;
        
        // Run benchmarks based on flags and algorithm filter
        if (hash_only_) {
            benchmark_hash(json_results);
        } else if (kdf_only_) {
            benchmark_kdf(json_results);
        } else if (compression_only_) {
            benchmark_compression(json_results);
        } else if (!algorithm_.empty() && algorithm_ != "all") {
            // Specific algorithm - determine type and run only that category
            std::string algo_lower = algorithm_;
            std::transform(algo_lower.begin(), algo_lower.end(), algo_lower.begin(), ::tolower);
            
            if (algo_lower.find("sha") != std::string::npos || 
                algo_lower.find("blake") != std::string::npos ||
                algo_lower.find("md5") != std::string::npos) {
                benchmark_hash(json_results);
            } else if (algo_lower.find("argon") != std::string::npos ||
                      algo_lower.find("pbkdf") != std::string::npos ||
                      algo_lower.find("scrypt") != std::string::npos) {
                benchmark_kdf(json_results);
            } else if (algo_lower.find("zlib") != std::string::npos ||
                      algo_lower.find("bzip") != std::string::npos ||
                      algo_lower.find("lzma") != std::string::npos) {
                benchmark_compression(json_results);
            } else {
                // Assume symmetric/asymmetric/pqc
                benchmark_symmetric(json_results);
                benchmark_asymmetric(json_results);
                benchmark_pqc(json_results);
            }
        } else if (pqc_only_) {
            benchmark_pqc(json_results);
        } else if (symmetric_only_) {
            benchmark_symmetric(json_results);
        } else if (asymmetric_only_) {
            benchmark_asymmetric(json_results);
        } else {
            // Default: all benchmarks
            benchmark_symmetric(json_results);
            benchmark_asymmetric(json_results);
            benchmark_pqc(json_results);
            benchmark_kdf(json_results);
            benchmark_compression(json_results);
            benchmark_hash(json_results);
        }
        
        // Save output if requested
        if (!output_file_.empty() || json_output_) {
            save_json_output(json_results);
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        utils::Console::error(fmt::format("Benchmark failed: {}", e.what()));
        return 1;
    }
}

std::string BenchmarkCommand::get_platform_info() {
#ifdef _WIN32
    return "Windows";
#elif __linux__
    return "Linux";
#elif __APPLE__
    return "macOS";
#else
    return "Unknown";
#endif
}

void BenchmarkCommand::benchmark_symmetric(nlohmann::json& json_results) {
    if (!json_output_) {
        print_benchmark_section("SYMMETRIC ENCRYPTION ALGORITHMS", "🔐");
    }
    
    json_results["symmetric"] = nlohmann::json::array();
    
    // AEAD Algorithms
    if (!json_output_) {
        fmt::print("\n📦 AEAD (Authenticated Encryption):\n");
    }
    
    tabulate::Table aead_table = create_benchmark_table({"Algorithm", "Encrypt", "Decrypt", "Notes"});
    
    std::vector<std::pair<core::AlgorithmType, std::string>> aead_algos = {
        {core::AlgorithmType::AES_128_GCM, "NIST Standard"},
        {core::AlgorithmType::AES_192_GCM, "NIST Standard"},
        {core::AlgorithmType::AES_256_GCM, "Recommended"},
        {core::AlgorithmType::CHACHA20_POLY1305, "RFC 8439"},
        {core::AlgorithmType::SERPENT_256_GCM, "AES Finalist"},
        {core::AlgorithmType::TWOFISH_128_GCM, "AES Finalist"},
        {core::AlgorithmType::TWOFISH_192_GCM, "AES Finalist"},
        {core::AlgorithmType::TWOFISH_256_GCM, "AES Finalist"},
        {core::AlgorithmType::CAMELLIA_128_GCM, "ISO 18033-3"},
        {core::AlgorithmType::CAMELLIA_192_GCM, "ISO 18033-3"},
        {core::AlgorithmType::CAMELLIA_256_GCM, "ISO 18033-3"},
        {core::AlgorithmType::ARIA_128_GCM, "Korean Std"},
        {core::AlgorithmType::ARIA_192_GCM, "Korean Std"},
        {core::AlgorithmType::ARIA_256_GCM, "Korean Std"},
        {core::AlgorithmType::SM4_GCM, "Chinese Std"},
    };
    
    for (const auto& [algo_type, notes] : aead_algos) {
        auto result = benchmark_algorithm(algo_type);
        if (result.success) {
            aead_table.add_row({result.algorithm, format_mbps(result.encrypt_mbps), 
                               format_mbps(result.decrypt_mbps), notes});
            json_results["symmetric"].push_back({
                {"algorithm", result.algorithm},
                {"type", "AEAD"},
                {"encrypt_mbps", result.encrypt_mbps},
                {"decrypt_mbps", result.decrypt_mbps},
                {"encrypt_ms", result.encrypt_ms},
                {"decrypt_ms", result.decrypt_ms}
            });
        }
    }
    
    if (!json_output_) {
        std::cout << aead_table << std::endl;
    }
    
    // Non-AEAD Block Cipher Modes
    if (!json_output_) {
        fmt::print("\n📦 Block Cipher Modes (Non-AEAD):\n");
    }
    
    tabulate::Table block_table = create_benchmark_table({"Algorithm", "Encrypt", "Decrypt", "Notes"});
    
    std::vector<std::pair<core::AlgorithmType, std::string>> block_modes = {
        {core::AlgorithmType::AES_128_CBC, "Legacy"},
        {core::AlgorithmType::AES_192_CBC, "Legacy"},
        {core::AlgorithmType::AES_256_CBC, "Legacy"},
        {core::AlgorithmType::AES_128_CTR, "Stream"},
        {core::AlgorithmType::AES_192_CTR, "Stream"},
        {core::AlgorithmType::AES_256_CTR, "Stream"},
        {core::AlgorithmType::AES_128_CFB, "Stream"},
        {core::AlgorithmType::AES_192_CFB, "Stream"},
        {core::AlgorithmType::AES_256_CFB, "Stream"},
        {core::AlgorithmType::AES_128_OFB, "Stream"},
        {core::AlgorithmType::AES_192_OFB, "Stream"},
        {core::AlgorithmType::AES_256_OFB, "Stream"},
        {core::AlgorithmType::AES_128_XTS, "Disk Enc"},
        {core::AlgorithmType::AES_256_XTS, "Disk Enc"},
        {core::AlgorithmType::AES_128_ECB, "INSECURE"},
        {core::AlgorithmType::AES_192_ECB, "INSECURE"},
        {core::AlgorithmType::AES_256_ECB, "INSECURE"},
        {core::AlgorithmType::TRIPLE_DES_CBC, "Legacy"},
    };
    
    size_t row_index = 1;
    for (const auto& [algo_type, notes] : block_modes) {
        auto result = benchmark_algorithm(algo_type);
        if (result.success) {
            block_table.add_row({result.algorithm, format_mbps(result.encrypt_mbps), 
                                format_mbps(result.decrypt_mbps), notes});
            
            // Mark insecure algorithms in red
            if (notes == "INSECURE") {
                block_table[row_index].format().font_color(tabulate::Color::red);
            }
            row_index++;
            
            json_results["symmetric"].push_back({
                {"algorithm", result.algorithm},
                {"type", "Block"},
                {"encrypt_mbps", result.encrypt_mbps},
                {"decrypt_mbps", result.decrypt_mbps},
                {"encrypt_ms", result.encrypt_ms},
                {"decrypt_ms", result.decrypt_ms}
            });
        }
    }
    
    if (!json_output_) {
        std::cout << block_table << std::endl;
    }
}

void BenchmarkCommand::benchmark_asymmetric(nlohmann::json& json_results) {
    if (!json_output_) {
        print_benchmark_section("ASYMMETRIC ENCRYPTION ALGORITHMS", "🔑");
    }
    
    tabulate::Table table = create_benchmark_table({"Algorithm", "KeyGen", "Encrypt", "Decrypt", "Security"});
    
    json_results["asymmetric"] = nlohmann::json::array();
    
    // RSA
    std::vector<std::pair<core::AlgorithmType, std::string>> rsa_algos = {
        {core::AlgorithmType::RSA_2048, "112-bit"},
        {core::AlgorithmType::RSA_3072, "128-bit"},
        {core::AlgorithmType::RSA_4096, "140-bit"},
    };
    
    for (const auto& [algo_type, security] : rsa_algos) {
        auto result = benchmark_asymmetric_algorithm(algo_type);
        if (result.success) {
            table.add_row({result.algorithm, format_ms(result.keygen_ms), 
                          format_ms(result.encrypt_ms), format_ms(result.decrypt_ms), security});
            json_results["asymmetric"].push_back({
                {"algorithm", result.algorithm},
                {"type", "RSA"},
                {"keygen_ms", result.keygen_ms},
                {"encrypt_ms", result.encrypt_ms},
                {"decrypt_ms", result.decrypt_ms},
                {"security", security}
            });
        }
    }
    
    // ECC
    std::vector<std::pair<core::AlgorithmType, std::string>> ecc_algos = {
        {core::AlgorithmType::ECC_P256, "128-bit"},
        {core::AlgorithmType::ECC_P384, "192-bit"},
        {core::AlgorithmType::ECC_P521, "256-bit"},
    };
    
    for (const auto& [algo_type, security] : ecc_algos) {
        auto result = benchmark_asymmetric_algorithm(algo_type);
        if (result.success) {
            table.add_row({result.algorithm, format_ms(result.keygen_ms), 
                          format_ms(result.encrypt_ms), format_ms(result.decrypt_ms), security});
            json_results["asymmetric"].push_back({
                {"algorithm", result.algorithm},
                {"type", "ECC"},
                {"keygen_ms", result.keygen_ms},
                {"encrypt_ms", result.encrypt_ms},
                {"decrypt_ms", result.decrypt_ms},
                {"security", security}
            });
        }
    }
    
    if (!json_output_) {
        std::cout << table << std::endl;
    }
}

void BenchmarkCommand::benchmark_pqc(nlohmann::json& json_results) {
    if (!json_output_) {
        print_benchmark_section("POST-QUANTUM CRYPTOGRAPHY (NIST Standards)", "🔮");
    }
    
    json_results["pqc"] = nlohmann::json::array();
    
    // Kyber KEM
    if (!json_output_) {
        fmt::print("\n🔐 ML-KEM (Kyber) - Key Encapsulation:\n");
    }
    
    tabulate::Table kyber_table = create_benchmark_table({"Variant", "KeyGen", "Encaps", "Decaps", "Security"});
    
    std::vector<std::pair<core::AlgorithmType, std::string>> kyber_algos = {
        {core::AlgorithmType::KYBER_512, "NIST-1"},
        {core::AlgorithmType::KYBER_768, "NIST-3"},
        {core::AlgorithmType::KYBER_1024, "NIST-5"},
    };
    
    for (const auto& [algo_type, security] : kyber_algos) {
        auto result = benchmark_pqc_algorithm(algo_type);
        if (result.success) {
            kyber_table.add_row({result.algorithm, format_ms(result.keygen_ms), 
                                format_ms(result.encaps_ms), format_ms(result.decaps_ms), security});
            json_results["pqc"].push_back({
                {"algorithm", result.algorithm},
                {"type", "KEM"},
                {"keygen_ms", result.keygen_ms},
                {"encaps_ms", result.encaps_ms},
                {"decaps_ms", result.decaps_ms},
                {"security", security}
            });
        }
    }
    
    if (!json_output_) {
        std::cout << kyber_table << std::endl;
    }
    
    // KyberHybrid
    if (!json_output_) {
        fmt::print("\n🔐 ML-KEM Hybrid (Kyber + AES-256-GCM):\n");
    }
    
    tabulate::Table hybrid_table = create_benchmark_table({"Variant", "Encrypt", "Decrypt", "Security"});
    
    std::vector<std::pair<core::AlgorithmType, std::string>> hybrid_algos = {
        {core::AlgorithmType::KYBER_512_HYBRID, "NIST-1"},
        {core::AlgorithmType::KYBER_768_HYBRID, "NIST-3"},
        {core::AlgorithmType::KYBER_1024_HYBRID, "NIST-5"},
    };
    
    for (const auto& [algo_type, security] : hybrid_algos) {
        auto result = benchmark_hybrid_algorithm(algo_type);
        if (result.success) {
            hybrid_table.add_row({result.algorithm, format_mbps(result.encrypt_mbps), 
                                 format_mbps(result.decrypt_mbps), security});
            json_results["pqc"].push_back({
                {"algorithm", result.algorithm},
                {"type", "Hybrid"},
                {"encrypt_mbps", result.encrypt_mbps},
                {"decrypt_mbps", result.decrypt_mbps},
                {"security", security}
            });
        }
    }
    
    if (!json_output_) {
        std::cout << hybrid_table << std::endl;
    }
    
    // Dilithium Signatures
    if (!json_output_) {
        fmt::print("\n✍️  ML-DSA (Dilithium) - Digital Signatures:\n");
    }
    
    tabulate::Table dil_table = create_benchmark_table({"Variant", "KeyGen", "Sign", "Verify", "Security"});
    
    std::vector<std::pair<core::AlgorithmType, std::string>> dilithium_algos = {
        {core::AlgorithmType::DILITHIUM_2, "NIST-2"},
        {core::AlgorithmType::DILITHIUM_3, "NIST-3"},
        {core::AlgorithmType::DILITHIUM_5, "NIST-5"},
    };
    
    for (const auto& [algo_type, security] : dilithium_algos) {
        auto result = benchmark_signature_algorithm(algo_type);
        if (result.success) {
            dil_table.add_row({result.algorithm, format_ms(result.keygen_ms), 
                              format_ms(result.sign_ms), format_ms(result.verify_ms), security});
            json_results["pqc"].push_back({
                {"algorithm", result.algorithm},
                {"type", "Signature"},
                {"keygen_ms", result.keygen_ms},
                {"sign_ms", result.sign_ms},
                {"verify_ms", result.verify_ms},
                {"security", security}
            });
        }
    }
    
    if (!json_output_) {
        std::cout << dil_table << std::endl;
    }
}

void BenchmarkCommand::benchmark_kdf(nlohmann::json& json_results) {
    if (!json_output_) {
        print_benchmark_section("KEY DERIVATION FUNCTIONS", "🔑");
    }
    
    tabulate::Table table = create_benchmark_table({"Algorithm", "Time (ms)", "Rate (/s)", "Memory"});
    
    json_results["kdf"] = nlohmann::json::array();
    
    std::string password = "benchmark_password_123!@#";
    auto salt = engine_.generate_salt(32);
    
    std::vector<std::tuple<core::KDFType, std::string, std::string>> kdfs = {
        {core::KDFType::ARGON2ID, "Argon2id", "65 MB"},
        {core::KDFType::PBKDF2_SHA256, "PBKDF2-SHA256", "Minimal"},
        {core::KDFType::SCRYPT, "scrypt", "32 MB"},
    };
    
    for (const auto& [kdf, name, memory] : kdfs) {
        core::EncryptionConfig config;
        config.kdf = kdf;
        config.level = core::SecurityLevel::WEAK;  // Fast for benchmark
        config.apply_security_level();
        
        // Warm-up
        engine_.derive_key(password, salt, config);
        
        // Benchmark
        std::vector<double> times;
        for (int i = 0; i < iterations_; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            auto key = engine_.derive_key(password, salt, config);
            auto end = std::chrono::high_resolution_clock::now();
            times.push_back(std::chrono::duration<double, std::milli>(end - start).count());
        }
        
        double avg_time = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
        double rate = 1000.0 / avg_time;
        
        table.add_row({name, format_ms(avg_time), fmt::format("{:.1f}", rate), memory});
        
        json_results["kdf"].push_back({
            {"algorithm", name},
            {"time_ms", avg_time},
            {"rate_per_sec", rate},
            {"memory", memory}
        });
    }
    
    if (!json_output_) {
        std::cout << table << std::endl;
    }
}

void BenchmarkCommand::benchmark_compression(nlohmann::json& json_results) {
    if (!json_output_) {
        print_benchmark_section("COMPRESSION ALGORITHMS", "📦");
    }
    
    tabulate::Table table = create_benchmark_table({"Algorithm", "Compress", "Decompress", "Ratio"});
    
    json_results["compression"] = nlohmann::json::array();
    
    // Generate compressible test data
    std::vector<uint8_t> test_data(data_size_);
    for (size_t i = 0; i < test_data.size(); ++i) {
        test_data[i] = static_cast<uint8_t>((i % 256) ^ ((i / 256) % 256));
    }
    
    std::vector<std::pair<core::CompressionType, std::string>> compressors = {
        {core::CompressionType::ZLIB, "ZLIB"},
        {core::CompressionType::BZIP2, "BZIP2"},
        {core::CompressionType::LZMA, "LZMA"},
    };
    
    for (const auto& [type, name] : compressors) {
        try {
            auto comp = compression::CompressionService::create(type);
            if (!comp) continue;
            
            // Warm-up
            auto compressed = comp->compress(test_data, 6);
            
            // Benchmark compression
            std::vector<double> compress_times;
            compression::CompressionResult compressed_result;
            for (int i = 0; i < iterations_; ++i) {
                auto start = std::chrono::high_resolution_clock::now();
                compressed_result = comp->compress(test_data, 6);
                auto end = std::chrono::high_resolution_clock::now();
                compress_times.push_back(std::chrono::duration<double, std::milli>(end - start).count());
            }
            
            // Benchmark decompression
            std::vector<double> decompress_times;
            for (int i = 0; i < iterations_; ++i) {
                auto start = std::chrono::high_resolution_clock::now();
                auto decompressed = comp->decompress(compressed_result.data);
                auto end = std::chrono::high_resolution_clock::now();
                decompress_times.push_back(std::chrono::duration<double, std::milli>(end - start).count());
            }
            
            double avg_compress = std::accumulate(compress_times.begin(), compress_times.end(), 0.0) / compress_times.size();
            double avg_decompress = std::accumulate(decompress_times.begin(), decompress_times.end(), 0.0) / decompress_times.size();
            double compress_mbps = (data_size_ / 1024.0 / 1024.0) / (avg_compress / 1000.0);
            double decompress_mbps = (data_size_ / 1024.0 / 1024.0) / (avg_decompress / 1000.0);
            double ratio = static_cast<double>(test_data.size()) / compressed_result.data.size();
            
            table.add_row({name, format_mbps(compress_mbps), format_mbps(decompress_mbps), 
                          fmt::format("{:.2f}x", ratio)});
            
            json_results["compression"].push_back({
                {"algorithm", name},
                {"compress_mbps", compress_mbps},
                {"decompress_mbps", decompress_mbps},
                {"ratio", ratio}
            });
        } catch (const std::exception& e) {
            table.add_row({name, "Error", e.what(), "-"});
        }
    }
    
    if (!json_output_) {
        std::cout << table << std::endl;
    }
}

void BenchmarkCommand::benchmark_hash(nlohmann::json& json_results) {
    if (!json_output_) {
        print_benchmark_section("HASH FUNCTIONS", "🔢");
    }
    
    tabulate::Table table = create_benchmark_table({"Algorithm", "Throughput", "Digest"});
    
    json_results["hash"] = nlohmann::json::array();
    
    std::vector<uint8_t> test_data(data_size_, 0x42);
    
    std::vector<std::tuple<std::string, std::string, int>> hash_algos = {
        {"SHA-256", "SHA-256", 32},
        {"SHA-384", "SHA-384", 48},
        {"SHA-512", "SHA-512", 64},
        {"SHA3-256", "SHA-3(256)", 32},
        {"SHA3-512", "SHA-3(512)", 64},
        {"BLAKE2b", "BLAKE2b(512)", 64},
    };
    
    for (const auto& [name, botan_name, digest_size] : hash_algos) {
        try {
            auto hasher = Botan::HashFunction::create(botan_name);
            if (!hasher) continue;
            
            // Warm-up
            hasher->update(test_data.data(), test_data.size());
            auto digest = hasher->final();
            
            // Benchmark
            std::vector<double> times;
            for (int i = 0; i < iterations_; ++i) {
                hasher->clear();
                auto start = std::chrono::high_resolution_clock::now();
                hasher->update(test_data.data(), test_data.size());
                digest = hasher->final();
                auto end = std::chrono::high_resolution_clock::now();
                times.push_back(std::chrono::duration<double, std::milli>(end - start).count());
            }
            
            double avg_time = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
            double mbps = (data_size_ / 1024.0 / 1024.0) / (avg_time / 1000.0);
            
            table.add_row({name, format_mbps(mbps), fmt::format("{} bits", digest_size * 8)});
            
            json_results["hash"].push_back({
                {"algorithm", name},
                {"throughput_mbps", mbps},
                {"digest_bits", digest_size * 8}
            });
        } catch (...) {
            // Skip unavailable algorithms
        }
    }
    
    if (!json_output_) {
        std::cout << table << std::endl;
    }
}

BenchmarkResult BenchmarkCommand::benchmark_algorithm(core::AlgorithmType algo_type) {
    BenchmarkResult result;
    result.algorithm = engine_.algorithm_name(algo_type);
    
    auto* algo = engine_.get_algorithm(algo_type);
    if (!algo) {
        result.success = false;
        return result;
    }
    
    // Prepare test data
    std::vector<uint8_t> plaintext(data_size_, 0x42);
    std::vector<uint8_t> key(32, 0x00);
    
    core::EncryptionConfig config;
    config.nonce = engine_.generate_nonce(12);
    
    // Warm-up
    auto enc_result = algo->encrypt(plaintext, key, config);
    if (!enc_result.success) {
        result.success = false;
        return result;
    }
    
    // Benchmark encryption
    std::vector<double> enc_times;
    core::CryptoResult last_enc_result;
    for (int i = 0; i < iterations_; ++i) {
        config.nonce = engine_.generate_nonce(12);
        auto start = std::chrono::high_resolution_clock::now();
        last_enc_result = algo->encrypt(plaintext, key, config);
        auto end = std::chrono::high_resolution_clock::now();
        enc_times.push_back(std::chrono::duration<double, std::milli>(end - start).count());
    }
    
    // Benchmark decryption
    std::vector<double> dec_times;
    core::EncryptionConfig dec_config = config;
    dec_config.nonce = last_enc_result.nonce;
    dec_config.tag = last_enc_result.tag;
    
    for (int i = 0; i < iterations_; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        auto dec_result = algo->decrypt(last_enc_result.data, key, dec_config);
        auto end = std::chrono::high_resolution_clock::now();
        dec_times.push_back(std::chrono::duration<double, std::milli>(end - start).count());
    }
    
    result.encrypt_ms = std::accumulate(enc_times.begin(), enc_times.end(), 0.0) / enc_times.size();
    result.decrypt_ms = std::accumulate(dec_times.begin(), dec_times.end(), 0.0) / dec_times.size();
    result.encrypt_mbps = (data_size_ / 1024.0 / 1024.0) / (result.encrypt_ms / 1000.0);
    result.decrypt_mbps = (data_size_ / 1024.0 / 1024.0) / (result.decrypt_ms / 1000.0);
    result.success = true;
    
    return result;
}

AsymmetricBenchmarkResult BenchmarkCommand::benchmark_asymmetric_algorithm(core::AlgorithmType algo_type) {
    AsymmetricBenchmarkResult result;
    result.success = false;
    
    // Small test data for asymmetric (limited by key size)
    std::vector<uint8_t> plaintext(64, 0x42);  // 64 bytes
    
    try {
        // RSA benchmarks
        if (algo_type == core::AlgorithmType::RSA_2048 ||
            algo_type == core::AlgorithmType::RSA_3072 ||
            algo_type == core::AlgorithmType::RSA_4096) {
            
            size_t key_bits = 2048;
            if (algo_type == core::AlgorithmType::RSA_3072) key_bits = 3072;
            else if (algo_type == core::AlgorithmType::RSA_4096) key_bits = 4096;
            
            algorithms::asymmetric::RSA rsa(key_bits);
            result.algorithm = rsa.name();
            
            // Benchmark KeyGen
            std::vector<double> keygen_times;
            algorithms::asymmetric::RSAKeyPair keypair;
            for (int i = 0; i < iterations_; ++i) {
                auto start = std::chrono::high_resolution_clock::now();
                keypair = rsa.generate_key_pair();
                auto end = std::chrono::high_resolution_clock::now();
                keygen_times.push_back(std::chrono::duration<double, std::milli>(end - start).count());
            }
            
            // Benchmark Encrypt
            std::vector<double> enc_times;
            core::EncryptionConfig config;
            core::CryptoResult enc_result;
            for (int i = 0; i < iterations_; ++i) {
                auto start = std::chrono::high_resolution_clock::now();
                enc_result = rsa.encrypt(plaintext, keypair.public_key, config);
                auto end = std::chrono::high_resolution_clock::now();
                enc_times.push_back(std::chrono::duration<double, std::milli>(end - start).count());
            }
            
            // Benchmark Decrypt
            std::vector<double> dec_times;
            if (enc_result.success) {
                for (int i = 0; i < iterations_; ++i) {
                    auto start = std::chrono::high_resolution_clock::now();
                    auto dec_result = rsa.decrypt(enc_result.data, keypair.private_key, config);
                    auto end = std::chrono::high_resolution_clock::now();
                    dec_times.push_back(std::chrono::duration<double, std::milli>(end - start).count());
                }
            }
            
            result.keygen_ms = std::accumulate(keygen_times.begin(), keygen_times.end(), 0.0) / keygen_times.size();
            result.encrypt_ms = enc_times.empty() ? 0 : std::accumulate(enc_times.begin(), enc_times.end(), 0.0) / enc_times.size();
            result.decrypt_ms = dec_times.empty() ? 0 : std::accumulate(dec_times.begin(), dec_times.end(), 0.0) / dec_times.size();
            result.success = true;
        }
        // ECC benchmarks
        else if (algo_type == core::AlgorithmType::ECC_P256 ||
                 algo_type == core::AlgorithmType::ECC_P384 ||
                 algo_type == core::AlgorithmType::ECC_P521) {
            
            algorithms::asymmetric::ECCurve curve = algorithms::asymmetric::ECCurve::SECP256R1;
            if (algo_type == core::AlgorithmType::ECC_P384) curve = algorithms::asymmetric::ECCurve::SECP384R1;
            else if (algo_type == core::AlgorithmType::ECC_P521) curve = algorithms::asymmetric::ECCurve::SECP521R1;
            
            algorithms::asymmetric::ECCHybrid ecc(curve);
            result.algorithm = ecc.name();
            
            // Benchmark KeyGen
            std::vector<double> keygen_times;
            algorithms::asymmetric::ECCKeyPair keypair;
            for (int i = 0; i < iterations_; ++i) {
                auto start = std::chrono::high_resolution_clock::now();
                keypair = ecc.generate_key_pair();
                auto end = std::chrono::high_resolution_clock::now();
                keygen_times.push_back(std::chrono::duration<double, std::milli>(end - start).count());
            }
            
            // Benchmark Encrypt (ECIES)
            std::vector<double> enc_times;
            core::EncryptionConfig config;
            core::CryptoResult enc_result;
            for (int i = 0; i < iterations_; ++i) {
                auto start = std::chrono::high_resolution_clock::now();
                enc_result = ecc.encrypt(plaintext, keypair.public_key, config);
                auto end = std::chrono::high_resolution_clock::now();
                enc_times.push_back(std::chrono::duration<double, std::milli>(end - start).count());
            }
            
            // Benchmark Decrypt
            std::vector<double> dec_times;
            if (enc_result.success) {
                for (int i = 0; i < iterations_; ++i) {
                    auto start = std::chrono::high_resolution_clock::now();
                    auto dec_result = ecc.decrypt(enc_result.data, keypair.private_key, config);
                    auto end = std::chrono::high_resolution_clock::now();
                    dec_times.push_back(std::chrono::duration<double, std::milli>(end - start).count());
                }
            }
            
            result.keygen_ms = std::accumulate(keygen_times.begin(), keygen_times.end(), 0.0) / keygen_times.size();
            result.encrypt_ms = enc_times.empty() ? 0 : std::accumulate(enc_times.begin(), enc_times.end(), 0.0) / enc_times.size();
            result.decrypt_ms = dec_times.empty() ? 0 : std::accumulate(dec_times.begin(), dec_times.end(), 0.0) / dec_times.size();
            result.success = true;
        }
    } catch (const std::exception& e) {
        spdlog::error("Asymmetric benchmark failed: {}", e.what());
    }
    
    return result;
}

PQCBenchmarkResult BenchmarkCommand::benchmark_pqc_algorithm(core::AlgorithmType algo_type) {
    PQCBenchmarkResult result;
    result.success = false;
    
    try {
        algorithms::pqc::Kyber::Variant variant;
        switch (algo_type) {
            case core::AlgorithmType::KYBER_512:
                variant = algorithms::pqc::Kyber::Variant::Kyber512;
                result.algorithm = "Kyber-512";
                break;
            case core::AlgorithmType::KYBER_768:
                variant = algorithms::pqc::Kyber::Variant::Kyber768;
                result.algorithm = "Kyber-768";
                break;
            case core::AlgorithmType::KYBER_1024:
                variant = algorithms::pqc::Kyber::Variant::Kyber1024;
                result.algorithm = "Kyber-1024";
                break;
            default:
                return result;
        }
        
        algorithms::pqc::Kyber kyber(variant);
        
        // Benchmark KeyGen
        std::vector<double> keygen_times;
        algorithms::pqc::PQKeyPair keypair;
        for (int i = 0; i < iterations_; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            keypair = kyber.generate_keypair();
            auto end = std::chrono::high_resolution_clock::now();
            keygen_times.push_back(std::chrono::duration<double, std::milli>(end - start).count());
        }
        
        // Benchmark Encapsulation
        std::vector<double> encaps_times;
        core::CryptoResult encap_result;
        core::EncryptionConfig config;
        for (int i = 0; i < iterations_; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            encap_result = kyber.encrypt({}, keypair.public_key, config);
            auto end = std::chrono::high_resolution_clock::now();
            encaps_times.push_back(std::chrono::duration<double, std::milli>(end - start).count());
        }
        
        // Benchmark Decapsulation
        std::vector<double> decaps_times;
        for (int i = 0; i < iterations_; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            auto decap_result = kyber.decrypt(encap_result.data, keypair.private_key, config);
            auto end = std::chrono::high_resolution_clock::now();
            decaps_times.push_back(std::chrono::duration<double, std::milli>(end - start).count());
        }
        
        result.keygen_ms = std::accumulate(keygen_times.begin(), keygen_times.end(), 0.0) / keygen_times.size();
        result.encaps_ms = std::accumulate(encaps_times.begin(), encaps_times.end(), 0.0) / encaps_times.size();
        result.decaps_ms = std::accumulate(decaps_times.begin(), decaps_times.end(), 0.0) / decaps_times.size();
        result.success = true;
        
    } catch (const std::exception& e) {
        spdlog::error("PQC benchmark failed: {}", e.what());
    }
    
    return result;
}

BenchmarkResult BenchmarkCommand::benchmark_hybrid_algorithm(core::AlgorithmType algo_type) {
    BenchmarkResult result;
    result.success = false;
    
    try {
        algorithms::pqc::Kyber::Variant variant;
        switch (algo_type) {
            case core::AlgorithmType::KYBER_512_HYBRID:
                variant = algorithms::pqc::Kyber::Variant::Kyber512;
                result.algorithm = "Kyber-512-Hybrid";
                break;
            case core::AlgorithmType::KYBER_768_HYBRID:
                variant = algorithms::pqc::Kyber::Variant::Kyber768;
                result.algorithm = "Kyber-768-Hybrid";
                break;
            case core::AlgorithmType::KYBER_1024_HYBRID:
                variant = algorithms::pqc::Kyber::Variant::Kyber1024;
                result.algorithm = "Kyber-1024-Hybrid";
                break;
            default:
                return result;
        }
        
        algorithms::pqc::KyberHybrid hybrid(variant);
        
        // Generate keypair
        auto keypair = hybrid.generate_keypair();
        
        // Prepare test data
        std::vector<uint8_t> plaintext(data_size_, 0x42);
        
        // Benchmark encryption
        std::vector<double> enc_times;
        core::CryptoResult enc_result;
        core::EncryptionConfig config;
        for (int i = 0; i < iterations_; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            enc_result = hybrid.encrypt(plaintext, keypair.public_key, config);
            auto end = std::chrono::high_resolution_clock::now();
            enc_times.push_back(std::chrono::duration<double, std::milli>(end - start).count());
        }
        
        // Benchmark decryption
        std::vector<double> dec_times;
        for (int i = 0; i < iterations_; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            auto dec_result = hybrid.decrypt(enc_result.data, keypair.private_key, config);
            auto end = std::chrono::high_resolution_clock::now();
            dec_times.push_back(std::chrono::duration<double, std::milli>(end - start).count());
        }
        
        result.encrypt_ms = std::accumulate(enc_times.begin(), enc_times.end(), 0.0) / enc_times.size();
        result.decrypt_ms = std::accumulate(dec_times.begin(), dec_times.end(), 0.0) / dec_times.size();
        result.encrypt_mbps = (data_size_ / 1024.0 / 1024.0) / (result.encrypt_ms / 1000.0);
        result.decrypt_mbps = (data_size_ / 1024.0 / 1024.0) / (result.decrypt_ms / 1000.0);
        result.success = true;
        
    } catch (const std::exception& e) {
        spdlog::error("Hybrid benchmark failed: {}", e.what());
    }
    
    return result;
}

SignatureBenchmarkResult BenchmarkCommand::benchmark_signature_algorithm(core::AlgorithmType algo_type) {
    SignatureBenchmarkResult result;
    result.success = false;
    
    try {
        algorithms::pqc::Dilithium::Variant variant;
        switch (algo_type) {
            case core::AlgorithmType::DILITHIUM_2:
                variant = algorithms::pqc::Dilithium::Variant::Dilithium2;
                result.algorithm = "Dilithium-2";
                break;
            case core::AlgorithmType::DILITHIUM_3:
                variant = algorithms::pqc::Dilithium::Variant::Dilithium3;
                result.algorithm = "Dilithium-3";
                break;
            case core::AlgorithmType::DILITHIUM_5:
                variant = algorithms::pqc::Dilithium::Variant::Dilithium5;
                result.algorithm = "Dilithium-5";
                break;
            default:
                return result;
        }
        
        algorithms::pqc::Dilithium dilithium(variant);
        
        // Benchmark KeyGen
        std::vector<double> keygen_times;
        algorithms::pqc::PQKeyPair keypair;
        for (int i = 0; i < iterations_; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            keypair = dilithium.generate_keypair();
            auto end = std::chrono::high_resolution_clock::now();
            keygen_times.push_back(std::chrono::duration<double, std::milli>(end - start).count());
        }
        
        // Test message
        std::vector<uint8_t> message(1024, 0x42);  // 1KB test message
        
        // Benchmark Sign
        std::vector<double> sign_times;
        std::vector<uint8_t> signature;
        for (int i = 0; i < iterations_; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            signature = dilithium.sign(message, keypair.private_key);
            auto end = std::chrono::high_resolution_clock::now();
            sign_times.push_back(std::chrono::duration<double, std::milli>(end - start).count());
        }
        
        // Benchmark Verify
        std::vector<double> verify_times;
        for (int i = 0; i < iterations_; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            bool valid = dilithium.verify(message, signature, keypair.public_key);
            auto end = std::chrono::high_resolution_clock::now();
            verify_times.push_back(std::chrono::duration<double, std::milli>(end - start).count());
            (void)valid;
        }
        
        result.keygen_ms = std::accumulate(keygen_times.begin(), keygen_times.end(), 0.0) / keygen_times.size();
        result.sign_ms = std::accumulate(sign_times.begin(), sign_times.end(), 0.0) / sign_times.size();
        result.verify_ms = std::accumulate(verify_times.begin(), verify_times.end(), 0.0) / verify_times.size();
        result.success = true;
        
    } catch (const std::exception& e) {
        spdlog::error("Signature benchmark failed: {}", e.what());
    }
    
    return result;
}

void BenchmarkCommand::save_json_output(const nlohmann::json& results) {
    std::filesystem::create_directories("benchmarks");
    
    // Generate filename with timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    
    std::string filename = output_file_.empty() 
        ? fmt::format("benchmarks/benchmark_{}.json", ss.str())
        : output_file_;
    
    std::ofstream file(filename);
    file << results.dump(2);
    file.close();
    
    if (!json_output_) {
        fmt::print("✓ Benchmark results saved to: {}\n", filename);
    }
}

void BenchmarkCommand::save_log_output(const std::string& log_content) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    
    std::string filename = fmt::format("benchmarks/benchmark_{}.log", ss.str());
    
    std::ofstream file(filename);
    file << log_content;
    file.close();
    
    fmt::print("✓ Log saved to: {}\n", filename);
}

} // namespace cli
} // namespace filevault
