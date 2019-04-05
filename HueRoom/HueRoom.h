// HueRoom.h

#ifndef _HUEROOM_h
#define _HUEROOM_h

#include <Arduino.h>
#include <vector>

class HueRoom {
private:
	// Store id and name
	struct Room {
		uint8_t id;
		String name;
	};

	// "List" with structs of Room
	std::vector <Room> rooms;

public:
	const static bool ON = true;
	const static bool OFF = false;
	const static uint8_t STATE_ALL = 0;
	const static uint8_t STATE_ANY = 1;
	const static uint8_t ALERT_NONE = 0;
	const static uint8_t ALERT_SELECT = 1;
	const static uint8_t ALERT_LSELECT = 2;
	void add(uint8_t roomID, String roomName);
	int8_t operator [] (String roomName);
	void clear();
};

#endif
