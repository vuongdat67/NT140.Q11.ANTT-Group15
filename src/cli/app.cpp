#include "filevault/cli/app.hpp"
#include "filevault/cli/commands/encrypt_cmd.hpp"
#include "filevault/cli/commands/decrypt_cmd.hpp"
#include "filevault/cli/commands/hash_cmd.hpp"
#include "filevault/cli/commands/list_cmd.hpp"
#include "filevault/cli/commands/benchmark_cmd.hpp"
#include "filevault/cli/commands/config_cmd.hpp"
#include "filevault/cli/commands/info_cmd.hpp"
#include "filevault/utils/console.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace filevault {
namespace cli {

Application::Application() 
    : app_("FileVault", "Professional file encryption CLI tool") {
}

Application::~Application() = default;

void Application::initialize() {
    // Setup logging first
    setup_logging();
    
    // Initialize crypto engine
    engine_ = std::make_unique<core::CryptoEngine>();
    engine_->initialize();
    
    // Setup global options
    app_.add_flag("-v,--verbose", verbose_, "Verbose output");
    app_.add_option("--log-level", log_level_, "Log level (debug, info, warn, error)")
        ->check(CLI::IsMember({"debug", "info", "warn", "error"}));
    
    // Register commands
    register_commands();
    
    spdlog::info("FileVault initialized");
}

int Application::run(int argc, char** argv) {
    try {
        app_.parse(argc, argv);
        
        // Apply logging settings
        if (verbose_) {
            spdlog::set_level(spdlog::level::debug);
        } else {
            if (log_level_ == "debug") spdlog::set_level(spdlog::level::debug);
            else if (log_level_ == "info") spdlog::set_level(spdlog::level::info);
            else if (log_level_ == "warn") spdlog::set_level(spdlog::level::warn);
            else if (log_level_ == "error") spdlog::set_level(spdlog::level::err);
        }
        
        return 0;
        
    } catch (const CLI::ParseError& e) {
        return app_.exit(e);
    } catch (const std::exception& e) {
        utils::Console::error(std::string("Fatal error: ") + e.what());
        return 1;
    }
}

void Application::register_commands() {
    commands_.push_back(std::make_unique<EncryptCommand>(*engine_));
    commands_.push_back(std::make_unique<DecryptCommand>(*engine_));
    commands_.push_back(std::make_unique<HashCommand>(*engine_));
    commands_.push_back(std::make_unique<ListCommand>(*engine_));
    commands_.push_back(std::make_unique<BenchmarkCommand>(*engine_));
    commands_.push_back(std::make_unique<ConfigCommand>());
    commands_.push_back(std::make_unique<InfoCommand>(*engine_));
    
    // Setup each command
    for (auto& cmd : commands_) {
        cmd->setup(app_);
    }
    
    spdlog::info("Registered {} commands", commands_.size());
}

void Application::setup_logging() {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto logger = std::make_shared<spdlog::logger>("filevault", console_sink);
    
    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::info);
    spdlog::set_pattern("[%^%l%$] %v");
}

} // namespace cli
} // namespace filevault
