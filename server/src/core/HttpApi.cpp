// server/src/core/HttpApi.cpp
// Minimal HTTP/1.1 server — dashboard REST API
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#  define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif
#include "HttpApi.h"
#include "Server.h"
#include "Logger.h"
#include "../../../shared/include/Packets.h"
#include "../../include/PlayerManager.h"
#include "../resources/ResourceLoader.h"
#include "../database/Database.h"
#include "../network/NetworkServer.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cctype>

#pragma comment(lib, "Ws2_32.lib")

namespace Atlas {

// ── JSON escape ───────────────────────────────────────────────────────────────
std::string HttpApi::J(const std::string& s) {
    std::string o; o.reserve(s.size() + 2); o += '"';
    for (char c : s) {
        if      (c == '"')  o += "\\\"";
        else if (c == '\\') o += "\\\\";
        else if (c == '\n') o += "\\n";
        else if (c == '\r') o += "\\r";
        else if (c == '\t') o += "\\t";
        else o.push_back(c);
    }
    o += '"'; return o;
}
std::string HttpApi::StatusJson(int code, const std::string& msg) {
    std::ostringstream ss;
    ss << "{\"status\":" << code << ",\"message\":" << J(msg) << "}";
    return ss.str();
}

// ── Constructor/Destructor ────────────────────────────────────────────────────
HttpApi::HttpApi(Server* server, uint16_t port, const std::string& password)
    : m_server(server), m_port(port), m_password(password)
{
    m_logRing.resize(LOG_RING_SIZE);
}

HttpApi::~HttpApi() { Stop(); }

void HttpApi::AppendLog(const std::string& line) {
    m_logRing[m_logHead % LOG_RING_SIZE] = line;
    ++m_logHead;
    if (m_logCount < LOG_RING_SIZE) ++m_logCount;
}

// ── Start/Stop ────────────────────────────────────────────────────────────────
bool HttpApi::Start() {
    WSADATA wsa; WSAStartup(MAKEWORD(2,2), &wsa);

    m_listenSock = (int)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_listenSock < 0) {
        Logger::Error("[HttpApi] Failed to create socket");
        return false;
    }

