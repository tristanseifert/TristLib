#ifndef LOGGING_H
#define LOGGING_H

#include <cstddef>
#include <filesystem>

#include <plog/Log.h>

namespace TristLib::Core {
/// Set up logger without any outputs
void InitLogging(const int logLevel) noexcept;
/// Set up logger and attach tty output
void InitLogging(const int logLevel, const bool simple) noexcept;

/// Add an output to the system console
void AddLogDestinationStdout(const bool simple, const bool colorize = true) noexcept;
/// Add an output to syslog
void AddLogDestinationSyslog(const int facility) noexcept;
/// Add an output to the specified file
void AddLogDestinationFile(const std::filesystem::path &file, const size_t maxFileSize = 0,
        const size_t maxFiles = 0);

/// Update the log level
void SetLogLevel(const int logLevel) noexcept;
};

#endif
