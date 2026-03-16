#include <gtest/gtest.h>
#include "VehicleService.h"

class VehicleServiceTest : public ::testing::Test {
protected:
    VehicleService vehicleService;

    void SetUp() override {
        vehicleService.reset();
    }
};

// ===== Accelerate =====

TEST_F(VehicleServiceTest, AccelerateInDGearIncreasesSpeed) {
    vehicleService.changeGear(VehicleState::Gear::D);
    VehicleState& result = vehicleService.accelerate();
    EXPECT_EQ(10, result.getSpeed());
}

TEST_F(VehicleServiceTest, AccelerateInPGearDoesNothing) {
    VehicleState& result = vehicleService.accelerate();
    EXPECT_EQ(0, result.getSpeed());
}

TEST_F(VehicleServiceTest, AccelerateInRGearDoesNothing) {
    vehicleService.changeGear(VehicleState::Gear::R);
    VehicleState& result = vehicleService.accelerate();
    EXPECT_EQ(0, result.getSpeed());
}

TEST_F(VehicleServiceTest, SpeedDoesNotExceed180) {
    vehicleService.changeGear(VehicleState::Gear::D);
    for (int i = 0; i < 20; i++) {
        vehicleService.accelerate();
    }
    EXPECT_EQ(180, vehicleService.getState().getSpeed());
}

TEST_F(VehicleServiceTest, ConsecutiveAccelerationAccumulates) {
    vehicleService.changeGear(VehicleState::Gear::D);
    vehicleService.accelerate();
    vehicleService.accelerate();
    VehicleState& result = vehicleService.accelerate();
    EXPECT_EQ(30, result.getSpeed());
}

// ===== Decelerate =====

TEST_F(VehicleServiceTest, DecelerateReducesSpeed) {
    vehicleService.changeGear(VehicleState::Gear::D);
    vehicleService.accelerate();
    vehicleService.accelerate();
    vehicleService.accelerate();
    VehicleState& result = vehicleService.decelerate();
    EXPECT_EQ(20, result.getSpeed());
}

TEST_F(VehicleServiceTest, SpeedDoesNotGoBelowZero) {
    VehicleState& result = vehicleService.decelerate();
    EXPECT_EQ(0, result.getSpeed());
}

// ===== Gear change =====

TEST_F(VehicleServiceTest, CanChangeGearWhenStopped) {
    VehicleState& result = vehicleService.changeGear(VehicleState::Gear::D);
    EXPECT_EQ(VehicleState::Gear::D, result.getGear());
}

TEST_F(VehicleServiceTest, CannotChangeGearWhileMoving) {
    vehicleService.changeGear(VehicleState::Gear::D);
    vehicleService.accelerate();
    VehicleState& result = vehicleService.changeGear(VehicleState::Gear::P);
    EXPECT_EQ(VehicleState::Gear::D, result.getGear());
}

// ===== Reset =====

TEST_F(VehicleServiceTest, ResetSetsSpeedToZero) {
    vehicleService.changeGear(VehicleState::Gear::D);
    vehicleService.accelerate();
    VehicleState& result = vehicleService.reset();
    EXPECT_EQ(0, result.getSpeed());
}

TEST_F(VehicleServiceTest, ResetSetsGearToP) {
    vehicleService.changeGear(VehicleState::Gear::D);
    VehicleState& result = vehicleService.reset();
    EXPECT_EQ(VehicleState::Gear::P, result.getGear());
}

TEST_F(VehicleServiceTest, ResetClearsEngineWarning) {
    vehicleService.setEngineError(true);
    VehicleState& result = vehicleService.reset();
    EXPECT_FALSE(result.isEngineWarning());
}

// ===== Warnings =====

TEST_F(VehicleServiceTest, SpeedWarningAbove120) {
    vehicleService.changeGear(VehicleState::Gear::D);
    for (int i = 0; i < 13; i++) {
        vehicleService.accelerate();
    }
    EXPECT_TRUE(vehicleService.getState().isSpeedWarning());
}

TEST_F(VehicleServiceTest, EngineErrorForcesDecelerationAbove60) {
    vehicleService.changeGear(VehicleState::Gear::D);
    for (int i = 0; i < 10; i++) {
        vehicleService.accelerate();
    }
    VehicleState& result = vehicleService.setEngineError(true);
    EXPECT_EQ(60, result.getSpeed());
    EXPECT_TRUE(result.isEngineWarning());
}

TEST_F(VehicleServiceTest, EngineErrorNoDecelerationBelow60) {
    vehicleService.changeGear(VehicleState::Gear::D);
    for (int i = 0; i < 5; i++) {
        vehicleService.accelerate();
    }
    VehicleState& result = vehicleService.setEngineError(true);
    EXPECT_EQ(50, result.getSpeed());
}

TEST_F(VehicleServiceTest, SeatbeltFastenedClearsWarning) {
    EXPECT_TRUE(vehicleService.getState().isSeatbeltWarning());
    VehicleState& result = vehicleService.setSeatbelt(true);
    EXPECT_FALSE(result.isSeatbeltWarning());
}

// ===== Deceleration calculation =====

TEST_F(VehicleServiceTest, FullBrakeGivesDeceleration10) {
    EXPECT_EQ(10, VehicleService::calculateDeceleration(50, 1.0));
}

TEST_F(VehicleServiceTest, HalfBrakeGivesDeceleration5) {
    EXPECT_EQ(5, VehicleService::calculateDeceleration(50, 0.5));
}

TEST_F(VehicleServiceTest, HighSpeedReducesBrakeEfficiency) {
    int normal = VehicleService::calculateDeceleration(50, 1.0);
    int highSpeed = VehicleService::calculateDeceleration(120, 1.0);
    EXPECT_LT(highSpeed, normal);
}

TEST_F(VehicleServiceTest, InvalidBrakeForceThrows) {
    EXPECT_THROW(VehicleService::calculateDeceleration(50, 1.5), std::invalid_argument);
    EXPECT_THROW(VehicleService::calculateDeceleration(50, -0.1), std::invalid_argument);
}

TEST_F(VehicleServiceTest, NegativeSpeedThrows) {
    EXPECT_THROW(VehicleService::calculateDeceleration(-10, 0.5), std::invalid_argument);
}
