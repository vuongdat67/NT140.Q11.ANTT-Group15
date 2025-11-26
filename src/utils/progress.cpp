#include "filevault/utils/progress.hpp"
#include <iostream>

#ifdef _WIN32
    #include <io.h>
    #define ISATTY _isatty
    #define FILENO _fileno
#else
    #include <unistd.h>
    #define ISATTY isatty
    #define FILENO fileno
#endif

using namespace indicators;

namespace filevault {
namespace utils {

static bool is_terminal() {
    return ISATTY(FILENO(stdout)) != 0;
}

ProgressBar::ProgressBar(const std::string& prefix, size_t max_progress)
    : current_progress_(0), max_progress_(max_progress) {
    
    // Only show fancy progress bar if stdout is a terminal
    if (is_terminal()) {
        bar_ = std::make_unique<indicators::ProgressBar>(
            option::BarWidth{40},
            option::Start{"["},
            option::Fill{"="},
            option::Lead{">"},
            option::Remainder{" "},
            option::End{"]"},
            option::PrefixText{prefix + " "},
            option::PostfixText{" "},
            option::ForegroundColor{Color::cyan},
            option::ShowElapsedTime{false},
            option::ShowRemainingTime{false},
            option::ShowPercentage{true},
            option::MaxProgress{100}
        );
    } else {
        // Piped output - don't use progress bar
        bar_ = nullptr;
    }
}

ProgressBar::~ProgressBar() {
    if (bar_) {
        bar_->mark_as_completed();
    }
}

void ProgressBar::set_progress(size_t progress) {
    current_progress_ = progress;
    if (bar_) {
        bar_->set_progress(progress);
    }
}

void ProgressBar::tick() {
    if (current_progress_ < max_progress_) {
        set_progress(current_progress_ + 1);
    }
}

void ProgressBar::set_postfix(const std::string& text) {
    if (bar_) {
        bar_->set_option(option::PostfixText{text});
    }
}

void ProgressBar::mark_as_completed() {
    if (bar_) {
        bar_->set_progress(100);  // Ensure it shows 100% before completing
        bar_->mark_as_completed();
    }
}

void ProgressBar::hide() {
    indicators::show_console_cursor(false);
}

void ProgressBar::show() {
    indicators::show_console_cursor(true);
}

// BlockProgressBar implementation
BlockProgressBar::BlockProgressBar(const std::string& prefix, size_t max_progress) {
    bar_ = std::make_unique<indicators::BlockProgressBar>(
        option::BarWidth{80},
        option::ForegroundColor{Color::cyan},
        option::PrefixText{prefix},
        option::ShowPercentage{true},
        option::MaxProgress{static_cast<float>(max_progress)}
    );
}

void BlockProgressBar::set_progress(size_t progress) {
    if (bar_) {
        bar_->set_progress(progress);
    }
}

void BlockProgressBar::set_option_text(const std::string& text) {
    if (bar_) {
        bar_->set_option(option::PostfixText{text});
    }
}

void BlockProgressBar::mark_as_completed() {
    if (bar_) {
        bar_->mark_as_completed();
    }
}

} // namespace utils
} // namespace filevault
