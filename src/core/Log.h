#pragma once

#include <fstream>
#include <mutex>
#include <string>

namespace frr {

class Log {
public:
    static Log& instance();

    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);

private:
    Log() = default;

    void write(const char* level, const std::string& message);
    void open();

    std::ofstream stream_;
    std::mutex mutex_;
};

} // namespace frr
