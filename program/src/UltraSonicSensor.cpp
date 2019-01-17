﻿#include "UltraSonicSensor.hpp"
using namespace BBB;


BBB::UltraSonicSensor::UltraSonicSensor(int gpioNum_) : BBB::GPIO(gpioNum_)
{
	this->setDirection(IN);
	this->setEdge(true);
}

//距離[mm]を返す
double BBB::UltraSonicSensor::distance()
{
	if (!isGPIOSetted) throw BBB::ErrorBBB("GPIO num has NOT been setted.");

	std::stringstream path;
	path << "/sys/class/gpio/gpio" << gpioNum << "/value";

	auto valueFd = open(path.str().c_str(), O_RDONLY);

	pollfd pfd;
	pfd.fd = valueFd;
	pfd.events = POLLPRI;

	auto edgeUpTime = std::chrono::high_resolution_clock::now();
	auto nowTime = std::chrono::high_resolution_clock::now();

	char c;
	while (true)
	{
		lseek(valueFd, 0, SEEK_SET);
		auto ret = poll(&pfd, 1, -1);
		read(valueFd, &c, 1);
		//パルスの立ち上がり時刻を取得
		if (c == '1') edgeUpTime = std::chrono::high_resolution_clock::now();
		else continue;

		lseek(valueFd, 0, SEEK_SET);
		ret = poll(&pfd, 1, -1);
		read(valueFd, &c, 1);
		//パルスの立ち下がり時刻を所得
		if (c == '0') nowTime = std::chrono::high_resolution_clock::now();
		else continue;

		//立ち下がり - 立ち上がり の時間計算
		auto dtime = nowTime - edgeUpTime;
		// 2*距離d÷時間t = 音速V → d[mm] = 0.5*V*t = 0.1718[mm/μs]*t[μs]
		return 0.1717975*std::chrono::duration_cast<std::chrono::microseconds>(dtime).count();
	}


}
