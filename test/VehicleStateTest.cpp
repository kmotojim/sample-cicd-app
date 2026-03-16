#include <gtest/gtest.h>
#include <climits>
#include "VehicleState.h"

class VehicleStateTest : public ::testing::Test {
protected:
    VehicleState vehicleState;
};

// ===== Initial state =====

TEST_F(VehicleStateTest, InitialSpeedIsZero) {
    EXPECT_EQ(0, vehicleState.getSpeed());
}

TEST_F(VehicleStateTest, InitialGearIsP) {
    EXPECT_EQ(VehicleState::Gear::P, vehicleState.getGear());
}

TEST_F(VehicleStateTest, InitialEngineWarningIsFalse) {
    EXPECT_FALSE(vehicleState.isEngineWarning());
}

TEST_F(VehicleStateTest, InitialSeatbeltWarningIsTrue) {
    EXPECT_TRUE(vehicleState.isSeatbeltWarning());
}

TEST_F(VehicleStateTest, InitialSpeedWarningIsFalse) {
    EXPECT_FALSE(vehicleState.isSpeedWarning());
}

// ===== Speed =====

TEST_F(VehicleStateTest, SetNormalSpeed) {
    vehicleState.setSpeed(50);
    EXPECT_EQ(50, vehicleState.getSpeed());
}

TEST_F(VehicleStateTest, NegativeSpeedClampedToZero) {
    vehicleState.setSpeed(-10);
    EXPECT_EQ(0, vehicleState.getSpeed());
}

TEST_F(VehicleStateTest, SpeedAbove180ClampedTo180) {
    vehicleState.setSpeed(200);
    EXPECT_EQ(180, vehicleState.getSpeed());
}

TEST_F(VehicleStateTest, SpeedAt120NoWarning) {
    vehicleState.setSpeed(120);
    EXPECT_FALSE(vehicleState.isSpeedWarning());
}

TEST_F(VehicleStateTest, SpeedAt121TriggersWarning) {
    vehicleState.setSpeed(121);
    EXPECT_TRUE(vehicleState.isSpeedWarning());
}

TEST_F(VehicleStateTest, SpeedWarningClearedWhenBelow121) {
    vehicleState.setSpeed(130);
    EXPECT_TRUE(vehicleState.isSpeedWarning());
    vehicleState.setSpeed(100);
    EXPECT_FALSE(vehicleState.isSpeedWarning());
}

// ===== Gear =====

TEST_F(VehicleStateTest, SetGearToD) {
    vehicleState.setGear(VehicleState::Gear::D);
    EXPECT_EQ(VehicleState::Gear::D, vehicleState.getGear());
}

TEST_F(VehicleStateTest, SetGearToR) {
    vehicleState.setGear(VehicleState::Gear::R);
    EXPECT_EQ(VehicleState::Gear::R, vehicleState.getGear());
}

// ===== Gear string conversion =====

TEST(GearEnumTest, GearToStringAllValues) {
    EXPECT_EQ("P", VehicleState::gearToString(VehicleState::Gear::P));
    EXPECT_EQ("R", VehicleState::gearToString(VehicleState::Gear::R));
    EXPECT_EQ("N", VehicleState::gearToString(VehicleState::Gear::N));
    EXPECT_EQ("D", VehicleState::gearToString(VehicleState::Gear::D));
}

TEST(GearEnumTest, InvalidGearReturnsUnknown) {
    auto invalidGear = static_cast<VehicleState::Gear>(99);
    EXPECT_EQ("UNKNOWN", VehicleState::gearToString(invalidGear));
}

TEST(GearEnumTest, StringToGearAllValues) {
    EXPECT_EQ(VehicleState::Gear::P, VehicleState::stringToGear("P"));
    EXPECT_EQ(VehicleState::Gear::R, VehicleState::stringToGear("R"));
    EXPECT_EQ(VehicleState::Gear::N, VehicleState::stringToGear("N"));
    EXPECT_EQ(VehicleState::Gear::D, VehicleState::stringToGear("D"));
}

TEST(GearEnumTest, InvalidStringThrows) {
    EXPECT_THROW(VehicleState::stringToGear("X"), std::invalid_argument);
}

TEST(GearEnumTest, EmptyStringThrows) {
    EXPECT_THROW(VehicleState::stringToGear(""), std::invalid_argument);
}

TEST(GearEnumTest, LowercaseStringThrows) {
    EXPECT_THROW(VehicleState::stringToGear("d"), std::invalid_argument);
}

// ===== Boundary =====

TEST_F(VehicleStateTest, IntMaxClampedTo180) {
    vehicleState.setSpeed(INT_MAX);
    EXPECT_EQ(180, vehicleState.getSpeed());
}

TEST_F(VehicleStateTest, IntMinClampedToZero) {
    vehicleState.setSpeed(INT_MIN);
    EXPECT_EQ(0, vehicleState.getSpeed());
}
