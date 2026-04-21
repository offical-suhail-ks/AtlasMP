// ExecutableLoader.cpp
// Uses raw C function pointers (not std::function) so __try/__except
// works correctly — MSVC cannot mix SEH and C++ object unwinding.
#include <stdlib.h>
#include <math.h>
#include <windows.h>
#include "ExecutableLoader.h"
#include <stdexcept>
#include <cstring>
#include <cstdio>

ExecutableLoader::ExecutableLoader(const uint8_t* data) : m_data(data) {}
ExecutableLoader::~ExecutableLoader() {}

// Plain C struct — no destructor — safe to use inside __try
struct LoadResult { HMODULE h; DWORD seh; };

// Each of these is a standalone plain function with NO C++ objects.
// That's the only way __try/__except actually catches SEH on MSVC.
static LoadResult TryLoadLibA(const char* name, FnLibLoader fn, void* ctx)
{
    LoadResult r{};
    __try { r.h = fn(name, ctx); }
    __except(r.seh = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER) {}
    return r;
}

static LPVOID TryGetProc(HMODULE mod, const char* name,
                          FnFuncResolver fn, void* ctx)
{
    LPVOID addr = nullptr;
    __try { addr = fn(mod, name, ctx); }
    __except(EXCEPTION_EXECUTE_HANDLER) {}
    return addr;
}

void ExecutableLoader::Load()
{
    auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(m_data);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        throw std::runtime_error("Invalid DOS signature");

    auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(m_data + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE)
        throw std::runtime_error("Invalid PE signature");

    DWORD imgSize = nt->OptionalHeader.SizeOfImage;

    m_base = VirtualAlloc(
        reinterpret_cast<LPVOID>(nt->OptionalHeader.ImageBase),
        imgSize, MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!m_base)
        m_base = VirtualAlloc(nullptr, imgSize,
            MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!m_base)
        throw std::runtime_error("VirtualAlloc failed");

    m_ownsBase = true;
    printf("[Loader] Allocated %u KB at 0x%llX\n",
        imgSize/1024, (unsigned long long)(uintptr_t)m_base);

    ZeroMemory(m_base, imgSize);
    memcpy(m_base, m_data, nt->OptionalHeader.SizeOfHeaders);
    printf("[Loader] Headers copied. Mapping sections...\n"); fflush(stdout);

    MapSections(const_cast<PIMAGE_NT_HEADERS>(nt));
    printf("[Loader] Sections mapped. Applying relocs...\n"); fflush(stdout);

    ApplyRelocs(const_cast<PIMAGE_NT_HEADERS>(nt));
    printf("[Loader] Relocs applied. Loading imports...\n"); fflush(stdout);

    LoadImports(const_cast<PIMAGE_NT_HEADERS>(nt));
    printf("[Loader] Imports done.\n"); fflush(stdout);

    m_entryPoint = RVA<void>(nt->OptionalHeader.AddressOfEntryPoint);
    printf("[Loader] Entry point: 0x%llX\n",
        (unsigned long long)(uintptr_t)m_entryPoint);
}

void ExecutableLoader::MapSections(PIMAGE_NT_HEADERS nt)
{
    uintptr_t base   = reinterpret_cast<uintptr_t>(m_base);
    uintptr_t imgEnd = base + nt->OptionalHeader.SizeOfImage;
    auto* sec = IMAGE_FIRST_SECTION(nt);

    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++sec)
    {
        if (m_loadLimit && sec->VirtualAddress >= m_loadLimit) continue;
        uintptr_t dst = base + sec->VirtualAddress;
        if (dst >= imgEnd) continue;

        const uint8_t* src  = m_data + sec->PointerToRawData;
        DWORD raw  = sec->SizeOfRawData;
        DWORD virt = sec->Misc.VirtualSize;
        if (raw)        memcpy(reinterpret_cast<void*>(dst), src, raw);
        if (virt > raw) memset(reinterpret_cast<void*>(dst+raw), 0, virt-raw);
    }
}

void ExecutableLoader::ApplyRelocs(PIMAGE_NT_HEADERS nt)
{
    uintptr_t base      = reinterpret_cast<uintptr_t>(m_base);
    uintptr_t preferred = (uintptr_t)nt->OptionalHeader.ImageBase;
    intptr_t  delta     = (intptr_t)(base - preferred);
    if (!delta) return;

    auto& dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    if (!dir.Size) return;

    auto* blk    = RVA<IMAGE_BASE_RELOCATION>(dir.VirtualAddress);
    auto* endBlk = reinterpret_cast<IMAGE_BASE_RELOCATION*>(
        reinterpret_cast<uint8_t*>(blk) + dir.Size);

    while (blk < endBlk && blk->SizeOfBlock) {
        size_t    n   = (blk->SizeOfBlock - sizeof(*blk)) / 2;
        uint16_t* ent = reinterpret_cast<uint16_t*>(blk + 1);
        for (size_t j = 0; j < n; ++j) {
            int type = ent[j] >> 12, off = ent[j] & 0xFFF;
            if      (type == IMAGE_REL_BASED_DIR64)
                *RVA<uintptr_t>(blk->VirtualAddress + off) += (uintptr_t)delta;
            else if (type == IMAGE_REL_BASED_HIGHLOW)
                *RVA<uint32_t>(blk->VirtualAddress + off)  += (uint32_t)delta;
        }
        blk = reinterpret_cast<IMAGE_BASE_RELOCATION*>(
            reinterpret_cast<uint8_t*>(blk) + blk->SizeOfBlock);
    }
}

void ExecutableLoader::LoadImports(PIMAGE_NT_HEADERS nt)
{
    auto& dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!dir.Size) return;

    auto* desc = RVA<IMAGE_IMPORT_DESCRIPTOR>(dir.VirtualAddress);

    for (; desc->Name; ++desc)
    {
        const char* libName = RVA<char>(desc->Name);
        printf("[Loader]   %-44s ", libName); fflush(stdout);

        // TryLoadLibA is a plain C function — __try inside actually works
        LoadResult lr = TryLoadLibA(libName, m_libLoader, m_libCtx);
        HMODULE hLib  = lr.h;

        if (lr.seh) {
            printf("CRASHED loading (SEH 0x%08X) — skipped\n", lr.seh);
            fflush(stdout);
            continue;
        }
        printf("%s\n", hLib ? (hLib==INVALID_HANDLE_VALUE ? "(stub)" : "OK") : "NOT FOUND");
        fflush(stdout);

        if (!hLib || hLib == INVALID_HANDLE_VALUE) continue;

        uint32_t intRVA = desc->OriginalFirstThunk
            ? desc->OriginalFirstThunk : desc->FirstThunk;
        auto* thunk = RVA<uintptr_t>(intRVA);
        auto* iat   = RVA<uintptr_t>(desc->FirstThunk);

        for (; *thunk; ++thunk, ++iat)
        {
            LPVOID addr = nullptr;

            if (IMAGE_SNAP_BY_ORDINAL(*thunk)) {
                addr = TryGetProc(hLib,
                    reinterpret_cast<const char*>(IMAGE_ORDINAL(*thunk)),
                    m_funcResolver, m_funcCtx);
            } else {
                const char* fn =
                    RVA<IMAGE_IMPORT_BY_NAME>((uint32_t)*thunk)->Name;
                addr = TryGetProc(hLib, fn, m_funcResolver, m_funcCtx);
                if (!addr)
                    addr = (LPVOID)GetProcAddress(
                        GetModuleHandleA("kernel32.dll"), fn);
            }

            if (addr) *iat = reinterpret_cast<uintptr_t>(addr);
        }
    }
}
