#include <nfsmw_sdk/nfsmw_sdk.h>

#include <windows.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>

namespace frr {

constexpr const char* kName = "NFSMW Free Roam Rivals";
constexpr const char* kVersion = "0.0.1";

class Log {
public:
    static Log& instance() {
        static Log log;
        return log;
    }

    void write(const char* level, const std::string& message) {
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

private:
    void open() {
        std::error_code ec;
        const auto dir = std::filesystem::path("scripts") / "FreeRoamRivals";
        std::filesystem::create_directories(dir, ec);

        stream_.open(
            dir / "FreeRoamRivals.log",
            std::ios::out | std::ios::app
        );
    }

    std::ofstream stream_;
    std::mutex mutex_;
};

void info(const std::string& msg) {
    Log::instance().write("INFO", msg);
}

void warn(const std::string& msg) {
    Log::instance().write("WARN", msg);
}

// v0.0.1 intentionally performs no gameplay mutation.
// The next implementation step is a verified executable guard followed by
// a safe per-frame hook and player-vehicle discovery.
int bootstrap() {
    info(std::string(kName) + " v" + kVersion + " loaded.");
    info("Bootstrap-only build: gameplay hooks are not installed yet.");
    info("High-risk systems (economy / garage / pink slips) are disabled.");
    return NFSMW_OK;
}

} // namespace frr

NFSMW_PLUGIN_DECLARE("Free Roam Rivals", "0.0.1", "Rogeriomatador")

NFSMW_PLUGIN_MAIN() {
    return frr::bootstrap();
}
