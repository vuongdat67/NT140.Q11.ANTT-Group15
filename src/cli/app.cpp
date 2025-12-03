#include "filevault/cli/app.hpp"
#include "filevault/cli/commands/encrypt_cmd.hpp"
#include "filevault/cli/commands/decrypt_cmd.hpp"
#include "filevault/cli/commands/hash_cmd.hpp"
#include "filevault/cli/commands/list_cmd.hpp"
#include "filevault/cli/commands/benchmark_cmd.hpp"
#include "filevault/cli/commands/config_cmd.hpp"
#include "filevault/cli/commands/info_cmd.hpp"
#include "filevault/cli/commands/compress_cmd.hpp"
#include "filevault/cli/commands/stego_cmd.hpp"
#include "filevault/cli/commands/archive_cmd.hpp"
#include "filevault/cli/commands/keygen_cmd.hpp"
#include "filevault/utils/console.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace filevault {
namespace cli {

Application::Application() 
    : app_("FileVault", "Professional file encryption CLI tool") {
    // Require subcommand or show help
    app_.require_subcommand(0);
    app_.set_help_all_flag("--help-all", "Show all help");
    
    // Add footer with quick examples
    app_.footer(
        "\nQuick Examples:\n"
        "  Encrypt file:        filevault encrypt document.txt -m standard\n"
        "  Decrypt file:        filevault decrypt document.txt.fvlt\n"
        "  Hash file:           filevault hash document.txt -a sha256\n"
        "  Compress file:       filevault compress large_file.txt -a lzma\n"
        "  Create archive:      filevault archive create *.txt -o archive.fva\n"
        "  Hide data in image:  filevault stego embed secret.txt image.png output.png\n"
        "  List algorithms:     filevault list\n"
        "  Show config:         filevault config show\n"
        "\n"
        "For detailed usage examples, see USAGE.md or run: <command> --help\n"
    );
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
        // Pre-parse to check for verbose flag
        bool early_verbose = false;
        for (int i = 1; i < argc; ++i) {
            std::string arg(argv[i]);
            if (arg == "-v" || arg == "--verbose") {
                early_verbose = true;
                break;
            }
            if (arg.rfind("--log-level=", 0) == 0) {
                std::string level = arg.substr(12);
                if (level == "debug" || level == "info") {
                    early_verbose = true;
                }
                break;
            }
        }
        
        // Apply verbose early so initialization logs are visible
        if (early_verbose) {
            spdlog::set_level(spdlog::level::info);
        }
        
        app_.parse(argc, argv);
        
        // Apply logging settings after parse
        if (verbose_) {
            spdlog::set_level(spdlog::level::debug);
        } else if (!log_level_.empty()) {
            if (log_level_ == "debug") spdlog::set_level(spdlog::level::debug);
            else if (log_level_ == "info") spdlog::set_level(spdlog::level::info);
            else if (log_level_ == "warn") spdlog::set_level(spdlog::level::warn);
            else if (log_level_ == "error") spdlog::set_level(spdlog::level::err);
        }
        
        // Show help if no subcommand was provided
        if (app_.get_subcommands().empty()) {
            std::cout << app_.help() << std::endl;
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
    commands_.push_back(std::make_unique<CompressCommand>());
    commands_.push_back(std::make_unique<commands::StegoCommand>());
    commands_.push_back(std::make_unique<commands::ArchiveCommand>(*engine_));
    commands_.push_back(std::make_unique<KeygenCommand>(*engine_));
    
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
    // Default to error level - hide info/warnings unless --verbose is used
    spdlog::set_level(spdlog::level::err);
    spdlog::set_pattern("[%^%l%$] %v");
}

} // namespace cli
} // namespace filevault
