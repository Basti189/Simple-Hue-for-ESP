// HueLights

#ifndef _HUELIGHT_h
#define _HUELIGHT_h

#include <Arduino.h>
#include <vector>

class HueLight {
private:
	// Store id and name
	struct Light {
		uint8_t id;
		String name;
	};

	// "List" with structs of Light
	std::vector <Light> lights;

public:
	const static bool ON = true;
	const static bool OFF = false;
	const static uint8_t ALERT_NONE = 0;
	const static uint8_t ALERT_SELECT = 1;
	const static uint8_t ALERT_LSELECT = 2;
	const static uint8_t EFFECT_NONE = 3;
	const static uint8_t EFFECT_COLORLOOP = 4;
	void add(uint8_t lightID, String lightName);
	int8_t operator [] (String lightName);
	void clear();

	void getRGBtoXY(double &x, double&y, uint8_t &r, uint8_t &g, uint8_t &b);
};

#endif
