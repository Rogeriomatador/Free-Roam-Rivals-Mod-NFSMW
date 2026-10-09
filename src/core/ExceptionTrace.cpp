#include "ExceptionTrace.h"
#include <windows.h>
#include <tlhelp32.h>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace frr {
namespace {
constexpr DWORD headerBytes=32768, slotBytes=512, slotCount=128;
constexpr DWORD traceBytes=headerBytes+slotBytes*slotCount;
HANDLE file=INVALID_HANDLE_VALUE, mapping=nullptr;
char* view=nullptr;
void* observer=nullptr;
volatile LONG sequence=0;
volatile LONG busy[slotCount]{};
thread_local const char* phase="outside_FRR_callbacks";

// Fixed ASCII formatter: no CRT, logger, allocation, mutex, stack walk,
// module lookup or file I/O runs inside the vectored callback.
struct Line {
    char data[slotBytes]{};
    unsigned used=0;
    void text(const char* value) noexcept {
        while (*value && used<slotBytes-2) data[used++]=*value++;
    }
    void hex(std::uint32_t value) noexcept {
        text("0x");
        constexpr char digits[]="0123456789abcdef";
        for (int shift=28;shift>=0;shift-=4)
            if(used<slotBytes-2) data[used++]=digits[(value>>shift)&15];
    }
    void field(const char* key, std::uint32_t value) noexcept {
        text(key);hex(value);text(" ");
    }
};
bool selected(DWORD code) noexcept {
    return code==EXCEPTION_ACCESS_VIOLATION || code==EXCEPTION_IN_PAGE_ERROR ||
        code==EXCEPTION_ILLEGAL_INSTRUCTION || code==EXCEPTION_PRIV_INSTRUCTION ||
        code==EXCEPTION_INT_DIVIDE_BY_ZERO || code==EXCEPTION_INT_OVERFLOW;
}
LONG WINAPI observe(EXCEPTION_POINTERS* info) {
    if(!info || !info->ExceptionRecord || !info->ContextRecord || !view ||
        !selected(info->ExceptionRecord->ExceptionCode)) return EXCEPTION_CONTINUE_SEARCH;
    const auto count=static_cast<DWORD>(InterlockedIncrement(&sequence));
    const DWORD slot=(count-1)%slotCount;
    // No wait if a simultaneous exception is still writing this ring slot.
    if(InterlockedCompareExchange(&busy[slot],1,0)!=0) return EXCEPTION_CONTINUE_SEARCH;
    Line line{};
    const auto& event=*info->ExceptionRecord;
    const auto& context=*info->ContextRecord;
    line.text("FIRST_CHANCE ");line.field("sequence=",count);
    line.field("thread=",GetCurrentThreadId());line.field("code=",event.ExceptionCode);
    line.field("instruction=",reinterpret_cast<std::uint32_t>(event.ExceptionAddress));
    line.field("eip=",context.Eip);line.field("esp=",context.Esp);line.field("ebp=",context.Ebp);
    line.field("eax=",context.Eax);line.field("ebx=",context.Ebx);line.field("ecx=",context.Ecx);
    line.field("edx=",context.Edx);line.field("esi=",context.Esi);line.field("edi=",context.Edi);
    line.field("flags=",event.ExceptionFlags);
    if(event.NumberParameters>0) line.field("parameter0=",static_cast<std::uint32_t>(event.ExceptionInformation[0]));
    if(event.NumberParameters>1) line.field("parameter1=",static_cast<std::uint32_t>(event.ExceptionInformation[1]));
    if(event.NumberParameters>2) line.field("parameter2=",static_cast<std::uint32_t>(event.ExceptionInformation[2]));
    line.text("phase=");line.text(phase);line.text(" treatment=continue_search");
    char* output=view+headerBytes+slot*slotBytes;
    // Pre-mapped writable pages; only copies to our diagnostic memory.
    for(unsigned i=0;i<slotBytes-1;++i) output[i]=i<line.used?line.data[i]:' ';
    output[slotBytes-1]='\n';
    InterlockedExchange(&busy[slot],0);
    return EXCEPTION_CONTINUE_SEARCH;
}
void releaseResources() {
    if(view) {FlushViewOfFile(view,traceBytes);UnmapViewOfFile(view);view=nullptr;}
    if(mapping) {CloseHandle(mapping);mapping=nullptr;}
    if(file!=INVALID_HANDLE_VALUE) {CloseHandle(file);file=INVALID_HANDLE_VALUE;}
}
}

