#include "VehicleState.h"
#include <algorithm>
#include <stdexcept>

VehicleState::VehicleState()
    : speed_(0),
      gear_(Gear::P),
      engineWarning_(false),
      seatbeltWarning_(true),
      speedWarning_(false) {
}

int VehicleState::getSpeed() const {
    return speed_;
}

VehicleState::Gear VehicleState::getGear() const {
    return gear_;
}

bool VehicleState::isEngineWarning() const {
    return engineWarning_;
}

bool VehicleState::isSeatbeltWarning() const {
    return seatbeltWarning_;
}

bool VehicleState::isSpeedWarning() const {
    return speedWarning_;
}

void VehicleState::setSpeed(int speed) {
    speed_ = std::max(0, std::min(180, speed));
    speedWarning_ = speed_ > 120;
}

void VehicleState::setGear(Gear gear) {
    gear_ = gear;
}

void VehicleState::setEngineWarning(bool warning) {
    engineWarning_ = warning;
}

void VehicleState::setSeatbeltWarning(bool warning) {
    seatbeltWarning_ = warning;
}

void VehicleState::setSpeedWarning(bool warning) {
    speedWarning_ = warning;
}

std::string VehicleState::gearToString(Gear gear) {
    switch (gear) {
        case Gear::P: return "P";
        case Gear::R: return "R";
        case Gear::N: return "N";
        case Gear::D: return "D";
        default: return "UNKNOWN";
    }
}

VehicleState::Gear VehicleState::stringToGear(const std::string& str) {
    if (str == "P") return Gear::P;
    if (str == "R") return Gear::R;
    if (str == "N") return Gear::N;
    if (str == "D") return Gear::D;
    throw std::invalid_argument("Invalid gear: " + str);
}
