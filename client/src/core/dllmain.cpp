// client/src/core/dllmain.cpp
// AtlasMP Client DLL
// Injected at CREATE_SUSPENDED — DllMain runs before any GTA code.
// Installs MessageBoxW hook immediately to suppress ERR_NO_LAUNCHER.

#include "Client.h"
#include "Logger.h"
#include <Windows.h>
#include <MinHook.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <algorithm>
#include <cctype>

static Atlas::Client* g_client  = nullptr;
static HMODULE        g_hModule = nullptr;

// ── ERR_NO_LAUNCHER hook ──────────────────────────────────────────────────────
typedef int (WINAPI* FnMsgW)(HWND,LPCWSTR,LPCWSTR,UINT);
typedef int (WINAPI* FnMsgA)(HWND,LPCSTR, LPCSTR, UINT);
static FnMsgW g_origMsgW = nullptr;
static FnMsgA g_origMsgA = nullptr;

static int WINAPI Hook_MsgW(HWND h,LPCWSTR txt,LPCWSTR cap,UINT t){
    if((txt&&wcsstr(txt,L"ERR_NO_LAUNCHER"))||(cap&&wcsstr(cap,L"ERR_NO_LAUNCHER")))
        return IDOK;
    return g_origMsgW(h,txt,cap,t);
}
static int WINAPI Hook_MsgA(HWND h,LPCSTR txt,LPCSTR cap,UINT t){
    if((txt&&strstr(txt,"ERR_NO_LAUNCHER"))||(cap&&strstr(cap,"ERR_NO_LAUNCHER")))
        return IDOK;
    return g_origMsgA(h,txt,cap,t);
}

static void InstallHooks(){
    // Must succeed before any game code runs
    if(MH_Initialize()!=MH_OK)return;
    HMODULE u=LoadLibraryA("user32.dll");if(!u)return;
    void* w=GetProcAddress(u,"MessageBoxW");
    void* a=GetProcAddress(u,"MessageBoxA");
    if(w){MH_CreateHook(w,(void*)Hook_MsgW,(void**)&g_origMsgW);MH_EnableHook(w);}
    if(a){MH_CreateHook(a,(void*)Hook_MsgA,(void**)&g_origMsgA);MH_EnableHook(a);}
}

// ── Config ────────────────────────────────────────────────────────────────────
namespace {
struct Cfg{std::string host="127.0.0.1";uint16_t port=7788;std::string name="AtlasPlayer",pw;bool ac=true;};
static std::string Tr(std::string v){v.erase(v.begin(),std::find_if(v.begin(),v.end(),[](unsigned char c){return!std::isspace(c);}));v.erase(std::find_if(v.rbegin(),v.rend(),[](unsigned char c){return!std::isspace(c);}).base(),v.end());return v;}
static std::string Lo(std::string v){std::transform(v.begin(),v.end(),v.begin(),[](unsigned char c){return(char)std::tolower(c);});return v;}
static std::string Uq(std::string v){if(v.size()>=2&&((v.front()=='"'&&v.back()=='"')||(v.front()=='\''&&v.back()=='\''))){v=v.substr(1,v.size()-2);}return v;}
static bool LoadCfg(const std::string& path,Cfg& c){
    std::ifstream f(path);if(!f)return false;
    std::string sec,line;
    while(std::getline(f,line)){
        auto p=line.find('#'),q=line.find(';');size_t cut=std::string::npos;
        if(p!=std::string::npos)cut=p;if(q!=std::string::npos&&(cut==std::string::npos||q<cut))cut=q;
        if(cut!=std::string::npos)line=line.substr(0,cut);line=Tr(line);if(line.empty())continue;
        if(line.front()=='['&&line.back()==']'){sec=Lo(Tr(line.substr(1,line.size()-2)));continue;}
        if(!sec.empty()&&sec!="connection")continue;
        auto eq=line.find('=');if(eq==std::string::npos)continue;
        auto k=Lo(Tr(line.substr(0,eq)));auto v=Uq(Tr(line.substr(eq+1)));
        if(k=="host"&&!v.empty())c.host=v;
        else if(k=="port"){char* e=nullptr;auto n=strtoul(v.c_str(),&e,10);if(e!=v.c_str()&&n>=1&&n<=65535)c.port=(uint16_t)n;}
        else if(k=="name"&&!v.empty())c.name=v;
        else if(k=="password")c.pw=v;
        else if(k=="auto_connect"){auto lv=Lo(v);c.ac=(lv=="true"||lv=="1"||lv=="yes");}
    }
    return true;
}
static std::string FindCfg(){
    namespace fs=std::filesystem;
    wchar_t exe[MAX_PATH]{};GetModuleFileNameW(nullptr,exe,MAX_PATH);
    wchar_t dll[MAX_PATH]{};GetModuleFileNameW(g_hModule,dll,MAX_PATH);
    for(auto d:{fs::path(exe).parent_path().string(),fs::path(dll).parent_path().string()}){
        std::string p=d+"\\atlasmp-client.toml";if(fs::exists(p))return p;
    }
    return fs::path(dll).parent_path().string()+"\\atlasmp-client.toml";
}
}

