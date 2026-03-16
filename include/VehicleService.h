#ifndef VEHICLE_SERVICE_H
#define VEHICLE_SERVICE_H

#include "VehicleState.h"
#include <memory>

/**
 * 車両状態を管理するビジネスロジッククラス
 */
class VehicleService {
public:
    VehicleService();

    VehicleState& getState();
    const VehicleState& getState() const;

    VehicleState& accelerate();
    VehicleState& decelerate();
    VehicleState& changeGear(VehicleState::Gear gear);
    VehicleState& setSeatbelt(bool fastened);
    VehicleState& setEngineError(bool hasError);
    VehicleState& reset();

    /**
     * 速度から減速度を計算（ブレーキロジック）
     * @param currentSpeed 現在速度
     * @param brakeForce ブレーキ力 (0.0-1.0)
     * @return 減速度 (km/h/s)
     */
    static int calculateDeceleration(int currentSpeed, double brakeForce);

private:
    static constexpr int SPEED_INCREMENT = 10;
    static constexpr int SPEED_DECREMENT = 10;
    static constexpr int MAX_SPEED = 180;
    static constexpr int MIN_SPEED = 0;
    static constexpr int SPEED_WARNING_THRESHOLD = 120;

    VehicleState state_;
};

#endif // VEHICLE_SERVICE_H
