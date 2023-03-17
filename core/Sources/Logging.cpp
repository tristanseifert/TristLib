#include <syslog.h>
#include <unistd.h>

#include <cstring>
#include <list>
#include <stdexcept>

// when including plog, omit macros that clash with syslog macros
#define PLOG_OMIT_LOG_DEFINES 1

#include <plog/Log.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Appenders/ConsoleAppender.h>
#include <plog/Appenders/RollingFileAppender.h>
#include <plog/Formatters/CsvFormatter.h>
#include <plog/Formatters/FuncMessageFormatter.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Init.h>

#include "SyslogAppender.h"
#include "TristLib/Core/Logging.h"

namespace TristLib::Core {
/**
 * @brief List of all log appenders
 */
static std::list<plog::IAppender *> gAppenders;



/**
 * @brief Translate log level from integer to plog
 *
 * This will convert the integral log levels (centered around 0, where negative values mean less
 * logging, and positive values more) into the associated plog level.
 */
constexpr static inline auto TranslateLogLevel(const int level) noexcept {
    switch(std::max(std::min(level, 2), -3)) {
        case -3:
            return plog::Severity::fatal;
        case -2:
            return plog::Severity::error;
        case -1:
            return plog::Severity::warning;
        case 0:
            return plog::Severity::info;
        case 1:
            return plog::Severity::debug;
        case 2:
            return plog::Severity::verbose;
        default: // should never be reached
            return plog::Severity::none;
    }
}

/**
 * @brief Install a logging appender
 */
static void InstallAppender(plog::IAppender *appender) {
    plog::get()->addAppender(appender);
    gAppenders.push_back(appender);
}

/**
 * @brief Initialize logging
 *
 * Sets up the plog logging framework.
 *
 * @param level Minimum log level to output
 */
static void InitPlog(const plog::Severity level) {
    plog::init(level);
}



/**
 * @brief Initialize the logging system without any outputs
 *
 * @remark You _must_ call one of the `AddLogDestination` family functions to see any log messages.
 *
 * @param level What level messages to output ([-3, 2]) where 2 is the most
 * @param simple When set, no timestamp/function name info is printed
 */
void InitLogging(const int level) noexcept {
    InitPlog(TranslateLogLevel(level));
}

/**
 * @brief Initialize the logging system
 *
 * Set up plog with a console output.
 *
 * @param level What level messages to output ([-3, 2]) where 2 is the most
 * @param simple When set, no timestamp/function name info is printed
 */
void InitLogging(const int level, const bool simple) noexcept {
    InitPlog(TranslateLogLevel(level));
    AddLogDestinationStdout(simple);
}



/**
 * @brief Install a console/stdout logger
 *
 * If the standard output is a terminal, we'll attempt to colorize the logs as well.
 *
 * @param simple Whether the simple message output format (no timestamps) is used
 * @param colorize Apply color to messages iff stdout is a terminal
 */
void AddLogDestinationStdout(const bool simple, const bool colorize) noexcept {
    plog::IAppender *appender{nullptr};

    // figure out if the console is a tty
    const bool isTty = (isatty(fileno(stdout)) == 1);

    // set up the logger
    if(simple) {
        if(isTty && colorize) {
            appender = new plog::ColorConsoleAppender<plog::FuncMessageFormatter>();
        } else {
            appender = new plog::ConsoleAppender<plog::FuncMessageFormatter>();
        }
    } else {
        if(isTty && colorize) {
            appender = new plog::ColorConsoleAppender<plog::TxtFormatter>();
        } else {
            appender = new plog::ConsoleAppender<plog::TxtFormatter>();
        }
    }
    InstallAppender(appender);
}

/**
 * @brief Send log messages to syslog
 *
 * Log messages are sent to the system log via the C library's `syslog()` function under the given
 * facility.
 */
void AddLogDestinationSyslog(const int facility, const std::string_view ident) noexcept {
    // open the log connection
    openlog(ident.data(), 0, facility);

    // create an appender
    auto appender = new SyslogAppender<plog::FuncMessageFormatter>();
    InstallAppender(appender);
}

/**
 * @brief Send log messages to a file
 *
 * Set up a log file to which all subsequent log messages are sent.
 *
 * If both maxFileSize and maxFiles are nonzero, the log file will automatically be rolled if its
 * size exceeds a certain value.
 *
 * @remark The file is not created until the first message is sent.
 *
 * @param file Path to the log file
 * @param maxFileSize Maximum size of the log file, in bytes
 * @param maxFiles Maximum number of log files
 * @param csv Whether the log file is formatted as a CSV
 */
void AddLogDestinationFile(const std::filesystem::path &path, const size_t maxFileSize,
        const size_t maxFiles, const bool csv) {
    plog::IAppender *appender{nullptr};

    if(csv) {
        appender = new plog::RollingFileAppender<plog::CsvFormatter>(path.native().c_str(),
                maxFileSize, maxFiles);
    } else {
        appender = new plog::RollingFileAppender<plog::FuncMessageFormatter>(path.native().c_str(),
                maxFileSize, maxFiles);
    }
    InstallAppender(appender);
}



/**
 * @brief Update the log level
 */
void SetLogLevel(const int logLevel) {
    if(logLevel < -3 || logLevel > 2) {
        throw std::invalid_argument("invalid log level (must be [-3, 2])");
    }

    plog::get()->setMaxSeverity(TranslateLogLevel(logLevel));
}
}
