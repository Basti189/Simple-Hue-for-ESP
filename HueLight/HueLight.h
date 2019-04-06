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

	const static uint16_t CT_LIGHT_BULB = 370;
	const static uint16_t CT_HALOGEN_LAMP = 333;
	const static uint16_t CT_FLUORESCENT_LAMP = 250;
	const static uint16_t CT_DAYLIGHT = 182;
	const static uint16_t CT_COOL_WHITE = 153;

	void add(uint8_t lightID, String lightName);
	int8_t operator [] (String lightName);
	void clear();

	void getRGBtoXY(double &x, double&y, uint8_t &r, uint8_t &g, uint8_t &b);
};

#endif
