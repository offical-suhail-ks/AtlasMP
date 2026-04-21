// AtlasMP Launcher
// 1. Launch via PlayGTAV.exe -nobattleye (Rockstar auth, BE disabled)
// 2. When GTA5.exe spawns, immediately open it using token duplication
//    to work around the integrity level difference
// 3. Inject AtlasMP-Client.dll

#ifndef UNICODE
#define UNICODE
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdlib.h>
#include <windows.h>
#include <winternl.h>
#include <tlhelp32.h>
#include <shellapi.h>
#include <shlobj.h>
#include <commdlg.h>
#include <psapi.h>
#include <optional>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <iostream>
#pragma comment(lib,"comdlg32.lib")
#pragma comment(lib,"Psapi.lib")
namespace fs = std::filesystem;

using NtCreateThreadExFn = NTSTATUS(NTAPI*)(
    PHANDLE,ACCESS_MASK,PVOID,HANDLE,PVOID,PVOID,ULONG,SIZE_T,SIZE_T,SIZE_T,PVOID);

// helpers
static std::string  WU8(const std::wstring& w){if(w.empty())return{};int n=WideCharToMultiByte(CP_UTF8,0,w.c_str(),-1,nullptr,0,nullptr,nullptr);std::string o(n,0);WideCharToMultiByte(CP_UTF8,0,w.c_str(),-1,o.data(),n,nullptr,nullptr);while(!o.empty()&&!o.back())o.pop_back();return o;}
static std::wstring U8W(const std::string& s){if(s.empty())return{};int n=MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,nullptr,0);std::wstring o(n,0);MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,o.data(),n);while(!o.empty()&&!o.back())o.pop_back();return o;}
static std::string JE(const std::string& s){std::string o;for(char c:s){if(c=='"')o+="\\\"";else if(c=='\\')o+="\\\\";else o+=c;}return o;}
static std::string JS(const std::string& j,const std::string& k){auto p=j.find("\""+k+"\"");if(p==std::string::npos)return{};p=j.find(':',p);if(p==std::string::npos)return{};p=j.find('"',p);if(p==std::string::npos)return{};++p;std::string v;while(p<j.size()&&j[p]!='"'){if(j[p]=='\\'&&p+1<j.size())++p;v+=j[p++];}return v;}
static int JI(const std::string& j,const std::string& k,int d){auto p=j.find("\""+k+"\"");if(p==std::string::npos)return d;p=j.find(':',p);if(p==std::string::npos)return d;while(p<j.size()&&!isdigit((unsigned char)j[p]))++p;return p<j.size()?std::stoi(j.substr(p)):d;}

// config
struct Cfg{std::wstring gta;std::string host="127.0.0.1";uint16_t port=7788;std::string name="AtlasPlayer";};
static fs::path CfgDir(){wchar_t b[MAX_PATH]{};SHGetFolderPathW(nullptr,CSIDL_APPDATA,nullptr,0,b);return fs::path(b)/L"AtlasMP";}
static void SaveCfg(const Cfg& c){auto d=CfgDir();std::error_code e;fs::create_directories(d,e);std::ofstream f(d/L"lc.json",std::ios::trunc);if(!f)return;f<<"{\n\"g\":\""<<JE(WU8(c.gta))<<"\",\n\"h\":\""<<JE(c.host)<<"\",\n\"p\":"<<c.port<<",\n\"n\":\""<<JE(c.name)<<"\"\n}\n";}
static bool LoadCfg(Cfg& c){auto p=CfgDir()/L"lc.json";if(!fs::exists(p))return false;std::ifstream f(p);if(!f)return false;std::string j((std::istreambuf_iterator<char>(f)),{});auto g=JS(j,"g");if(!g.empty())c.gta=U8W(g);auto h=JS(j,"h");if(!h.empty())c.host=h;c.port=(uint16_t)JI(j,"p",7788);auto n=JS(j,"n");if(!n.empty())c.name=n;return true;}

// privilege
static bool Elevated(){HANDLE t{};TOKEN_ELEVATION e{};DWORD n=0;if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&t))return false;GetTokenInformation(t,TokenElevation,&e,sizeof(e),&n);CloseHandle(t);return e.TokenIsElevated!=0;}
static void DebugPriv(){HANDLE t{};OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,&t);LUID l{};LookupPrivilegeValueW(nullptr,SE_DEBUG_NAME,&l);TOKEN_PRIVILEGES tp{1};tp.Privileges[0]={l,SE_PRIVILEGE_ENABLED};AdjustTokenPrivileges(t,FALSE,&tp,sizeof(tp),nullptr,nullptr);CloseHandle(t);}
static bool RelaunchElevated(int argc,wchar_t** argv){
    wchar_t e[MAX_PATH]{};GetModuleFileNameW(nullptr,e,MAX_PATH);
    std::wstring a;for(int i=1;i<argc;++i){a+=L"\"";a+=argv[i];a+=L"\" ";}
    return (intptr_t)ShellExecuteW(nullptr,L"runas",e,a.empty()?nullptr:a.c_str(),nullptr,SW_SHOWNORMAL)>32;
}

