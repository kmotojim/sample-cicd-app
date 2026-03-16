#include "VehicleService.h"
#include <algorithm>
#include <stdexcept>

VehicleService::VehicleService() : state_() {
}

VehicleState& VehicleService::getState() {
    return state_;
}

const VehicleState& VehicleService::getState() const {
    return state_;
}

VehicleState& VehicleService::accelerate() {
    if (state_.getGear() == VehicleState::Gear::D) {
        int newSpeed = std::min(state_.getSpeed() + SPEED_INCREMENT, MAX_SPEED);
        state_.setSpeed(newSpeed);
    }
    return state_;
}

VehicleState& VehicleService::decelerate() {
    int newSpeed = std::max(state_.getSpeed() - SPEED_DECREMENT, MIN_SPEED);
    state_.setSpeed(newSpeed);
    return state_;
}

VehicleState& VehicleService::changeGear(VehicleState::Gear gear) {
    if (state_.getSpeed() == 0) {
        state_.setGear(gear);
    }
    return state_;
}

VehicleState& VehicleService::setSeatbelt(bool fastened) {
    state_.setSeatbeltWarning(!fastened);
    return state_;
}

VehicleState& VehicleService::setEngineError(bool hasError) {
    state_.setEngineWarning(hasError);
    if (hasError && state_.getSpeed() > 60) {
        state_.setSpeed(60);
    }
    return state_;
}

VehicleState& VehicleService::reset() {
    state_.setSpeed(0);
    state_.setGear(VehicleState::Gear::P);
    state_.setEngineWarning(false);
    state_.setSeatbeltWarning(true);
    return state_;
}

int VehicleService::calculateDeceleration(int currentSpeed, double brakeForce) {
    if (brakeForce < 0.0 || brakeForce > 1.0) {
        throw std::invalid_argument("brakeForce must be between 0.0 and 1.0");
    }
    if (currentSpeed < 0) {
        throw std::invalid_argument("currentSpeed must be non-negative");
    }

    int baseDeceleration = static_cast<int>(10 * brakeForce);

    if (currentSpeed > 100) {
        baseDeceleration = static_cast<int>(baseDeceleration * 0.8);
    }

    return baseDeceleration;
}