// ── Client thread ─────────────────────────────────────────────────────────────
static void ClientThread(){
    // Give GTA time to finish loading before we connect
    Sleep(8000);

    wchar_t dllW[MAX_PATH]{};GetModuleFileNameW(g_hModule,dllW,MAX_PATH);
    std::string log=std::filesystem::path(dllW).parent_path().string()+"\\AtlasMP-client.log";
    std::string cfg=FindCfg();

    Atlas::Logger::Init(log);
    Atlas::Logger::Info("[DLL] AtlasMP Client v" ATLAS_VERSION_STRING);
    Atlas::Logger::Info("[DLL] Log: %s",log.c_str());
    Atlas::Logger::Info("[DLL] Cfg: %s",cfg.c_str());
    Atlas::Logger::Info("[DLL] ERR_NO_LAUNCHER: suppressed via MessageBoxW hook");

    g_client=new Atlas::Client();
    if(!g_client->Initialize()){
        Atlas::Logger::Error("[DLL] Initialize failed");
        delete g_client;g_client=nullptr;return;
    }
    Atlas::Logger::Info("[DLL] Client initialized");

    Cfg c;
    if(LoadCfg(cfg,c))
        Atlas::Logger::Info("[DLL] Config: %s:%u name=%s",c.host.c_str(),(unsigned)c.port,c.name.c_str());
    else
        Atlas::Logger::Warn("[DLL] Config not found, using defaults");

    if(c.ac){
        Atlas::Logger::Info("[DLL] Connecting to %s:%u...",c.host.c_str(),(unsigned)c.port);
        g_client->Connect(c.host,c.port,c.name,c.pw);
    }

    Atlas::Logger::Info("[DLL] Running");
    while(g_client)Sleep(250);
    Atlas::Logger::Info("[DLL] Exiting");
}

// ── DllMain ───────────────────────────────────────────────────────────────────
BOOL APIENTRY DllMain(HMODULE hModule,DWORD reason,LPVOID lpReserved){
    switch(reason){
    case DLL_PROCESS_ATTACH:{
        // Named mutex guard — prevents double-init even if DLL is loaded
        // from two different paths (e.g. .output\ and D:\GTAV\)
        HANDLE hMutex = CreateMutexW(nullptr, TRUE, L"AtlasMP_Client_Init");
        if(GetLastError() == ERROR_ALREADY_EXISTS){
            if(hMutex) CloseHandle(hMutex);
            return TRUE;
        }
        g_hModule=hModule;
        DisableThreadLibraryCalls(hModule);
        // Install MessageBoxW hook HERE in DllMain — before ResumeThread
        // is called by the launcher. This guarantees the hook is active
        // before GTA's entry point runs a single instruction.
        // MinHook's VirtualProtect is safe to call from DllMain on Windows 10+.
        InstallHooks();
        std::thread(ClientThread).detach();
        break;
    }
    case DLL_PROCESS_DETACH:
        if(!lpReserved){
            MH_DisableHook(MH_ALL_HOOKS);
            MH_Uninitialize();
            if(g_client){g_client->Shutdown();delete g_client;g_client=nullptr;}
        }
        break;
    }
    return TRUE;
}
