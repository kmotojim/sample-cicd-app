#ifndef VEHICLE_RESOURCE_H
#define VEHICLE_RESOURCE_H

#include "VehicleService.h"
#include <nlohmann/json.hpp>
#include <httplib.h>
#include <memory>

/**
 * 車両状態を操作するREST APIエンドポイント
 */
class VehicleResource {
public:
    VehicleResource(std::shared_ptr<VehicleService> service);

    void setupRoutes(httplib::Server& server);

private:
    std::shared_ptr<VehicleService> vehicleService_;

    nlohmann::json vehicleStateToJson(const VehicleState& state) const;

    void handleGetState(const httplib::Request& req, httplib::Response& res);
    void handleAccelerate(const httplib::Request& req, httplib::Response& res);
    void handleDecelerate(const httplib::Request& req, httplib::Response& res);
    void handleChangeGear(const httplib::Request& req, httplib::Response& res);
    void handleSetSeatbelt(const httplib::Request& req, httplib::Response& res);
    void handleSetEngineError(const httplib::Request& req, httplib::Response& res);
    void handleReset(const httplib::Request& req, httplib::Response& res);
};

#endif // VEHICLE_RESOURCE_H
