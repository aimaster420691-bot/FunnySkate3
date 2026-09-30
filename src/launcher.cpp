#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <filesystem>
#include <iostream>
#include <string>

DWORD FindProcess(const std::wstring& exe) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W e{sizeof(e)};
    DWORD pid=0;
    if (Process32FirstW(snap,&e)) {
        do {
            if (_wcsicmp(e.szExeFile, exe.c_str())==0) { pid=e.th32ProcessID; break; }
        } while (Process32NextW(snap,&e));
    }
    CloseHandle(snap);
    return pid;
}

bool Inject(DWORD pid, const std::wstring& dll) {
    HANDLE p=OpenProcess(PROCESS_CREATE_THREAD|PROCESS_QUERY_INFORMATION|
                         PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ,FALSE,pid);
    if (!p) return false;

    SIZE_T bytes=(dll.size()+1)*sizeof(wchar_t);
    void* mem=VirtualAllocEx(p,nullptr,bytes,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if (!mem) { CloseHandle(p); return false; }

    if (!WriteProcessMemory(p,mem,dll.c_str(),bytes,nullptr)) {
        VirtualFreeEx(p,mem,0,MEM_RELEASE); CloseHandle(p); return false;
    }

    auto load=GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"LoadLibraryW");
    HANDLE th=CreateRemoteThread(p,nullptr,0,
        reinterpret_cast<LPTHREAD_START_ROUTINE>(load),mem,0,nullptr);
    if (!th) {
        VirtualFreeEx(p,mem,0,MEM_RELEASE); CloseHandle(p); return false;
    }
    WaitForSingleObject(th,10000);
    CloseHandle(th);
    VirtualFreeEx(p,mem,0,MEM_RELEASE);
    CloseHandle(p);
    return true;
}

int wmain(int argc, wchar_t** argv) {
    std::wstring game = argc>1 ? argv[1] : L"skate3.exe";
    std::filesystem::path dll =
        std::filesystem::absolute(std::filesystem::path(L"Skate3FunTrainer.dll"));

    DWORD pid=FindProcess(game);
    if (!pid) {
        std::wcout << L"Start skate3.exe first, then run this launcher.\n";
        return 2;
    }
    if (!std::filesystem::exists(dll)) {
        std::wcerr << L"Missing " << dll << L"\n";
        return 3;
    }
    if (!Inject(pid,dll.wstring())) {
        std::wcerr << L"Injection failed. Run with the same/elevated permissions as skate3.exe.\n";
        return 4;
    }
    std::wcout << L"Trainer loaded. Press F2 in-game.\n";
    return 0;
}
