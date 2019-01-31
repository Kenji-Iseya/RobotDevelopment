﻿#include "RobotController.hpp"

void RobotController::initMotorPWM(int motor1_PWM, int motor2_PWM)
{
	motor1.setPWM(motor1_PWM);
	motor2.setPWM(motor2_PWM);
	isSetMotorPWM = true;
}

void RobotController::initMotorGPIO(int motor1_IN1, int motor1_IN2, int motor2_IN1, int motor2_IN2)
{
	motor1.setGPIO(motor1_IN1, motor1_IN2);
	motor2.setGPIO(motor2_IN1, motor2_IN2);
	isSetMotorGPIO = true;
}

void RobotController::initSensorGPIO(int east, int west, int south, int north)
{
	sensorEast.setGPIO(east);
	sensorWest.setGPIO(west);
	sensorSouth.setGPIO(south);
	sensorNorth.setGPIO(north);
	isSetSensorGPIO = true;

	using namespace BBB;

	//スレッド作成
	std::thread th_checkSensor(&RobotController::correctPosition, this);
	//スレッド実行
	th_checkSensor.join();
}

void RobotController::correctPosition()
{
	Map map("field.txt");

	while (true)
	{
		mSecWait(500);

		for (int mapX = 0; mapX < Map::MAP_SIZE; mapX++) {

			//北側に壁がある場合
			for (int mapY = 0; mapY < Map::MAP_SIZE && map[mapX][mapY].north == 1; mapY++) {

				//北側に壁があるマスの下に、ロボットがいるとき→ロボットの位置を補正
				if ((int)x() / Map::MAP_SIZE == mapX && (int)y() / Map::MAP_SIZE - 1 == mapY) {

					double distance;

					while (true) {
						auto d1 = sensorNorth.distance();
						auto d2 = sensorNorth.distance();
						//2回測った距離の差が、30cmを超えたら、無視
						if (std::abs(d1 - d2) > 30.0) continue;

						distance = (d1 + d2) / 2.0;
						break;
					}

					//位置補正
					if (distance > 20.0) {
						//スレッドセーフにするための処理
						std::lock_guard<std::mutex> lock(mtx);
						_x = (mapX + 0.5) * Map::CELL_SIZE;
						_y = (mapY - 1.0 + 0.5) * Map::CELL_SIZE;
					}

				}

			}
		}

	}//while終わり
}

void RobotController::initPosition(size_t __x, size_t __y)
{
	//スレッドセーフにするための処理
	std::lock_guard<std::mutex> lock(mtx);
	_x = __x;
	_y = __y;

}

void RobotController::mSecWait(const size_t time)
{
	auto start = std::chrono::high_resolution_clock::now();
	while (true)
	{
		auto nowTime = std::chrono::high_resolution_clock::now();
		auto dTime = nowTime - start;
		auto msec = std::chrono::duration_cast<std::chrono::milliseconds>(dTime).count();
		if (msec > time) return;
	}
}

bool RobotController::checkRobotProperties()
{
	if (isSetMotorGPIO && isSetMotorPWM && isSetSensorGPIO && isInitializePosition)
		return true;
	else
		return false;
}

void RobotController::moveEastTime(const size_t mSec)
{
	if (!checkRobotProperties())
		BBB::ErrorBBB("Properties is not set.");

	eastTime += mSec;

	motor1.runReverse();
	motor2.runNormal();

	mSecWait(mSec);

	motor1.stop();
	motor2.stop();
}

void RobotController::moveWestTime(const size_t mSec)
{
	if (!checkRobotProperties())
		BBB::ErrorBBB("Properties is not set.");

	westTime += mSec;

	motor1.runNormal();
	motor2.runReverse();

	mSecWait(mSec);

	motor1.stop();
	motor2.stop();
}

void RobotController::moveSouthTime(const size_t mSec)
{
	if (!checkRobotProperties())
		BBB::ErrorBBB("Properties is not set.");

	southTime += mSec;

	motor1.runReverse();
	motor2.runReverse();

	mSecWait(mSec);

	motor1.stop();
	motor2.stop();
}

void RobotController::moveNorthTime(const size_t mSec)
{
	if (!checkRobotProperties())
		BBB::ErrorBBB("Properties is not set.");

	northTime += mSec;

	motor1.runNormal();
	motor2.runNormal();

	mSecWait(mSec);

	motor1.stop();
	motor2.stop();
}

void RobotController::moveEast(const double distance)
{
	auto aveDutyRate = 0.50*(motor1.duty() + motor2.duty());
	//距離÷速度 で時間を出して、その時間分だけ動かす
	moveEastTime(distance / (DUTY_TO_DISTANCE*aveDutyRate));
}

void RobotController::moveWest(const double distance)
{
	auto aveDutyRate = 0.50*(motor1.duty() + motor2.duty());
	moveWestTime(distance / (DUTY_TO_DISTANCE*aveDutyRate));
}

void RobotController::moveSouth(const double distance)
{
	auto aveDutyRate = 0.50*(motor1.duty() + motor2.duty());
	moveSouthTime(distance / (DUTY_TO_DISTANCE*aveDutyRate));
}

void RobotController::moveNorth(const double distance)
{
	auto aveDutyRate = 0.50*(motor1.duty() + motor2.duty());
	moveNorthTime(distance / (DUTY_TO_DISTANCE*aveDutyRate));
}

void RobotController::setDutyRate(double bothMotorRate)
{
	if (!checkRobotProperties())
		BBB::ErrorBBB("Properties is not set.");

	motor1.setDutyRate(bothMotorRate);
	motor2.setDutyRate(bothMotorRate);
}

void RobotController::setDutyRate(double motor1Rate, double motor2Rate)
{
	if (!checkRobotProperties())
		BBB::ErrorBBB("Properties is not set.");

	motor1.setDutyRate(motor1Rate);
	motor2.setDutyRate(motor2Rate);
}

size_t RobotController::x()
{
	auto aveDutyRate = 0.5*(motor1.dutyRate() + motor2.dutyRate());
	// 下記の数字はduty を 速度[cm/ms] に変換する係数
	return DUTY_TO_DISTANCE * aveDutyRate*(eastTime - westTime);
}

size_t RobotController::y()
{
	auto aveDutyRate = 0.5*(motor1.dutyRate() + motor2.dutyRate());
	// 下記の数字はduty を 速度[cm/ms] に変換する係数
	return DUTY_TO_DISTANCE*aveDutyRate * (northTime - southTime);
}
