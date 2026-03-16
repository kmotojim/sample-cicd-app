#include "VehicleResource.h"
#include <iostream>
#include <memory>
#include <csignal>
#include <filesystem>
#include <chrono>

std::unique_ptr<httplib::Server> server;
auto startTime = std::chrono::steady_clock::now();

void signalHandler(int signum) {
    std::cout << "\nShutting down..." << std::endl;
    if (server) {
        server->stop();
    }
    exit(signum);
}

int main() {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    auto vehicleService = std::make_shared<VehicleService>();
    auto vehicleResource = std::make_unique<VehicleResource>(vehicleService);

    server = std::make_unique<httplib::Server>();

    server->set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type"}
    });

    std::string publicDir = "./public";
    if (!server->set_mount_point("/", publicDir)) {
        std::cerr << "Warning: static file directory not found: " << publicDir << std::endl;
    } else {
        std::cout << "Serving static files from: " << std::filesystem::absolute(publicDir).string() << std::endl;
    }

    vehicleResource->setupRoutes(*server);

    server->Get("/health", [](const httplib::Request&, httplib::Response& res) {
        auto now = std::chrono::steady_clock::now();
        auto uptimeSec = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
        nlohmann::json health = {
            {"status", "UP"},
            {"uptimeSeconds", uptimeSec}
        };
        res.set_content(health.dump(), "application/json");
    });

    const char* host = "0.0.0.0";
    int port = 8080;

    std::cout << "Smart Mobility Dashboard Backend (C++) v1.0" << std::endl;
    std::cout << "Server starting: http://" << host << ":" << port << std::endl;

    if (!server->listen(host, port)) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }

    return 0;
}
