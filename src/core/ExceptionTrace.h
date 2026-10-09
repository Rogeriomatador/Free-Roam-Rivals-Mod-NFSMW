#pragma once

namespace frr {
// Process-lifetime observer. Records selected first-chance native exceptions;
// never handles them or changes the exception context. Path must be writable.
bool installExceptionTrace(const wchar_t* path, const char* version);
// Quiescent test teardown only; the production ASI is pinned once installed.
void stopExceptionTraceForTest();
class ExceptionTracePhase {
public:
    explicit ExceptionTracePhase(const char* staticLabel) noexcept;
    ~ExceptionTracePhase();
    ExceptionTracePhase(const ExceptionTracePhase&) = delete;
    ExceptionTracePhase& operator=(const ExceptionTracePhase&) = delete;
private:
    const char* previous_;
};
}
