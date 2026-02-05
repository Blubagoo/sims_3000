/**
 * @file Logger.cpp
 * @brief Logger implementation.
 */

#include "sims3000/core/Logger.h"
#include <SDL3/SDL_log.h>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace sims3000 {

const char* getLogLevelName(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

Logger& Logger::instance() {
    static Logger s_instance;
    return s_instance;
}

Logger::~Logger() {
    shutdown();
}

bool Logger::init(const std::string& logFilePath, LogLevel minLevel) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_initialized) {
        return true;
    }

    m_minLevel = minLevel;

    // Open log file if path provided
    if (!logFilePath.empty()) {
        m_logFile.open(logFilePath, std::ios::out | std::ios::app);
        if (m_logFile.is_open()) {
            m_fileEnabled = true;

            // Write session header
            auto now = std::chrono::system_clock::now();
            std::time_t now_time = std::chrono::system_clock::to_time_t(now);
            std::tm local_tm;
#ifdef _WIN32
            localtime_s(&local_tm, &now_time);
#else
            localtime_r(&now_time, &local_tm);
#endif
            std::ostringstream oss;
            oss << "\n========================================\n";
            oss << "Log session started: ";
            oss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
            oss << "\n========================================\n";
            m_logFile << oss.str();
            m_logFile.flush();
        } else {
            SDL_Log("Logger: Failed to open log file: %s", logFilePath.c_str());
        }
    }

    m_initialized = true;

    // Log initialization
    log(LogLevel::INFO, "Logger initialized (min level: %s, file: %s, console: %s)",
        getLogLevelName(m_minLevel),
        m_fileEnabled ? "enabled" : "disabled",
        m_consoleEnabled ? "enabled" : "disabled");

    return true;
}

void Logger::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized) {
        return;
    }

    if (m_logFile.is_open()) {
        // Write session footer
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::tm local_tm;
#ifdef _WIN32
        localtime_s(&local_tm, &now_time);
#else
        localtime_r(&now_time, &local_tm);
#endif
        std::ostringstream oss;
        oss << "Log session ended: ";
        oss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
        oss << "\n========================================\n";
        m_logFile << oss.str();
        m_logFile.close();
    }

    m_fileEnabled = false;
    m_initialized = false;
}

void Logger::setMinLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_minLevel = level;
}

LogLevel Logger::getMinLevel() const {
    return m_minLevel;
}

void Logger::setConsoleEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_consoleEnabled = enabled;
}

void Logger::setFileEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_fileEnabled = enabled && m_logFile.is_open();
}

void Logger::log(LogLevel level, const char* file, int line, const char* fmt, ...) {
    if (static_cast<std::uint8_t>(level) < static_cast<std::uint8_t>(m_minLevel)) {
        return;
    }

    char buffer[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    writeLog(level, file, line, buffer);
}

void Logger::log(LogLevel level, const char* fmt, ...) {
    if (static_cast<std::uint8_t>(level) < static_cast<std::uint8_t>(m_minLevel)) {
        return;
    }

    char buffer[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    writeLog(level, nullptr, 0, buffer);
}

void Logger::flush() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_logFile.is_open()) {
        m_logFile.flush();
    }
}

LogLevel Logger::getDefaultLogLevel() {
#ifdef SIMS3000_DEBUG
    return LogLevel::DEBUG;
#else
    return LogLevel::INFO;
#endif
}

std::string Logger::formatTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::tm local_tm;
#ifdef _WIN32
    localtime_s(&local_tm, &time);
#else
    localtime_r(&time, &local_tm);
#endif

    std::ostringstream oss;
    oss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

void Logger::writeLog(LogLevel level, const char* file, int line, const char* message) {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::string timestamp = formatTimestamp();
    const char* levelStr = getLogLevelName(level);

    // Build the log line
    std::ostringstream logLine;
    logLine << "[" << timestamp << "] [" << levelStr << "] ";

    // Add source location if provided
    if (file != nullptr && line > 0) {
        // Extract just the filename from the full path
        const char* filename = file;
        const char* lastSlash = strrchr(file, '/');
        const char* lastBackslash = strrchr(file, '\\');
        if (lastSlash != nullptr) {
            filename = lastSlash + 1;
        }
        if (lastBackslash != nullptr && lastBackslash > lastSlash) {
            filename = lastBackslash + 1;
        }
        logLine << "[" << filename << ":" << line << "] ";
    }

    logLine << message;

    std::string logStr = logLine.str();

    // Console output via SDL_Log
    if (m_consoleEnabled) {
        // Map our log levels to SDL log priorities for proper console formatting
        SDL_LogPriority sdlPriority;
        switch (level) {
            case LogLevel::TRACE: sdlPriority = SDL_LOG_PRIORITY_VERBOSE; break;
            case LogLevel::DEBUG: sdlPriority = SDL_LOG_PRIORITY_DEBUG; break;
            case LogLevel::INFO:  sdlPriority = SDL_LOG_PRIORITY_INFO; break;
            case LogLevel::WARN:  sdlPriority = SDL_LOG_PRIORITY_WARN; break;
            case LogLevel::ERROR: sdlPriority = SDL_LOG_PRIORITY_ERROR; break;
            case LogLevel::FATAL: sdlPriority = SDL_LOG_PRIORITY_CRITICAL; break;
            default: sdlPriority = SDL_LOG_PRIORITY_INFO; break;
        }
        SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, sdlPriority, "%s", logStr.c_str());
    }

    // File output
    if (m_fileEnabled && m_logFile.is_open()) {
        m_logFile << logStr << "\n";

        // Flush immediately for ERROR and FATAL to ensure they're written
        if (level >= LogLevel::ERROR) {
            m_logFile.flush();
        }
    }
}

} // namespace sims3000
