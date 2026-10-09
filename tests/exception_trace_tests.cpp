#include "core/ExceptionTrace.h"
#include <windows.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {
void require(bool ok,const char* message) {
    if(!ok) {std::cerr<<"FAILED: "<<message<<'\n';std::exit(1);}
}
bool handled(DWORD code) {
    const ULONG_PTR parameters[]{0,0x1234};
    __try {RaiseException(code,0,2,parameters);}
    __except(EXCEPTION_EXECUTE_HANDLER) {return true;}
    return false;
}
DWORD WINAPI otherThread(void*) {
    frr::ExceptionTracePhase scope("other_thread_test");
    return handled(EXCEPTION_ACCESS_VIOLATION)?0:1;
}
std::string read(const std::filesystem::path& path) {
    std::ifstream stream(path,std::ios::binary);
    return {std::istreambuf_iterator<char>(stream),{}};
}
}
int wmain(int argc,wchar_t** argv) {
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    if(argc==3 && std::wstring(argv[1])==L"--unhandled") {
        require(frr::installExceptionTrace(argv[2],"child-test"),"child trace installed");
        frr::ExceptionTracePhase scope("unhandled_child_test");
        void* inaccessible=VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_NOACCESS);
        require(inaccessible!=nullptr,"child inaccessible page allocated");
        *static_cast<volatile DWORD*>(inaccessible)=42; // Real unhandled access violation.
        return 7;
    }
    const auto folder=std::filesystem::temp_directory_path()/
        (L"FRR-exception-test-"+std::to_wstring(GetCurrentProcessId()));
    std::filesystem::create_directories(folder);
    const auto path=folder/L"handled.log";
    require(frr::installExceptionTrace(path.c_str(),"parent-test"),"trace installed");
    {
        frr::ExceptionTracePhase outer("outer_test");
        {
            frr::ExceptionTracePhase inner("inner_test");
            require(handled(EXCEPTION_ACCESS_VIOLATION),"observer leaves SEH handling intact");
        }
        require(handled(EXCEPTION_ILLEGAL_INSTRUCTION),"nested phase restored");
    }
    require(handled(0xe0001234),"unselected exception still propagates normally");
    const HANDLE thread=CreateThread(nullptr,0,&otherThread,nullptr,0,nullptr);
    require(thread!=nullptr && WaitForSingleObject(thread,5000)==WAIT_OBJECT_0,"second thread completed");
    DWORD threadExit=1;GetExitCodeThread(thread,&threadExit);CloseHandle(thread);
    require(threadExit==0,"second thread exception handled");
    require(handled(EXCEPTION_ACCESS_VIOLATION),"main-thread phase restored independently");
    frr::stopExceptionTraceForTest();
    const auto captured=read(path);
    require(captured.size()==98304,"bounded text trace size");
    require(captured.find("code=0xc0000005")!=std::string::npos,"access violation recorded");
    require(captured.find("parameter1=0x00001234")!=std::string::npos,"fault parameters preserved");
    require(captured.find("phase=inner_test")!=std::string::npos &&
        captured.find("phase=outer_test")!=std::string::npos,"nested phase labels recorded");
    require(captured.find("phase=other_thread_test")!=std::string::npos &&
        captured.find("phase=outside_FRR_callbacks")!=std::string::npos,"phase state belongs to each thread");
    require(captured.find("code=0xe0001234")==std::string::npos,"unselected exception omitted");
    require(captured.find("moduleInventoryComplete=1")!=std::string::npos,"module inventory recorded");
    require(handled(EXCEPTION_ACCESS_VIOLATION),"SEH continues after observer removal");
    require(read(path)==captured,"removed observer no longer writes");
    require(frr::installExceptionTrace(path.c_str(),"ring-test"),"trace reinstalled");
    for(unsigned i=0;i<130;++i) require(handled(EXCEPTION_ACCESS_VIOLATION),"ring exceptions handled");
    frr::stopExceptionTraceForTest();
    const auto ring=read(path);
    require(ring.find("sequence=0x00000082")!=std::string::npos &&
        ring.find("sequence=0x00000001 ")==std::string::npos,"ring retains latest entries");

    wchar_t executable[32768]{};
    require(GetModuleFileNameW(nullptr,executable,32768)>0,"test executable resolved");
    const auto childPath=folder/L"unhandled.log";
    std::wstring command=L"\""+std::wstring(executable)+L"\" --unhandled \""+childPath.wstring()+L"\"";
    STARTUPINFOW startup{};startup.cb=sizeof(startup);
    PROCESS_INFORMATION process{};
    require(CreateProcessW(executable,command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,
        nullptr,nullptr,&startup,&process)!=FALSE,"unhandled test child started");
    const auto wait=WaitForSingleObject(process.hProcess,10000);
    if(wait!=WAIT_OBJECT_0) TerminateProcess(process.hProcess,8);
    require(wait==WAIT_OBJECT_0,"unhandled child exits promptly");
    DWORD exitCode=0;GetExitCodeProcess(process.hProcess,&exitCode);
    CloseHandle(process.hThread);CloseHandle(process.hProcess);
    require(exitCode==EXCEPTION_ACCESS_VIOLATION,"observer does not swallow real fatal fault");
    const auto child=read(childPath);
    require(child.find("code=0xc0000005")!=std::string::npos &&
        child.find("phase=unhandled_child_test")!=std::string::npos,
        "mapped record survives fatal child process termination");
    std::filesystem::remove_all(folder);
    std::cout<<"Exception trace: handled/unhandled propagation, persistent crash record, phases and ring passed\n";
}
