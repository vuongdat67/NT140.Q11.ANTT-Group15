#ifndef FILEVAULT_CLI_COMMANDS_BENCHMARK_CMD_HPP
#define FILEVAULT_CLI_COMMANDS_BENCHMARK_CMD_HPP

#include "filevault/cli/command.hpp"
#include "filevault/core/crypto_engine.hpp"
#include <nlohmann/json.hpp>

namespace filevault {
namespace cli {

class BenchmarkCommand : public ICommand {
public:
    explicit BenchmarkCommand(core::CryptoEngine& engine);
    
    std::string name() const override { return "benchmark"; }
    std::string description() const override { return "Benchmark algorithms"; }
    
    void setup(CLI::App& app) override;
    int execute() override;

private:
    struct BenchmarkResult {
        std::string algorithm;
        size_t data_size;
        double time_ms;
        double throughput_mbps;
    };
    
    void benchmark_encryption();
    void benchmark_kdf();
    void benchmark_compression();
    void save_json_output(const nlohmann::json& results);
    void save_log_output(const std::string& log_content);
    
    core::CryptoEngine& engine_;
    std::string algorithm_;
    std::string output_file_;
    bool all_ = false;
    bool json_output_ = false;
    std::vector<BenchmarkResult> results_;
};

} // namespace cli
} // namespace filevault

#endif
