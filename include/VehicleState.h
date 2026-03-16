#ifndef VEHICLE_STATE_H
#define VEHICLE_STATE_H

#include <string>

/**
 * 車両状態を表すモデルクラス
 */
class VehicleState {
public:
    enum class Gear {
        P,  // パーキング
        R,  // リバース
        N,  // ニュートラル
        D   // ドライブ
    };

    VehicleState();

    int getSpeed() const;
    Gear getGear() const;
    bool isEngineWarning() const;
    bool isSeatbeltWarning() const;
    bool isSpeedWarning() const;

    void setSpeed(int speed);
    void setGear(Gear gear);
    void setEngineWarning(bool warning);
    void setSeatbeltWarning(bool warning);
    void setSpeedWarning(bool warning);

    static std::string gearToString(Gear gear);
    static Gear stringToGear(const std::string& str);

private:
    int speed_;
    Gear gear_;
    bool engineWarning_;
    bool seatbeltWarning_;
    bool speedWarning_;
};

#endif // VEHICLE_STATE_H