bool installExceptionTrace(const wchar_t* path, const char* version) {
    if(observer) return true;
    file=CreateFileW(path,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,nullptr,
        CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE) return false;
    mapping=CreateFileMappingW(file,nullptr,PAGE_READWRITE,0,traceBytes,nullptr);
    if(!mapping) {releaseResources();return false;}
    view=static_cast<char*>(MapViewOfFile(mapping,FILE_MAP_WRITE,0,0,traceBytes));
    if(!view) {releaseResources();return false;}
    std::memset(view,' ',traceBytes); // Pre-touch all pages before registering.
    for(DWORD i=511;i<traceBytes;i+=512) view[i]='\n';
    char header[1024]{};
    const int length=std::snprintf(header,sizeof(header),
        "FRR_EXCEPTION_TRACE version=%s pid=%lu ringSlots=128 slotBytes=512\n"
        "First-chance observations can be handled by the game. They do not prove a fatal crash.\n"
        "Latest 128 selected exceptions; sort by sequence. Concurrent busy slots may be skipped.\n"
        "No exception context is modified. No game objects are read or written.\n"
        "MODULES_AT_INSTALL (later module loads/unloads are not tracked):\n",version,GetCurrentProcessId());
    DWORD used=0;
    if(length>0 && static_cast<unsigned>(length)<sizeof(header)) {
        std::memcpy(view,header,length);used=static_cast<DWORD>(length);
    }
    const HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,GetCurrentProcessId());
    bool complete=false;
    if(snapshot!=INVALID_HANDLE_VALUE) {
        MODULEENTRY32W entry{};entry.dwSize=sizeof(entry);
        bool more=Module32FirstW(snapshot,&entry)!=FALSE;
        while(more && used+1024<headerBytes) {
            char name[768]{};
            WideCharToMultiByte(CP_UTF8,0,entry.szModule,-1,name,sizeof(name),nullptr,nullptr);
            const int n=std::snprintf(header,sizeof(header),"module=%s base=0x%08lx bytes=0x%08lx\n",
                name,reinterpret_cast<unsigned long>(entry.modBaseAddr),entry.modBaseSize);
            if(n>0 && static_cast<unsigned>(n)<sizeof(header)) {
                std::memcpy(view+used,header,n);used+=static_cast<DWORD>(n);
            }
            more=Module32NextW(snapshot,&entry)!=FALSE;
        }
        complete=!more && GetLastError()==ERROR_NO_MORE_FILES;
        CloseHandle(snapshot);
    }
    const int n=std::snprintf(header,sizeof(header),"moduleInventoryComplete=%u\n",complete?1u:0u);
    if(n>0 && used+static_cast<DWORD>(n)<headerBytes) std::memcpy(view+used,header,n);
    FlushViewOfFile(view,traceBytes);
    // A callback in an unloaded DLL would be invalid. Pin this ASI for the
    // process lifetime, just as its existing runtime hooks/workers require.
    HMODULE self=nullptr;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&observe),&self)) {releaseResources();return false;}
    sequence=0;
    observer=AddVectoredExceptionHandler(1,&observe);
    if(!observer) {releaseResources();return false;}
    return true;
}
void stopExceptionTraceForTest() {
    if(observer) {RemoveVectoredExceptionHandler(observer);observer=nullptr;}
    releaseResources(); // Tests call only after their exception threads exit.
}
ExceptionTracePhase::ExceptionTracePhase(const char* staticLabel) noexcept:previous_(phase) {
    phase=staticLabel;
}
ExceptionTracePhase::~ExceptionTracePhase() {phase=previous_;}
}
