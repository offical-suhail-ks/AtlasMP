#pragma once
// server/src/core/HttpApi.h
// Lightweight HTTP/1.1 server for the dashboard REST API.
// Uses only Winsock — no external HTTP library dependency.
// Endpoints:
//   GET  /api/v1/status          → server info, player count, uptime
//   GET  /api/v1/players         → player list
//   POST /api/v1/players/:id/kick → kick player
//   GET  /api/v1/resources       → resource list
//   POST /api/v1/resources/:name/start|stop → manage resources
//   GET  /api/v1/bans            → ban list
//   POST /api/v1/bans            → add ban
//   DELETE /api/v1/bans/:id      → remove ban
//   GET  /api/v1/console         → last N log lines
//   POST /api/v1/console         → execute server command

#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <memory>

namespace Atlas {

class Server;

class HttpApi {
public:
    explicit HttpApi(Server* server, uint16_t port = 7789,
                     const std::string& password = "");
    ~HttpApi();

    bool Start();
    void Stop();

    // Called by Logger to append to in-memory console ring buffer
    void AppendLog(const std::string& line);

private:
    void ListenLoop();
    void HandleClient(int sock);

    struct Request {
        std::string method;
        std::string path;
        std::string body;
        std::string password; // from X-Dashboard-Password header
    };

    struct Response {
        int         status = 200;
        std::string body;
        std::string contentType = "application/json";
    };

    Response Route(const Request& req);
    Response RouteStatus(const Request& req);
    Response RoutePlayers(const Request& req);
    Response RouteResources(const Request& req);
    Response RouteBans(const Request& req);
    Response RouteConsole(const Request& req);

    // JSON helpers
    static std::string J(const std::string& s);  // escape string
    static std::string StatusJson(int code, const std::string& msg);

    Server*           m_server;
    uint16_t          m_port;
    std::string       m_password;
    std::atomic<bool> m_running{false};
    std::thread       m_thread;
    int               m_listenSock = -1;

    // Console ring buffer (last 500 lines)
    static constexpr size_t LOG_RING_SIZE = 500;
    std::vector<std::string> m_logRing;
    size_t m_logHead = 0;
    size_t m_logCount = 0;
};

} // namespace Atlas
