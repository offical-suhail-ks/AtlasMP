#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>

// Raw C function pointer types — NO std::function, no destructors.
// This is required so __try/__except can catch SEH inside the loader.
typedef HMODULE (*FnLibLoader)   (const char* libName, void* ctx);
typedef LPVOID  (*FnFuncResolver)(HMODULE mod, const char* funcName, void* ctx);

class ExecutableLoader
{
public:
    explicit ExecutableLoader(const uint8_t* data);
    ~ExecutableLoader();

    void SetLoadLimit    (uintptr_t limit)     { m_loadLimit    = limit; }
    void SetLibraryLoader(FnLibLoader fn, void* ctx=nullptr)
        { m_libLoader = fn; m_libCtx = ctx; }
    void SetFunctionResolver(FnFuncResolver fn, void* ctx=nullptr)
        { m_funcResolver = fn; m_funcCtx = ctx; }

    void  Load();
    void* GetBase()       const { return m_base; }
    void* GetEntryPoint() const { return m_entryPoint; }

private:
    template<typename T>
    T* RVA(uint32_t rva) {
        return reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(m_base) + rva);
    }
    void MapSections (PIMAGE_NT_HEADERS nt);
    void ApplyRelocs (PIMAGE_NT_HEADERS nt);
    void LoadImports (PIMAGE_NT_HEADERS nt);

    const uint8_t*  m_data        = nullptr;
    void*           m_base        = nullptr;
    void*           m_entryPoint  = nullptr;
    uintptr_t       m_loadLimit   = 0;
    bool            m_ownsBase    = false;
    FnLibLoader     m_libLoader   = nullptr;
    FnFuncResolver  m_funcResolver= nullptr;
    void*           m_libCtx      = nullptr;
    void*           m_funcCtx     = nullptr;
};