// detect GTA
static std::optional<std::wstring> RR(HKEY h,const wchar_t* k,const wchar_t* v){DWORD t=0,b=0;if(RegGetValueW(h,k,v,RRF_RT_REG_SZ,&t,nullptr,&b)!=ERROR_SUCCESS)return{};std::wstring o(b/sizeof(wchar_t),0);if(RegGetValueW(h,k,v,RRF_RT_REG_SZ,&t,o.data(),&b)!=ERROR_SUCCESS)return{};while(!o.empty()&&!o.back())o.pop_back();return o;}
static std::optional<fs::path> DetectGTA(){
    for(auto h:{HKEY_LOCAL_MACHINE,HKEY_CURRENT_USER})
        for(auto k:{L"SOFTWARE\\WOW6432Node\\Rockstar Games\\Grand Theft Auto V",
                    L"SOFTWARE\\Rockstar Games\\Grand Theft Auto V"})
            if(auto v=RR(h,k,L"InstallFolder")){fs::path p=fs::path(*v)/L"GTA5.exe";if(fs::exists(p))return p;}
    return{};
}
static std::optional<fs::path> Browse(){
    wchar_t b[MAX_PATH]{};OPENFILENAMEW o{};o.lStructSize=sizeof(o);
    o.lpstrFilter=L"GTA5.exe\0GTA5.exe\0All\0*.*\0";o.lpstrFile=b;o.nMaxFile=MAX_PATH;
    o.lpstrTitle=L"Select GTA5.exe";o.Flags=OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST|OFN_EXPLORER;
    if(!GetOpenFileNameW(&o))return{};return fs::exists(b)?std::optional(fs::path(b)):std::nullopt;
}

// get all current GTA5 PIDs
static std::set<DWORD> GetGTAPids(){
    std::set<DWORD> s;
    HANDLE sn=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if(sn==INVALID_HANDLE_VALUE)return s;
    PROCESSENTRY32W pe{};pe.dwSize=sizeof(pe);
    if(Process32FirstW(sn,&pe))do{
        if(_wcsicmp(pe.szExeFile,L"GTA5.exe")==0)s.insert(pe.th32ProcessID);
    }while(Process32NextW(sn,&pe));
    CloseHandle(sn);return s;
}

// wait for a new GTA5.exe PID
static DWORD WaitForNewGTA5(const std::set<DWORD>& existing, int ms=120000){
    auto end=GetTickCount64()+ms;
    while(GetTickCount64()<(ULONGLONG)end){
        HANDLE sn=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
        if(sn!=INVALID_HANDLE_VALUE){
            PROCESSENTRY32W pe{};pe.dwSize=sizeof(pe);
            if(Process32FirstW(sn,&pe))do{
                if(_wcsicmp(pe.szExeFile,L"GTA5.exe")==0&&!existing.count(pe.th32ProcessID)){
                    CloseHandle(sn);
                    printf("[Launcher] GTA5.exe PID %lu\n",pe.th32ProcessID);
                    return pe.th32ProcessID;
                }
            }while(Process32NextW(sn,&pe));
            CloseHandle(sn);
        }
        Sleep(100);
    }
    return 0;
}

// wait for grcWindow (game fully loaded)
static bool WaitForGRCWindow(DWORD pid,int ms=300000){
    auto end=GetTickCount64()+ms;
    int dots=0;
    while(GetTickCount64()<(ULONGLONG)end){
        HWND hw=FindWindowW(L"grcWindow",nullptr);
        if(hw){DWORD wp=0;GetWindowThreadProcessId(hw,&wp);if(wp==pid){printf("\n[Launcher] Game loaded (grcWindow)\n");return true;}}
        HANDLE hp=OpenProcess(SYNCHRONIZE,FALSE,pid);
        if(!hp){printf("\n[Launcher] GTA5 exited\n");return false;}
        bool dead=WaitForSingleObject(hp,0)==WAIT_OBJECT_0;
        CloseHandle(hp);if(dead){printf("\n[Launcher] GTA5 exited\n");return false;}
        if(++dots%6==0){printf(".");fflush(stdout);}
        Sleep(500);
    }
    return false;
}

