/*
 Name:		Philips_Hue_API.ino
 Created:	17.03.2019 14:29:00
 Author:	Sebastian Wolf
*/


#include "HueBridge/HueBridge.h"

// Network
const char *ssid = "";
const char *password = "";

HueBridge bridge;

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
	delay(500);
	Serial.println("Connecting to WiFi..");
  }
  WiFi.persistent(true);
  Serial.println("Connected to the WiFi network");
  
  bridge.init("Bridge_Control", "ESP8266");
  // bridge.setDebugOutput(true); // For debugging
  
  if (bridge.discoverUPNP()) { // Returned true if bridge is discovered
		if (bridge.requestToken()) { // Returned true if an token exists
			switch (bridge.verify()) { // Check status
				case HueBridge::OK:
					// TODO
					break;
				case HueBridge::CONNECTION_REFUSED:
					// TODO
					break;
				case HueBridge::TOKEN_INVALID:
					// TODO
					break;
			}
		}
	} else {
		// TODO
	}
  
	if(bridge.isVerified()) {
		bridge.getAllLights(); // Optional
		bridge.getAllRooms(); // Optional
  } else {
	 while(true); // Or whatever
  }
}

// Now you can do what you want ^.^
void loop() {
  // TODO
}
  
  
