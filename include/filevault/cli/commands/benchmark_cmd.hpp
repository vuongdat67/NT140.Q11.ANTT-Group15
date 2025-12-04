#ifndef FILEVAULT_CLI_COMMANDS_BENCHMARK_CMD_HPP
#define FILEVAULT_CLI_COMMANDS_BENCHMARK_CMD_HPP

#include "filevault/cli/command.hpp"
#include "filevault/core/crypto_engine.hpp"
#include <nlohmann/json.hpp>
#include <botan/hash.h>

namespace filevault {
namespace cli {

struct BenchmarkResult {
    std::string algorithm;
    double encrypt_ms = 0;
    double decrypt_ms = 0;
    double encrypt_mbps = 0;
    double decrypt_mbps = 0;
    bool success = false;
};

struct AsymmetricBenchmarkResult {
    std::string algorithm;
    double keygen_ms = 0;
    double encrypt_ms = 0;
    double decrypt_ms = 0;
    bool success = false;
};

struct PQCBenchmarkResult {
    std::string algorithm;
    double keygen_ms = 0;
    double encaps_ms = 0;
    double decaps_ms = 0;
    bool success = false;
};

struct SignatureBenchmarkResult {
    std::string algorithm;
    double keygen_ms = 0;
    double sign_ms = 0;
    double verify_ms = 0;
    bool success = false;
};

class BenchmarkCommand : public ICommand {
public:
    explicit BenchmarkCommand(core::CryptoEngine& engine);
    
    std::string name() const override { return "benchmark"; }
    std::string description() const override { return "Benchmark all cryptographic algorithms"; }
    
    void setup(CLI::App& app) override;
    int execute() override;

private:
    // Benchmark functions
    void benchmark_symmetric(nlohmann::json& json_results);
    void benchmark_asymmetric(nlohmann::json& json_results);
    void benchmark_pqc(nlohmann::json& json_results);
    void benchmark_kdf(nlohmann::json& json_results);
    void benchmark_compression(nlohmann::json& json_results);
    void benchmark_hash(nlohmann::json& json_results);
    
    // Algorithm-specific benchmarks
    BenchmarkResult benchmark_algorithm(core::AlgorithmType algo_type);
    AsymmetricBenchmarkResult benchmark_asymmetric_algorithm(core::AlgorithmType algo_type);
    PQCBenchmarkResult benchmark_pqc_algorithm(core::AlgorithmType algo_type);
    BenchmarkResult benchmark_hybrid_algorithm(core::AlgorithmType algo_type);
    SignatureBenchmarkResult benchmark_signature_algorithm(core::AlgorithmType algo_type);
    
    // Helpers
    std::string get_platform_info();
    void save_json_output(const nlohmann::json& results);
    void save_log_output(const std::string& log_content);
    
    core::CryptoEngine& engine_;
    std::string algorithm_;
    std::string output_file_;
    size_t data_size_ = 1048576;  // 1MB default
    int iterations_ = 5;
    bool all_ = false;
    bool json_output_ = false;
    bool pqc_only_ = false;
    bool symmetric_only_ = false;
    bool asymmetric_only_ = false;
    bool hash_only_ = false;
    bool kdf_only_ = false;
    bool compression_only_ = false;
};

} // namespace cli
} // namespace filevault

#endif
