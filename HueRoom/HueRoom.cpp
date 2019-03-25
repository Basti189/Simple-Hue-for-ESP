// 
// 
// 

#include "HueRoom.h"

// Add struct with id and name to vector
void HueRoom::add(uint8_t roomID, String roomName) {
	Room room;
	room.id = roomID;
	room.name = roomName;
	rooms.push_back(room);
}

// returns id by name or -1 (not found)
int8_t HueRoom::operator [] (String roomName) {
	for (Room &room : rooms) {
		if (room.name == roomName) {
			return room.id;
		}
	}
	return -1;
}

// Removes all entries
void HueRoom::clear() {
	rooms.clear();
}