// inject using LoadLibraryW remote thread
struct IR{bool ok=false;std::wstring stage;DWORD err=0;};
static IR InjectDll(HANDLE hp,const fs::path& dll){
    IR r;
    std::wstring fp=fs::absolute(dll).wstring();
    SIZE_T bytes=(fp.size()+1)*sizeof(wchar_t);
    LPVOID mem=VirtualAllocEx(hp,nullptr,bytes,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!mem){r.stage=L"VirtualAllocEx";r.err=GetLastError();return r;}
    WriteProcessMemory(hp,mem,fp.c_str(),bytes,nullptr);
    auto ll=(LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"LoadLibraryW");
    HANDLE ht=nullptr;
    auto ntdll=GetModuleHandleW(L"ntdll.dll");
    auto NtEx=ntdll?(NtCreateThreadExFn)GetProcAddress(ntdll,"NtCreateThreadEx"):nullptr;
    if(NtEx){NTSTATUS s=NtEx(&ht,THREAD_ALL_ACCESS,nullptr,hp,(PVOID)ll,mem,0,0,0,0,nullptr);if(s<0)ht=nullptr;}
    if(!ht)ht=CreateRemoteThread(hp,nullptr,0,ll,mem,0,nullptr);
    if(!ht){r.stage=L"CreateRemoteThread";r.err=GetLastError();VirtualFreeEx(hp,mem,0,MEM_RELEASE);return r;}
    DWORD exit=0;WaitForSingleObject(ht,30000);GetExitCodeThread(ht,&exit);CloseHandle(ht);
    VirtualFreeEx(hp,mem,0,MEM_RELEASE);
    if(!exit){r.stage=L"LoadLibraryW returned NULL";r.err=ERROR_MOD_NOT_FOUND;return r;}
    r.ok=true;return r;
}

// open process using token impersonation to bypass integrity level
static HANDLE OpenProcessAnyIL(DWORD pid){
    // Try normal open first
    HANDLE hp=OpenProcess(
        PROCESS_CREATE_THREAD|PROCESS_QUERY_INFORMATION|
        PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ,
        FALSE,pid);
    if(hp)return hp;

    // Duplicate token from winlogon/lsass to get SYSTEM token,
    // then impersonate and retry — works when GTA runs at High IL
    HANDLE sn=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if(sn==INVALID_HANDLE_VALUE)return nullptr;

    DWORD systemPid=0;
    PROCESSENTRY32W pe{};pe.dwSize=sizeof(pe);
    if(Process32FirstW(sn,&pe))do{
        if(_wcsicmp(pe.szExeFile,L"winlogon.exe")==0){
            systemPid=pe.th32ProcessID;break;
        }
    }while(Process32NextW(sn,&pe));
    CloseHandle(sn);

    if(!systemPid)return nullptr;

    HANDLE hSys=OpenProcess(PROCESS_QUERY_INFORMATION,FALSE,systemPid);
    if(!hSys)return nullptr;

    HANDLE hToken=nullptr,hDup=nullptr;
    OpenProcessToken(hSys,TOKEN_DUPLICATE|TOKEN_ASSIGN_PRIMARY|TOKEN_QUERY,&hToken);
    CloseHandle(hSys);
    if(!hToken)return nullptr;

    DuplicateTokenEx(hToken,TOKEN_ALL_ACCESS,nullptr,
        SecurityImpersonation,TokenImpersonation,&hDup);
    CloseHandle(hToken);
    if(!hDup)return nullptr;

    // Impersonate SYSTEM and retry open
    SetThreadToken(nullptr,hDup);
    hp=OpenProcess(
        PROCESS_CREATE_THREAD|PROCESS_QUERY_INFORMATION|
        PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ,
        FALSE,pid);
    RevertToSelf();
    CloseHandle(hDup);
    return hp;
}

static int Fatal(const wchar_t* msg){
    fwprintf(stderr,L"\n[FATAL] %ls\n\nPress Enter...\n",msg);
    getchar();return 1;
}

