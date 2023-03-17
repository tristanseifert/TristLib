/**
 * @file
 *
 * @brief Syslog output appender
 *
 * Writes plog messages to syslog
 */
#ifndef SYSLOGAPPENDER_H
#define SYSLOGAPPENDER_H

#include <syslog.h>

#define PLOG_OMIT_LOG_DEFINES 1
#include <plog/Log.h>

namespace TristLib::Core {
/**
 * @brief plog Appender for writing messages to syslog
 */
template<class Formatter>
class SyslogAppender : public plog::IAppender {
    private:
        /**
         * @brief Convert plog severity to syslog facility code
         */
        constexpr static inline int ConvertSeverity(const plog::Severity severity) {
            using S = plog::Severity;

            switch(severity) {
                case S::fatal:
                    return LOG_EMERG;
                case S::error:
                    return LOG_ERR;
                case S::warning:
                    return LOG_WARNING;
                case S::info:
                    return LOG_INFO;
                case S::debug: [[fallthrough]];
                case S::verbose:
                    return LOG_DEBUG;

                case S::none: [[fallthrough]];
                default:
                    return LOG_NOTICE;
            }
        }

    public:
        /**
         * @brief Format message and write to syslog
         */
        void write(const plog::Record &record) override {
            const auto str = Formatter::format(record);
            const int priority = ConvertSeverity(record.getSeverity());

            syslog(priority, "%s", str.c_str());
        }
};
}

#endif
