#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>
#include <httplib.h>
#include "VehicleResource.h"

using json = nlohmann::json;

class VehicleResourceTest : public ::testing::Test {
protected:
    static std::unique_ptr<httplib::Server> server;
    static std::shared_ptr<VehicleService> vehicleService;
    static std::unique_ptr<VehicleResource> vehicleResource;
    static std::thread serverThread;
    static int port;

    static void SetUpTestSuite() {
        vehicleService = std::make_shared<VehicleService>();
        vehicleResource = std::make_unique<VehicleResource>(vehicleService);
        server = std::make_unique<httplib::Server>();

        server->set_default_headers({
            {"Access-Control-Allow-Origin", "*"},
            {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
            {"Access-Control-Allow-Headers", "Content-Type"}
        });

        vehicleResource->setupRoutes(*server);

        port = server->bind_to_any_port("127.0.0.1");
        serverThread = std::thread([&]() {
            server->listen_after_bind();
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    static void TearDownTestSuite() {
        server->stop();
        if (serverThread.joinable()) {
            serverThread.join();
        }
    }

    void SetUp() override {
        httplib::Client cli("127.0.0.1", port);
        auto res = cli.Post("/api/vehicle/reset");
        ASSERT_TRUE(res);
        ASSERT_EQ(200, res->status);
    }

    httplib::Client createClient() {
        return httplib::Client("127.0.0.1", port);
    }
};

std::unique_ptr<httplib::Server> VehicleResourceTest::server;
std::shared_ptr<VehicleService> VehicleResourceTest::vehicleService;
std::unique_ptr<VehicleResource> VehicleResourceTest::vehicleResource;
std::thread VehicleResourceTest::serverThread;
int VehicleResourceTest::port = 0;

TEST_F(VehicleResourceTest, GetStateReturnsInitialState) {
    auto cli = createClient();
    auto res = cli.Get("/api/vehicle/state");
    ASSERT_TRUE(res);
    EXPECT_EQ(200, res->status);

    auto body = json::parse(res->body);
    EXPECT_EQ(0, body["speed"].get<int>());
    EXPECT_EQ("P", body["gear"].get<std::string>());
    EXPECT_FALSE(body["engineWarning"].get<bool>());
    EXPECT_TRUE(body["seatbeltWarning"].get<bool>());
}

TEST_F(VehicleResourceTest, AccelerateInDGear) {
    auto cli = createClient();
    cli.Post("/api/vehicle/gear/D");
    auto res = cli.Post("/api/vehicle/accelerate");
    ASSERT_TRUE(res);
    EXPECT_EQ(200, res->status);
    auto body = json::parse(res->body);
    EXPECT_EQ(10, body["speed"].get<int>());
}

TEST_F(VehicleResourceTest, DecelerateReducesSpeed) {
    auto cli = createClient();
    cli.Post("/api/vehicle/gear/D");
    cli.Post("/api/vehicle/accelerate");
    cli.Post("/api/vehicle/accelerate");
    auto res = cli.Post("/api/vehicle/decelerate");
    ASSERT_TRUE(res);
    auto body = json::parse(res->body);
    EXPECT_EQ(10, body["speed"].get<int>());
}

TEST_F(VehicleResourceTest, ChangeGearToD) {
    auto cli = createClient();
    auto res = cli.Post("/api/vehicle/gear/D");
    ASSERT_TRUE(res);
    EXPECT_EQ(200, res->status);
    auto body = json::parse(res->body);
    EXPECT_EQ("D", body["gear"].get<std::string>());
}

TEST_F(VehicleResourceTest, InvalidGearReturns400) {
    auto cli = createClient();
    auto res = cli.Post("/api/vehicle/gear/X");
    ASSERT_TRUE(res);
    EXPECT_EQ(400, res->status);
    auto body = json::parse(res->body);
    EXPECT_TRUE(body["error"].get<std::string>().find("Invalid gear") != std::string::npos);
}

TEST_F(VehicleResourceTest, LowercaseGearIsAccepted) {
    auto cli = createClient();
    auto res = cli.Post("/api/vehicle/gear/d");
    ASSERT_TRUE(res);
    EXPECT_EQ(200, res->status);
    auto body = json::parse(res->body);
    EXPECT_EQ("D", body["gear"].get<std::string>());
}

TEST_F(VehicleResourceTest, SeatbeltToggle) {
    auto cli = createClient();
    auto res = cli.Post("/api/vehicle/seatbelt/true");
    ASSERT_TRUE(res);
    auto body = json::parse(res->body);
    EXPECT_FALSE(body["seatbeltWarning"].get<bool>());
}

TEST_F(VehicleResourceTest, EngineErrorToggle) {
    auto cli = createClient();
    auto res = cli.Post("/api/vehicle/engine-error/true");
    ASSERT_TRUE(res);
    auto body = json::parse(res->body);
    EXPECT_TRUE(body["engineWarning"].get<bool>());
}

TEST_F(VehicleResourceTest, ResetRestoresDefaults) {
    auto cli = createClient();
    cli.Post("/api/vehicle/gear/D");
    cli.Post("/api/vehicle/accelerate");
    cli.Post("/api/vehicle/engine-error/true");

    auto res = cli.Post("/api/vehicle/reset");
    ASSERT_TRUE(res);
    auto body = json::parse(res->body);
    EXPECT_EQ(0, body["speed"].get<int>());
    EXPECT_EQ("P", body["gear"].get<std::string>());
    EXPECT_FALSE(body["engineWarning"].get<bool>());
}

TEST_F(VehicleResourceTest, SpeedWarningWhenExceeding120) {
    auto cli = createClient();
    cli.Post("/api/vehicle/gear/D");
    for (int i = 0; i < 13; i++) {
        cli.Post("/api/vehicle/accelerate");
    }
    auto res = cli.Get("/api/vehicle/state");
    ASSERT_TRUE(res);
    auto body = json::parse(res->body);
    EXPECT_EQ(130, body["speed"].get<int>());
    EXPECT_TRUE(body["speedWarning"].get<bool>());
}

TEST_F(VehicleResourceTest, CannotChangeGearWhileMoving) {
    auto cli = createClient();
    cli.Post("/api/vehicle/gear/D");
    cli.Post("/api/vehicle/accelerate");
    auto res = cli.Post("/api/vehicle/gear/P");
    ASSERT_TRUE(res);
    auto body = json::parse(res->body);
    EXPECT_EQ("D", body["gear"].get<std::string>());
}

TEST_F(VehicleResourceTest, EngineErrorForcesDecelerationAtHighSpeed) {
    auto cli = createClient();
    cli.Post("/api/vehicle/gear/D");
    for (int i = 0; i < 10; i++) {
        cli.Post("/api/vehicle/accelerate");
    }
    auto res = cli.Post("/api/vehicle/engine-error/true");
    ASSERT_TRUE(res);
    auto body = json::parse(res->body);
    EXPECT_EQ(60, body["speed"].get<int>());
    EXPECT_TRUE(body["engineWarning"].get<bool>());
}