    int yes = 1;
    setsockopt((SOCKET)m_listenSock, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(m_port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1 only

    if (bind((SOCKET)m_listenSock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        Logger::Error("[HttpApi] Bind failed on port {}", m_port);
        closesocket((SOCKET)m_listenSock); m_listenSock = -1;
        return false;
    }
    listen((SOCKET)m_listenSock, 10);

    m_running = true;
    m_thread = std::thread([this]{ ListenLoop(); });
    Logger::Info("[HttpApi] Dashboard API listening on http://127.0.0.1:{}", m_port);
    return true;
}

void HttpApi::Stop() {
    if (!m_running) return;
    m_running = false;
    if (m_listenSock >= 0) {
        closesocket((SOCKET)m_listenSock);
        m_listenSock = -1;
    }
    if (m_thread.joinable()) m_thread.join();
    WSACleanup();
}

// ── Listen loop ───────────────────────────────────────────────────────────────
void HttpApi::ListenLoop() {
    while (m_running) {
        sockaddr_in clientAddr{}; int addrLen = sizeof(clientAddr);
        SOCKET client = accept((SOCKET)m_listenSock, (sockaddr*)&clientAddr, &addrLen);
        if (client == INVALID_SOCKET) break;
        // Handle inline (blocking but fast for dashboard)
        HandleClient((int)client);
        closesocket(client);
    }
}

// ── Parse HTTP request ────────────────────────────────────────────────────────
void HttpApi::HandleClient(int sock) {
    char buf[8192]{};
    int n = recv((SOCKET)sock, buf, sizeof(buf)-1, 0);
    if (n <= 0) return;
    buf[n] = '\0';

    Request req;
    std::istringstream ss(buf);
    std::string line;

    // Parse request line: "GET /path HTTP/1.1"
    if (!std::getline(ss, line)) return;
    std::istringstream rl(line);
    std::string ver;
    rl >> req.method >> req.path >> ver;

    // Parse headers
    size_t contentLength = 0;
    while (std::getline(ss, line) && line != "\r" && !line.empty()) {
        if (line.back() == '\r') line.pop_back();
        if (line.rfind("Content-Length:", 0) == 0)
            contentLength = std::stoul(line.substr(16));
        if (line.rfind("X-Dashboard-Password:", 0) == 0)
            req.password = line.substr(22);
    }

    // Body
    if (contentLength > 0) {
        // Find body after \r\n\r\n
        const char* bodyStart = strstr(buf, "\r\n\r\n");
        if (bodyStart) req.body = std::string(bodyStart + 4, contentLength);
    }

    // Auth check
    if (!m_password.empty() && req.password != m_password) {
        const char* r = "HTTP/1.1 401 Unauthorized\r\nContent-Type: application/json\r\nContent-Length: 28\r\nAccess-Control-Allow-Origin: *\r\n\r\n{\"error\":\"Unauthorized\"}";
        send((SOCKET)sock, r, (int)strlen(r), 0);
        return;
    }

    // Handle CORS preflight
    if (req.method == "OPTIONS") {
        const char* r = "HTTP/1.1 204 No Content\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: GET,POST,DELETE,PUT\r\nAccess-Control-Allow-Headers: Content-Type,X-Dashboard-Password\r\n\r\n";
        send((SOCKET)sock, r, (int)strlen(r), 0);
        return;
    }

    Response resp = Route(req);

    std::ostringstream out;
    out << "HTTP/1.1 " << resp.status << " OK\r\n";
    out << "Content-Type: " << resp.contentType << "\r\n";
    out << "Content-Length: " << resp.body.size() << "\r\n";
    out << "Access-Control-Allow-Origin: *\r\n";
    out << "\r\n";
    out << resp.body;
    std::string raw = out.str();
    send((SOCKET)sock, raw.c_str(), (int)raw.size(), 0);
}

// ── Router ────────────────────────────────────────────────────────────────────
HttpApi::Response HttpApi::Route(const Request& req) {
    // Strip query string
    std::string path = req.path;
    auto qpos = path.find('?');
    if (qpos != std::string::npos) path = path.substr(0, qpos);

    if (path == "/api/v1/status"   || path == "/api/v1/status/")
        return RouteStatus(req);
    if (path.rfind("/api/v1/players", 0) == 0)
        return RoutePlayers(req);
    if (path.rfind("/api/v1/resources", 0) == 0)
        return RouteResources(req);
    if (path.rfind("/api/v1/bans", 0) == 0)
        return RouteBans(req);
    if (path.rfind("/api/v1/console", 0) == 0)
        return RouteConsole(req);

    Response r; r.status = 404;
    r.body = StatusJson(404, "Not Found");
    return r;
}

// ── /api/v1/status ────────────────────────────────────────────────────────────
HttpApi::Response HttpApi::RouteStatus(const Request&) {
    static auto startTime = std::chrono::steady_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - startTime).count();

    auto* pm = m_server->GetPlayers();
    uint32_t playerCount = pm ? pm->GetPlayerCount() : 0;
    auto& cfg = m_server->GetConfig();

    std::ostringstream ss;
    ss << "{"
       << "\"name\":" << J(cfg.GetServerName()) << ","
       << "\"version\":\"" ATLAS_VERSION_STRING "\","
       << "\"playerCount\":" << playerCount << ","
       << "\"maxPlayers\":" << cfg.GetMaxPlayers() << ","
       << "\"uptime\":" << uptime << ","
       << "\"port\":" << cfg.GetPort() << ","
       << "\"tps\":60"
       << "}";
    Response r; r.body = ss.str(); return r;
}

// ── /api/v1/players ───────────────────────────────────────────────────────────
HttpApi::Response HttpApi::RoutePlayers(const Request& req) {
    std::string path = req.path;

    // POST /api/v1/players/:id/kick
    auto kickPos = path.find("/kick");
    if (req.method == "POST" && kickPos != std::string::npos) {
        std::string idStr = path.substr(path.rfind('/', kickPos-1) + 1,
                                         kickPos - path.rfind('/', kickPos-1) - 1);
        try {
            uint16_t pid = (uint16_t)std::stoul(idStr);
            if (auto* net = m_server->GetNetwork())
                net->KickPlayer(pid, "Kicked from dashboard");
            Response r; r.body = StatusJson(200, "Kicked"); return r;
        } catch(...) {}
        Response r; r.status=400; r.body=StatusJson(400,"Bad id"); return r;
    }

    // GET /api/v1/players
    auto* pm = m_server->GetPlayers();
    std::ostringstream ss; ss << "[";
    bool first = true;
    if (pm) {
        pm->ForEach([&](Player& p) {
            if (!first) ss << ",";
            ss << "{"
               << "\"id\":" << p.id << ","
               << "\"name\":" << J(p.name) << ","
               << "\"ip\":" << J(p.ip) << ","
               << "\"ping\":0"
               << "}";
            first = false;
        });
    }
    ss << "]";
    Response r; r.body = ss.str(); return r;
}

// ── /api/v1/resources ─────────────────────────────────────────────────────────
HttpApi::Response HttpApi::RouteResources(const Request& req) {
    auto* rl = m_server->GetResources();
    if (!rl) { Response r; r.body="[]"; return r; }

    std::ostringstream ss; ss << "[";
    bool first = true;
    for (const auto& res : rl->GetAll()) {
        if (!first) ss << ",";
        ss << "{"
           << "\"name\":" << J(res.manifest.name) << ","
           << "\"running\":" << (res.running ? "true" : "false") << ","
           << "\"author\":" << J(res.manifest.author) << ","
           << "\"version\":" << J(res.manifest.version)
           << "}";
        first = false;
    }
    ss << "]";

    // POST /api/v1/resources/:name/start  or /stop
    if (req.method == "POST") {
        std::string path = req.path;
        bool isStart = path.find("/start") != std::string::npos;
        bool isStop  = path.find("/stop")  != std::string::npos;
        if (isStart || isStop) {
            // extract resource name
            auto p1 = path.find("/resources/");
            auto p2 = path.rfind('/');
            if (p1 != std::string::npos && p2 > p1 + 11) {
                std::string name = path.substr(p1 + 11, p2 - p1 - 11);
                if (isStart) rl->StartResource(name);
                else         rl->StopResource(name);
                Response r; r.body = StatusJson(200, isStart ? "Started" : "Stopped");
                return r;
            }
        }
    }

    Response r; r.body = ss.str(); return r;
}

// ── /api/v1/bans ─────────────────────────────────────────────────────────────
HttpApi::Response HttpApi::RouteBans(const Request& req) {
    auto* db = m_server->GetDatabase();
    if (!db) { Response r; r.body="[]"; return r; }

    // DELETE /api/v1/bans/:id
    if (req.method == "DELETE") {
        std::string path = req.path;
        auto pos = path.rfind('/');
        if (pos != std::string::npos && pos < path.size()-1) {
            try {
                int id = std::stoi(path.substr(pos+1));
                db->RemoveBan(id);
                Response r; r.body = StatusJson(200, "Removed"); return r;
            } catch(...) {}
        }
        Response r; r.status=400; r.body=StatusJson(400,"Bad id"); return r;
    }

    // POST /api/v1/bans  — simple body: {"name":"x","ip":"y","reason":"z"}
    if (req.method == "POST") {
        BanEntry b;
        // Minimal JSON parse
        auto jstr = [&](const std::string& key) {
            auto p = req.body.find("\""+key+"\"");
            if (p == std::string::npos) return std::string{};
            p = req.body.find('"', req.body.find(':',p));
            if (p == std::string::npos) return std::string{};
            ++p; std::string v;
            while (p < req.body.size() && req.body[p] != '"') v.push_back(req.body[p++]);
            return v;
        };
        b.playerName = jstr("name");
        b.ip         = jstr("ip");
        b.reason     = jstr("reason");
        b.adminName  = "dashboard";
        if (!b.playerName.empty() || !b.ip.empty()) {
            db->AddBan(b);
            Response r; r.body = StatusJson(200, "Banned"); return r;
        }
        Response r; r.status=400; r.body=StatusJson(400,"Missing fields"); return r;
    }

    // GET all bans
    auto bans = db->GetBans();
    std::ostringstream ss; ss << "[";
    bool first = true;
    for (auto& b : bans) {
        if (!first) ss << ",";
        ss << "{"
           << "\"id\":" << b.id << ","
           << "\"name\":" << J(b.playerName) << ","
           << "\"ip\":" << J(b.ip) << ","
           << "\"reason\":" << J(b.reason) << ","
           << "\"admin\":" << J(b.adminName)
           << "}";
        first = false;
    }
    ss << "]";
    Response r; r.body = ss.str(); return r;
}

// ── /api/v1/console ───────────────────────────────────────────────────────────
HttpApi::Response HttpApi::RouteConsole(const Request& req) {
    if (req.method == "POST") {
        // Execute console command — TODO: route to server command handler
        Logger::Info("[Console] Dashboard command: {}", req.body);
        Response r; r.body = StatusJson(200, "OK"); return r;
    }

    // GET — return last N log lines
    std::ostringstream ss; ss << "[";
    size_t count = std::min(m_logCount, LOG_RING_SIZE);
    size_t start = (m_logHead >= count) ? m_logHead - count : 0;
    bool first = true;
    for (size_t i = 0; i < count; ++i) {
        size_t idx = (start + i) % LOG_RING_SIZE;
        if (!m_logRing[idx].empty()) {
            if (!first) ss << ",";
            ss << J(m_logRing[idx]);
            first = false;
        }
    }
    ss << "]";
    Response r; r.body = ss.str(); return r;
}

} // namespace Atlas
