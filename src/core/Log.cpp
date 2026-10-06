#include "Log.h"

#include <windows.h>

#include <cstdio>
#include <filesystem>

namespace frr {

Log& Log::instance() {
    static Log log;
    return log;
}

void Log::info(const std::string& message) {
    write("INFO", message);
}

void Log::warn(const std::string& message) {
    write("WARN", message);
}

void Log::error(const std::string& message) {
    write("ERROR", message);
}

void Log::write(const char* level, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!stream_.is_open()) {
        open();
    }

    if (!stream_.is_open()) {
        return;
    }

    SYSTEMTIME st{};
    GetLocalTime(&st);

    char timestamp[64]{};
    std::snprintf(
        timestamp,
        sizeof(timestamp),
        "%04u-%02u-%02u %02u:%02u:%02u.%03u",
        st.wYear,
        st.wMonth,
        st.wDay,
        st.wHour,
        st.wMinute,
        st.wSecond,
        st.wMilliseconds
    );

    stream_ << "[" << timestamp << "]"
            << " [" << level << "] "
            << message << "\n";
    stream_.flush();
}

void Log::open() {
    std::error_code ec;
    const auto dir = std::filesystem::path("scripts") / "FreeRoamRivals";
    std::filesystem::create_directories(dir, ec);

    stream_.open(
        dir / "FreeRoamRivals.log",
        std::ios::out | std::ios::app
    );
}

} // namespace frr
