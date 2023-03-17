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
#include <plog/Formatters/FuncMessageFormatter.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Init.h>

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
constexpr static inline auto TranslateLogLevel(const int level) {
    plog::Severity logLevel{plog::Severity::info};

    switch(std::max(std::min(level, 2), -3)) {
        case -3:
            logLevel = plog::Severity::fatal;
            break;
        case -2:
            logLevel = plog::Severity::error;
            break;
        case -1:
            logLevel = plog::Severity::warning;
            break;
        case 0:
            logLevel = plog::Severity::info;
            break;
        case 1:
            logLevel = plog::Severity::debug;
            break;
        case 2:
            logLevel = plog::Severity::verbose;
            break;
    }

    return logLevel;
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

    plog::get()->addAppender(appender);
    gAppenders.push_back(appender);
}

}