int wmain(int argc,wchar_t* argv[])
{
    printf("===========================================\n");
    printf("  AtlasMP Launcher\n");
    printf("===========================================\n\n");

    if(!Elevated()){
        printf("[Launcher] Requesting elevation...\n");
        if(RelaunchElevated(argc,argv))return 0;
        return Fatal(L"Must run as Administrator");
    }
    DebugPriv();
    printf("[Launcher] Administrator: OK\n");

    Cfg cfg;LoadCfg(cfg);
    for(int i=1;i<argc;++i){
        std::wstring a=argv[i];
        auto nx=[&](std::wstring& o){if(i+1<argc){o=argv[++i];return true;}return false;};
        if(a==L"--gta")nx(cfg.gta);
        else if(a==L"--host"){std::wstring h;nx(h);cfg.host=WU8(h);}
        else if(a==L"--port"){std::wstring p;nx(p);cfg.port=(uint16_t)std::stoul(p);}
        else if(a==L"--name"){std::wstring n;nx(n);cfg.name=WU8(n);}
    }

    if(cfg.gta.empty()||!fs::exists(cfg.gta)){
        if(auto d=DetectGTA())cfg.gta=d->wstring();
        else{if(auto p=Browse())cfg.gta=p->wstring();else return Fatal(L"GTA5.exe not selected.");}
    }
    printf("[Launcher] GTA5.exe : %ls\n",cfg.gta.c_str());

    fs::path gtaDir=fs::path(cfg.gta).parent_path();
    fs::path playGta=gtaDir/L"PlayGTAV.exe";
    if(!fs::exists(playGta))return Fatal(L"PlayGTAV.exe not found");

    SaveCfg(cfg);

    // Write connection config
    {
        std::ofstream f(gtaDir/L"atlasmp-client.toml",std::ios::trunc);
        if(f)f<<"[connection]\nhost=\""<<cfg.host<<"\"\nport="<<cfg.port
              <<"\nname=\""<<cfg.name<<"\"\nauto_connect=true\n";
    }
    printf("[Launcher] Server   : %s:%d  name=%s\n",cfg.host.c_str(),(int)cfg.port,cfg.name.c_str());

    // Copy DLL
    wchar_t self[MAX_PATH]{};GetModuleFileNameW(nullptr,self,MAX_PATH);
    fs::path selfDir=fs::path(self).parent_path();
    fs::path dllDst=gtaDir/L"AtlasMP-Client.dll";
    bool hasDll=false;
    for(auto c:{selfDir/L"AtlasMP-Client.dll",dllDst}){
        if(fs::exists(c)){
            std::error_code ec;
            if(!fs::equivalent(c,dllDst,ec)||ec)
                fs::copy_file(c,dllDst,fs::copy_options::overwrite_existing,ec);
            hasDll=true;break;
        }
    }
    printf("[Launcher] DLL      : %ls\n",hasDll?dllDst.c_str():L"NOT FOUND");

    auto existing=GetGTAPids();

    // Launch via PlayGTAV.exe with -nobattleye
    // This disables BattlEye so GTA launches without the anticheat service
    printf("[Launcher] Launching via PlayGTAV.exe -nobattleye...\n");
    std::wstring cmd=L"\""+playGta.wstring()+L"\" -nobattleye";
    std::vector<wchar_t> cb(cmd.begin(),cmd.end());cb.push_back(0);
    STARTUPINFOW si{};si.cb=sizeof(si);PROCESS_INFORMATION pi{};
    if(!CreateProcessW(nullptr,cb.data(),nullptr,nullptr,FALSE,
        0,nullptr,gtaDir.wstring().c_str(),&si,&pi)){
        wchar_t msg[128];swprintf_s(msg,128,L"Failed to launch PlayGTAV.exe (err %lu)",GetLastError());
        return Fatal(msg);
    }
    CloseHandle(pi.hThread);CloseHandle(pi.hProcess);
    printf("[Launcher] Waiting for GTA5.exe to appear...\n");

    DWORD gtaPid=WaitForNewGTA5(existing,120000);
    if(!gtaPid)return Fatal(L"Timed out waiting for GTA5.exe");

    printf("[Launcher] Waiting for game to finish loading");fflush(stdout);
    WaitForGRCWindow(gtaPid,300000);
    Sleep(2000);

    // Open GTA process — use token impersonation if normal open fails
    printf("[Launcher] Opening GTA5 process...\n");
    HANDLE hp=OpenProcessAnyIL(gtaPid);
    if(!hp){
        return Fatal(L"Cannot open GTA5 process.\n"
                     L"Make sure this launcher runs as Administrator.");
    }

    // Inject
    if(hasDll){
        printf("[Launcher] Injecting AtlasMP-Client.dll...\n");
        auto r=InjectDll(hp,dllDst);
        if(r.ok) printf("[Launcher] Injection OK. Connecting to %s:%d\n",
                        cfg.host.c_str(),(int)cfg.port);
        else     printf("[Launcher] Injection FAILED: %ls (err %lu)\n",
                        r.stage.c_str(),r.err);
    }
    CloseHandle(hp);

    HANDLE hw=OpenProcess(SYNCHRONIZE,FALSE,gtaPid);
    if(hw){
        printf("[Launcher] Monitoring GTA5...\n");
        WaitForSingleObject(hw,INFINITE);
        CloseHandle(hw);
    }
    printf("[Launcher] GTA5 exited.\n");
    return 0;
}
