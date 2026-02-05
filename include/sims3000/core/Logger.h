/**
 * @file Logger.h
 * @brief Logging system for Sims 3000
 *
 * Provides a flexible logging system with:
 * - Multiple log levels (TRACE, DEBUG, INFO, WARN, ERROR, FATAL)
 * - Console and file output
 * - Timestamps on each log entry
 * - Source file/line info in debug builds
 * - Convenient LOG_* macros
 */

#ifndef SIMS3000_CORE_LOGGER_H
#define SIMS3000_CORE_LOGGER_H

#include <cstdint>
#include <string>
#include <fstream>
#include <mutex>

namespace sims3000 {

/**
 * @enum LogLevel
 * @brief Severity levels for log messages.
 */
enum class LogLevel : std::uint8_t {
    TRACE = 0,  ///< Detailed tracing information
    DEBUG = 1,  ///< Debug information for development
    INFO  = 2,  ///< General informational messages
    WARN  = 3,  ///< Warning messages for potential issues
    ERROR = 4,  ///< Error messages for recoverable failures
    FATAL = 5   ///< Fatal errors that may crash the application
};

/**
 * @brief Convert LogLevel to string representation.
 */
const char* getLogLevelName(LogLevel level);

/**
 * @class Logger
 * @brief Singleton logging system with console and file output.
 *
 * Usage:
 * @code
 * Logger::instance().init("sims3000.log");
 * Logger::instance().log(LogLevel::INFO, "Application started");
 * Logger::instance().shutdown();
 * @endcode
 *
 * Or use the convenience macros:
 * @code
 * LOG_INFO("Player %d joined", playerId);
 * LOG_ERROR("Failed to load asset: %s", path);
 * @endcode
 */
class Logger {
public:
    /**
     * @brief Get the singleton Logger instance.
     */
    static Logger& instance();

    /**
     * @brief Initialize the logger with optional file output.
     * @param logFilePath Path to the log file (empty to disable file logging)
     * @param minLevel Minimum log level to output (default: DEBUG in debug builds, INFO in release)
     * @return true if initialization succeeded
     */
    bool init(const std::string& logFilePath = "", LogLevel minLevel = getDefaultLogLevel());

    /**
     * @brief Shutdown the logger and close the log file.
     */
    void shutdown();

    /**
     * @brief Set the minimum log level.
     * Messages below this level will be ignored.
     */
    void setMinLevel(LogLevel level);

    /**
     * @brief Get the current minimum log level.
     */
    LogLevel getMinLevel() const;

    /**
     * @brief Enable or disable console output.
     */
    void setConsoleEnabled(bool enabled);

    /**
     * @brief Enable or disable file output.
     */
    void setFileEnabled(bool enabled);

    /**
     * @brief Log a message with the specified level.
     * @param level The severity level
     * @param file Source file name (use __FILE__)
     * @param line Source line number (use __LINE__)
     * @param fmt Printf-style format string
     * @param ... Format arguments
     */
    void log(LogLevel level, const char* file, int line, const char* fmt, ...);

    /**
     * @brief Log a message without source location.
     * @param level The severity level
     * @param fmt Printf-style format string
     * @param ... Format arguments
     */
    void log(LogLevel level, const char* fmt, ...);

    /**
     * @brief Flush any buffered log output.
     */
    void flush();

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static LogLevel getDefaultLogLevel();
    std::string formatTimestamp() const;
    void writeLog(LogLevel level, const char* file, int line, const char* message);

    std::ofstream m_logFile;
    std::mutex m_mutex;
    LogLevel m_minLevel = LogLevel::INFO;
    bool m_consoleEnabled = true;
    bool m_fileEnabled = false;
    bool m_initialized = false;
};

} // namespace sims3000

// =============================================================================
// Logging Macros
// =============================================================================

#ifdef SIMS3000_DEBUG
    // Debug builds: include source file and line information
    #define LOG_TRACE(fmt, ...) \
        ::sims3000::Logger::instance().log(::sims3000::LogLevel::TRACE, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define LOG_DEBUG(fmt, ...) \
        ::sims3000::Logger::instance().log(::sims3000::LogLevel::DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define LOG_INFO(fmt, ...) \
        ::sims3000::Logger::instance().log(::sims3000::LogLevel::INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define LOG_WARN(fmt, ...) \
        ::sims3000::Logger::instance().log(::sims3000::LogLevel::WARN, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define LOG_ERROR(fmt, ...) \
        ::sims3000::Logger::instance().log(::sims3000::LogLevel::ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define LOG_FATAL(fmt, ...) \
        ::sims3000::Logger::instance().log(::sims3000::LogLevel::FATAL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#else
    // Release builds: no source file/line information
    #define LOG_TRACE(fmt, ...) \
        ::sims3000::Logger::instance().log(::sims3000::LogLevel::TRACE, fmt, ##__VA_ARGS__)
    #define LOG_DEBUG(fmt, ...) \
        ::sims3000::Logger::instance().log(::sims3000::LogLevel::DEBUG, fmt, ##__VA_ARGS__)
    #define LOG_INFO(fmt, ...) \
        ::sims3000::Logger::instance().log(::sims3000::LogLevel::INFO, fmt, ##__VA_ARGS__)
    #define LOG_WARN(fmt, ...) \
        ::sims3000::Logger::instance().log(::sims3000::LogLevel::WARN, fmt, ##__VA_ARGS__)
    #define LOG_ERROR(fmt, ...) \
        ::sims3000::Logger::instance().log(::sims3000::LogLevel::ERROR, fmt, ##__VA_ARGS__)
    #define LOG_FATAL(fmt, ...) \
        ::sims3000::Logger::instance().log(::sims3000::LogLevel::FATAL, fmt, ##__VA_ARGS__)
#endif

#endif // SIMS3000_CORE_LOGGER_H
