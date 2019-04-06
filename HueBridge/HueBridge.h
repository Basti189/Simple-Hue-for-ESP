// HueBridge.h

#ifndef _HUEBRIDGE_h
#define _HUEBRIDGE_h

#ifdef ARDUINO_NodeMCU_32S
#include "WiFiClientSecure.h"
#include <WiFiClient.h>
#include <HttpClient.h>
#include "SPIFFS.h"
#endif

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#endif

#include "FS.h"
#include <WiFiUdp.h>
#include "HueLight/HueLight.h"
#include "HueRoom/HueRoom.h"

class HueBridge {

 private:
 	// For debug messages
	bool _debug = false;

	// Discover
	const String DISCOVERY_HOST = "www.meethue.com";
	const String DISCOVERY_API = "/api/nupnp";
	const String SSDP_HOST = "239.255.255.250";
	const unsigned long SSDP_TIMEOUT = 60000;
	const uint16_t SSDP_PORT = 1900;
	uint16_t HTTPS_PORT = 443;
	String _application_name;
	String _devicename;

	// SPIFFS
	const String ROOT_PATH = "/huebridge/";
	String getFileContent(String filename);
	bool setFileContent(String filename, String content);
	bool deleteFile(String filename);

	// Connection
	String _ip = "";
	String _id = "";
	String _token = "";
	bool _verify = false;

	void println(const String &s);
	int GET(String call, String &content);
	int PUT(String call, String payload, String &content);

	// Helper
	String getValueFromObject(String name, String content);

 public:

	// For method verify
	const static uint8_t OK = 0;
	const static uint8_t TOKEN_INVALID = 1;
	const static uint8_t CONNECTION_REFUSED = 2;

	// Init
	HueBridge();
	~HueBridge();
	void init(String application_name, String devicename);
	void setDebugOutput(bool debug);

	// Discover & Token
	void setAddress(String ip);
	void setAddressandToken(String ip, String token);
	bool discoverNUPNP();
	bool discoverUPNP();
	bool requestToken();
	uint8_t verify();
	bool isVerified();

	// Hue management
	HueLight light;
	uint8_t getAllLights();

	HueRoom room;
	uint8_t getAllRooms();

	// Hue Lights
	bool setLightState(uint8_t lightID, bool on);
	bool getLightState(uint8_t lightID);
	
	bool setLightBrightness(uint8_t lightID, uint8_t brightness);
	uint8_t getLightBrightness(uint8_t lightID);

	bool setLightColorTemperature(uint8_t lightID, uint16_t colorTemperature);
	uint16_t getLightColorTemperature(uint8_t lightID);
	uint16_t getLightColorTemperature(uint8_t lightID, bool convertToKelvin);

	bool setLightAlert(uint8_t lightID, uint8_t alert);

	bool isLightReachable(uint8_t lightID);

	// Color Lights only
	bool setLightSaturation(uint8_t lightID, uint8_t saturation);
	uint8_t getLightSaturation(uint8_t lightID);

	bool setLightHue(uint8_t lightID, uint16_t hue);
	uint16_t getLightHue(uint8_t lightID);

	bool setLightColor(uint8_t lightID, double x, double y);
	bool setLightColor(uint8_t lightID, uint8_t r, uint8_t g, uint8_t b);

	bool setLightEffect(uint8_t lightID, uint8_t effect);

	// Mixed methods
	bool setLightBrightnessAndColorTemperature(uint8_t lightID, uint8_t brightness, uint8_t colorTemperature);

	// Hue Rooms
	bool setRoomState(uint8_t roomID, bool on);
	bool getRoomState(uint8_t roomID, uint8_t state);

	bool setRoomBrightness(uint8_t roomID, uint8_t brightness);
	uint8_t getRoomBrightness(uint8_t roomID);

	bool setRoomColorTemperature(uint8_t roomID, uint16_t colorTemperature);
	uint16_t getRoomColorTemperature(uint8_t roomID);
	uint16_t getRoomColorTemperature(uint8_t roomID, bool convertToKelvin);

	bool setRoomAlert(uint8_t roomID, uint8_t alert);

	// Color Lights only
	bool setRoomSaturation(uint8_t roomID, uint8_t saturation);
	uint8_t getRoomSaturation(uint8_t roomID);

	bool setRoomHue(uint8_t roomID, uint16_t hue);
	uint16_t getRoomHue(uint8_t roomID);

	bool setRoomColor(uint8_t roomID, double x, double y);
	bool setRoomColor(uint8_t roomID, uint8_t r, uint8_t g, uint8_t b);

	bool setRoomEffect(uint8_t roomID, uint8_t effect);

	// Mixed methods
	bool setRoomBrightnessAndColorTemperature(uint8_t rooID, uint8_t brightness, uint8_t colorTemperature);
};

#endif
