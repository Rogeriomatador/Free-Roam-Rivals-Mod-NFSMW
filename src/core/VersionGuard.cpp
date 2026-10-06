#include "VersionGuard.h"

#include <windows.h>
#include <wincrypt.h>

#include <array>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

namespace frr {
namespace {

constexpr const char* kSupportedMd5 = "C0516B485065FABDD69579816B5DF763";
constexpr std::uint64_t kSupportedSize = 6029312ull;

std::string currentExecutablePath() {
    std::vector<char> buffer(32768, '\0');
    const DWORD length = GetModuleFileNameA(
        nullptr,
        buffer.data(),
        static_cast<DWORD>(buffer.size())
    );

    if (length == 0 || length >= buffer.size()) {
        return {};
    }

    return std::string(buffer.data(), length);
}

std::uint64_t fileSize(const std::string& path) {
    HANDLE file = CreateFileA(
        path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (file == INVALID_HANDLE_VALUE) {
        return 0;
    }

    LARGE_INTEGER size{};
    const BOOL ok = GetFileSizeEx(file, &size);
    CloseHandle(file);

    if (!ok || size.QuadPart < 0) {
        return 0;
    }

    return static_cast<std::uint64_t>(size.QuadPart);
}

std::string md5File(const std::string& path) {
    HANDLE file = CreateFileA(
        path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
        nullptr
    );

    if (file == INVALID_HANDLE_VALUE) {
        return {};
    }

    HCRYPTPROV provider = 0;
    HCRYPTHASH hash = 0;

    if (!CryptAcquireContextA(
            &provider,
            nullptr,
            nullptr,
            PROV_RSA_FULL,
            CRYPT_VERIFYCONTEXT)) {
        CloseHandle(file);
        return {};
    }

    if (!CryptCreateHash(provider, CALG_MD5, 0, 0, &hash)) {
        CryptReleaseContext(provider, 0);
        CloseHandle(file);
        return {};
    }

    std::array<BYTE, 64 * 1024> buffer{};
    DWORD read = 0;
    bool ok = true;

    while (ReadFile(
        file,
        buffer.data(),
        static_cast<DWORD>(buffer.size()),
        &read,
        nullptr
    ) && read != 0) {
        if (!CryptHashData(hash, buffer.data(), read, 0)) {
            ok = false;
            break;
        }
    }

    std::array<BYTE, 16> digest{};
    DWORD digestSize = static_cast<DWORD>(digest.size());

    if (ok && !CryptGetHashParam(
            hash,
            HP_HASHVAL,
            digest.data(),
            &digestSize,
            0)) {
        ok = false;
    }

    CryptDestroyHash(hash);
    CryptReleaseContext(provider, 0);
    CloseHandle(file);

    if (!ok) {
        return {};
    }

    std::ostringstream out;
    out << std::uppercase << std::hex << std::setfill('0');

    for (BYTE b : digest) {
        out << std::setw(2) << static_cast<unsigned>(b);
    }

    return out.str();
}

} // namespace

VersionCheck VersionGuard::checkCurrentExecutable() {
    VersionCheck result{};

    result.executable.path = currentExecutablePath();
    if (result.executable.path.empty()) {
        result.reason = "Could not resolve speed.exe path.";
        return result;
    }

    result.executable.size = fileSize(result.executable.path);
    if (result.executable.size == 0) {
        result.reason = "Could not read executable size.";
        return result;
    }

    result.executable.md5 = md5File(result.executable.path);
    if (result.executable.md5.empty()) {
        result.reason = "Could not calculate executable MD5.";
        return result;
    }

    if (result.executable.size != kSupportedSize) {
        result.reason = "Executable size is not the currently supported build.";
        return result;
    }

    if (result.executable.md5 != kSupportedMd5) {
        result.reason = "Executable MD5 is not the currently supported build.";
        return result;
    }

    result.supported = true;
    result.reason = "Supported NFSMW v1.3 executable.";
    return result;
}

} // namespace frr
