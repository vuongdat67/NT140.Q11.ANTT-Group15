#include "filevault/cli/commands/benchmark_cmd.hpp"
#include "filevault/utils/console.hpp"
#include "filevault/utils/crypto_utils.hpp"
#include <chrono>
#include <iomanip>
#include <fstream>
#include <filesystem>

namespace filevault {
namespace cli {

BenchmarkCommand::BenchmarkCommand(core::CryptoEngine& engine)
    : engine_(engine) {
}

void BenchmarkCommand::setup(CLI::App& app) {
    auto* cmd = app.add_subcommand(name(), description());
    
    cmd->add_option("-a,--algorithm", algorithm_, "Algorithm to benchmark");
    cmd->add_flag("--all", all_, "Benchmark all algorithms");
    cmd->add_option("-o,--output", output_file_, "Output JSON results to file");
    cmd->add_flag("--json", json_output_, "Output results in JSON format");
    
    cmd->callback([this]() { execute(); });
}

int BenchmarkCommand::execute() {
    try {
        if (!json_output_) {
            utils::Console::header("FileVault Performance Benchmark");
        }
        
        nlohmann::json json_results;
        json_results["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
        json_results["platform"] = "Windows MSYS2 UCRT64";
        
        // Run benchmarks
        benchmark_encryption();
        benchmark_kdf();
        benchmark_compression();
        
        // Save output if requested
        if (!output_file_.empty()) {
            // Prepare JSON data
            json_results["encryption"] = nlohmann::json::array();
            for (const auto& res : results_) {
                json_results["encryption"].push_back({
                    {"algorithm", res.algorithm},
                    {"data_size", res.data_size},
                    {"time_ms", res.time_ms},
                    {"throughput_mbps", res.throughput_mbps}
                });
            }
            save_json_output(json_results);
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        utils::Console::error(fmt::format("Benchmark failed: {}", e.what()));
        return 1;
    }
}

void BenchmarkCommand::benchmark_encryption() {
    // Test data sizes: 1KB, 10KB, 100KB, 1MB, 10MB
    std::vector<size_t> sizes = {1024, 10240, 102400, 1048576, 10485760};
    
    if (!json_output_) {
        fmt::print("\n📊 Encryption Performance:\n\n");
        fmt::print("{:<12} │ {:<15} │ {:<15} │ {:<15} │ {:<15}\n", 
                   "Size", "AES-128-GCM", "AES-192-GCM", "AES-256-GCM", "ChaCha20-Poly");
        fmt::print("{:-<12}─┼─{:-<15}─┼─{:-<15}─┼─{:-<15}─┼─{:-<15}\n", "", "", "", "", "");
    }
    
    for (auto size : sizes) {
        std::vector<uint8_t> plaintext(size, 0x42);
        std::vector<uint8_t> key(32, 0x00);
        
        if (!json_output_) {
            fmt::print("{:<12} │ ", utils::CryptoUtils::format_bytes(size));
        }
        
        // Test algorithms
        std::vector<core::AlgorithmType> algos = {
            core::AlgorithmType::AES_128_GCM,
            core::AlgorithmType::AES_192_GCM,
            core::AlgorithmType::AES_256_GCM,
            core::AlgorithmType::CHACHA20_POLY1305
        };
        
        int algo_count = 0;
        for (auto algo_type : algos) {
            auto* algo = engine_.get_algorithm(algo_type);
            if (!algo) {
                if (!json_output_) {
                    fmt::print("{:<15} │ ", "N/A");
                }
                algo_count++;
                continue;
            }
            
            core::EncryptionConfig config;
            config.nonce = engine_.generate_nonce(12);
            
            // Warm-up
            algo->encrypt(plaintext, key, config);
            
            // Benchmark (5 runs)
            double total_time = 0.0;
            for (int i = 0; i < 5; ++i) {
                config.nonce = engine_.generate_nonce(12);
                auto start = std::chrono::high_resolution_clock::now();
                auto result = algo->encrypt(plaintext, key, config);
                auto end = std::chrono::high_resolution_clock::now();
                
                if (!result.success) continue;
                total_time += std::chrono::duration<double, std::milli>(end - start).count();
            }
            
            double avg_time = total_time / 5.0;
            double throughput = (size / 1024.0 / 1024.0) / (avg_time / 1000.0);
            
            if (!json_output_) {
                fmt::print("{:<15}", fmt::format("{:.2f} MB/s", throughput));
                if (algo_count < 3) {
                    fmt::print(" │ ");
                }
            }
            
            // Store result
            results_.push_back({
                engine_.algorithm_name(algo_type),
                size,
                avg_time,
                throughput
            });
            
            algo_count++;
        }
        
        if (!json_output_) {
            fmt::print("\n");
        }
    }
}

void BenchmarkCommand::benchmark_kdf() {
    if (!json_output_) {
        fmt::print("\n⚡ KDF Performance (256-bit key):\n\n");
        fmt::print("{:<18} │ {:<12} │ {:<12} │ {:<10}\n", 
                   "Algorithm", "Time (ms)", "Rate (/s)", "Memory");
        fmt::print("{:-<18}─┼─{:-<12}─┼─{:-<12}─┼─{:-<10}\n", "", "", "", "");
    }
    
    std::string password = "test_password_123";
    auto salt = engine_.generate_salt(32);
    
    std::vector<core::KDFType> kdfs = {
        core::KDFType::ARGON2ID,
        core::KDFType::PBKDF2_SHA256
        // TODO: Fix Scrypt parameters before adding back
    };
    
    for (auto kdf : kdfs) {
        core::EncryptionConfig config;
        config.kdf = kdf;
        config.level = core::SecurityLevel::WEAK;
        config.apply_security_level();
        
        auto start = std::chrono::high_resolution_clock::now();
        auto key = engine_.derive_key(password, salt, config);
        auto end = std::chrono::high_resolution_clock::now();
        
        double time_ms = std::chrono::duration<double, std::milli>(end - start).count();
        double rate = 1000.0 / time_ms;
        
        std::string memory = (kdf == core::KDFType::ARGON2ID) ? "65 MB" : "N/A";
        
        if (!json_output_) {
            fmt::print("{:<18} │ {:<12} │ {:<12} │ {:<10}\n",
                      engine_.kdf_name(kdf),
                      fmt::format("{:.2f}", time_ms),
                      fmt::format("{:.1f}", rate),
                      memory);
        }
    }
}

void BenchmarkCommand::benchmark_compression() {
    // Compression benchmarking coming soon
    // TODO: Add compression benchmarks when ICompressor interface is ready
    if (!json_output_) {
        fmt::print("\n");
    }
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
    
    fmt::print("\n✓ Benchmark results saved to: {}\n", filename);
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
