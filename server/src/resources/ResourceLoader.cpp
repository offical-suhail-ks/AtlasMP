// server/src/resources/ResourceLoader.cpp
#include "ResourceLoader.h"
#include "../core/Logger.h"
#include "../../../sdk/include/IScriptRuntime.h"
#include <toml++/toml.hpp>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;
namespace Atlas {

ResourceLoader::ResourceLoader() = default;
ResourceLoader::~ResourceLoader() = default;

void ResourceLoader::RegisterRuntime(const std::string& id, IScriptRuntime* rt) {
    m_runtimes[id] = rt;
    Logger::Info("[Resources] Registered runtime: {}", id);
}

void ResourceLoader::ScanDirectory(const std::string& path) {
    if (!fs::exists(path)) {
        fs::create_directories(path);
        Logger::Info("[Resources] Created resources directory: {}", path);
        return;
    }

    for (const auto& entry : fs::directory_iterator(path)) {
        if (!entry.is_directory()) continue;

        std::string manifestPath = entry.path().string() + "/resource.toml";
        if (!fs::exists(manifestPath)) continue;

        ResourceManifest manifest = ParseManifest(manifestPath);
        if (!manifest.valid) continue;

        // Check for duplicate
        bool exists = false;
        for (auto& r : m_resources)
            if (r.manifest.name == manifest.name) { exists = true; break; }
        if (exists) continue;

        LoadedResource res;
        res.id       = m_nextId++;
        res.manifest = manifest;
        res.basePath = entry.path().string();
        res.state    = ResourceState::Unloaded;
        m_resources.push_back(std::move(res));
        Logger::Debug("[Resources] Found resource: {}", manifest.name);
    }

    Logger::Info("[Resources] Scanned {} — found {} resources",
        path, m_resources.size());
}

bool ResourceLoader::StartResource(const std::string& name) {
    auto* res = GetResource(name);
    if (!res) {
        Logger::Warn("[Resources] Resource not found: {}", name);
        return false;
    }
    if (res->state == ResourceState::Running) return true;

    // Detect language
    if (res->manifest.language.empty())
        res->manifest.language = DetectLanguage(res->manifest, res->basePath);

    // Find runtime
    res->runtime = SelectRuntime(res->manifest);
    if (!res->runtime) {
        Logger::Error("[Resources] No runtime for language '{}' in resource '{}'",
            res->manifest.language, name);
        res->state = ResourceState::Error;
        return false;
    }

    // Load into runtime
    res->state = ResourceState::Loading;
    IResource* handle = res->runtime->LoadResource(name, res->basePath);
    if (!handle) {
        Logger::Error("[Resources] Runtime failed to load resource: {}", name);
        res->state = ResourceState::Error;
        return false;
    }

    res->runtimeHandle = handle;
    res->state         = ResourceState::Running;
    Logger::Info("[Resources] Started: {} ({})", name, res->manifest.language);

    // Fire resourceStart event
    res->runtime->TriggerResourceEvent(handle, "resourceStart", {});

    if (m_onStart) m_onStart(*res);
    return true;
}

bool ResourceLoader::StopResource(const std::string& name) {
    auto* res = GetResource(name);
    if (!res || res->state != ResourceState::Running) return false;

    res->state = ResourceState::Stopping;

    if (res->runtime && res->runtimeHandle) {
        res->runtime->TriggerResourceEvent(
            static_cast<IResource*>(res->runtimeHandle), "resourceStop", {});
        res->runtime->UnloadResource(static_cast<IResource*>(res->runtimeHandle));
    }

    res->runtimeHandle = nullptr;
    res->state         = ResourceState::Unloaded;
    Logger::Info("[Resources] Stopped: {}", name);

    if (m_onStop) m_onStop(*res);
    return true;
}

bool ResourceLoader::RestartResource(const std::string& name) {
    StopResource(name);
    return StartResource(name);
}

LoadedResource* ResourceLoader::GetResource(const std::string& name) {
    for (auto& r : m_resources)
        if (r.manifest.name == name) return &r;
    return nullptr;
}

void ResourceLoader::TriggerEvent(const std::string& event,
                                   const std::vector<std::string>& /*args*/) {
    for (auto& res : m_resources) {
        if (res.state != ResourceState::Running || !res.runtime || !res.runtimeHandle)
            continue;
        res.runtime->TriggerResourceEvent(
            static_cast<IResource*>(res.runtimeHandle), event, {});
    }
}

ResourceManifest ResourceLoader::ParseManifest(const std::string& path) {
    ResourceManifest m;
    try {
        auto tbl = toml::parse_file(path);

        if (auto r = tbl["resource"].as_table()) {
            m.name        = r->get("name")        ? r->get("name")->value_or(std::string{})        : "";
            m.version     = r->get("version")     ? r->get("version")->value_or(std::string{"1.0.0"}) : "1.0.0";
            m.author      = r->get("author")      ? r->get("author")->value_or(std::string{})      : "";
            m.description = r->get("description") ? r->get("description")->value_or(std::string{}) : "";
            m.language    = r->get("language")    ? r->get("language")->value_or(std::string{})    : "";
        }

        auto loadStrArray = [&](const char* section, const char* key,
                                 std::vector<std::string>& out) {
            if (auto arr = tbl[section][key].as_array())
                for (auto& el : *arr)
                    if (auto sv = el.value<std::string>()) out.push_back(*sv);
        };

        loadStrArray("scripts", "server", m.serverScripts);
        loadStrArray("scripts", "client", m.clientScripts);
        loadStrArray("scripts", "shared", m.sharedScripts);
        loadStrArray("files",   "stream", m.files);
        loadStrArray("dependencies", "requires", m.depends);

        m.valid = !m.name.empty();
    } catch (const std::exception& e) {
        Logger::Warn("[Resources] Failed to parse manifest {}: {}", path, e.what());
    }
    return m;
}

IScriptRuntime* ResourceLoader::SelectRuntime(const ResourceManifest& manifest) {
    auto it = m_runtimes.find(manifest.language);
    if (it != m_runtimes.end()) return it->second;
    return nullptr;
}

std::string ResourceLoader::DetectLanguage(const ResourceManifest& m,
                                            const std::string& basePath) {
    // Check script file extensions
    auto checkExt = [&](const std::vector<std::string>& scripts) -> std::string {
        for (const auto& s : scripts) {
            if (s.size() > 3 && s.substr(s.size()-4) == ".lua") return "lua";
            if (s.size() > 3 && s.substr(s.size()-3) == ".js")  return "js";
            if (s.size() > 3 && s.substr(s.size()-3) == ".cs")  return "csharp";
        }
        return "";
    };

    std::string lang = checkExt(m.serverScripts);
    if (lang.empty()) lang = checkExt(m.clientScripts);
    if (lang.empty()) lang = checkExt(m.sharedScripts);

    // Fallback: look for files in basePath
    if (lang.empty()) {
        for (const auto& entry : fs::directory_iterator(basePath)) {
            auto ext = entry.path().extension().string();
            if (ext == ".lua")  { lang = "lua";    break; }
            if (ext == ".js")   { lang = "js";     break; }
            if (ext == ".cs")   { lang = "csharp"; break; }
        }
    }

    return lang.empty() ? "lua" : lang;
}

} // namespace Atlas
