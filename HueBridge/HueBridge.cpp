// 
// 
// 

#include "HueBridge.h"

// ****************************** [ Initialization ] ****************************** //

HueBridge::HueBridge() {
	
}

HueBridge::~HueBridge() {
	
}

void HueBridge::init(String application_name, String devicename) {
#ifdef ARDUINO_NodeMCU_32S
	if (!SPIFFS.begin(true)) {
		println("An Error has occurred while mounting SPIFFS");
	}
#endif
#ifdef ESP8266
	SPIFFS.begin();
#endif
	_application_name = application_name;
	_devicename = devicename;
}

void HueBridge::setDebugOutput(bool debug) {
	_debug = debug;
}

void HueBridge::println(const String &s) {
	if (_debug) {
		Serial.println("<HueBridge> " + s);
	}
}


// ****************************** [ Discover & Token management] ****************************** //

void HueBridge::setAddress(String ip) {
	_ip = ip;
}
void HueBridge::setAddressandToken(String ip, String token) {
	_ip = ip;
	_token = token;
}

bool HueBridge::discoverNUPNP() {
	if (!WiFi.isConnected()) {
		println("No network connection!");
		return false;
	}

	WiFiClientSecure client;
	if (client.connect(DISCOVERY_HOST.c_str(), HTTPS_PORT)) { // Doesn't work on ESP8266 => https://github.com/esp8266/Arduino/issues/4826
		client.println("GET " + DISCOVERY_API + " HTTP/1.1");
		client.println("Host: " + String(DISCOVERY_HOST));
		client.println("Accept: */*");
		client.println();

		String line = "";
		while (client.connected()) {
			line = client.readStringUntil('\n');
			if (line.indexOf("id") != -1 && line.indexOf("internalipaddress") != -1) {
				// ID
				_id = line.substring(0, line.indexOf(","));
				_id.replace("[{\"id\":\"", "");
				_id.replace("\"", "");
				println("Bridge ID: " + _id);

				//IP
				_ip = line.substring(line.indexOf(",") + 1, line.length());
				_ip.replace("\"internalipaddress\":\"", "");
				_ip.replace("\"}]", "");
				println("Bridge IP: " + _ip);

				client.stop();
				return true;
			}
		}
		client.stop();
	} else {
		println("Can't connect to \"" + String(DISCOVERY_HOST) + "\"");
		println("Looking for last known address");
		String lastKnownAddress = getFileContent("lastKnownAddress");
		if (lastKnownAddress != "") {
			println("Last known address => " + lastKnownAddress);
			_ip = lastKnownAddress;
			return true;
		}
		println("No last known address!");
	}
	println("No Bridge discovered!");
	return false;
}

bool HueBridge::discoverUPNP() {
	println("Looking for last known address");
	String lastKnownAddress = getFileContent("lastKnownAddress");
	if (lastKnownAddress != "") {
		println("Last known address => " + lastKnownAddress);
		_ip = lastKnownAddress;
		return true;
	}
	println("No last known address!");

	println("[SSDP] Wait for HueBridge (max. 1 min)");
	WiFiUDP udp;
	IPAddress address;
	address.fromString(SSDP_HOST);
	udp.beginMulticast(WiFi.localIP(), address, SSDP_PORT);

	long start = millis();
	while (millis() - start < SSDP_TIMEOUT) {
		int packetSize = udp.parsePacket();
		if (packetSize) {
			String inPacket = udp.readString();
			if (inPacket.indexOf("hue-bridgeid:") != -1) {
				_ip = inPacket.substring(inPacket.indexOf("LOCATION: http://") + 17, inPacket.indexOf(":80/description.xml"));
				println("[SSDP] HueBridge IP => " + _ip);
				udp.stop();
				return true;
			}
		}
	}
	println("[SSDP] Timeout");
	return false;
}

bool HueBridge::requestToken() {
	println("Looking for last known token");
	String lastToken = getFileContent("lastToken");
	if (lastToken != "") {
		println("Token => " + lastToken);
		_token = lastToken;
		return true;
	} else {
		println("No token found");
		println("Request token");
		if (_ip == "") {
			println("Can't request token. Where is the Bridge? <IP>");
			return false;
		}
		String devicetypeBody = "{\"devicetype\":\"" + _application_name + "#" + _devicename + "\"}";
		WiFiClient wifi_client;
		HTTPClient client;
		for (int i = 0; i < 3; i++) {
			delay(10000);		
			if (client.begin(wifi_client, _ip, 80, "/api")) {
				client.addHeader("Host", _ip);
				client.addHeader("Accept", "*/*");
				client.addHeader("Content-Length", String(devicetypeBody.length()));
				client.addHeader("Content-Type", "application/json;charset=UTF-8");
				int httpCode = client.POST(devicetypeBody);
				if (httpCode > 0) {
					String answer = client.getString();
					if (answer.indexOf("\"success\":{\"username\"") != -1) {
						_token = answer;
						_token.replace("[{\"success\":{\"username\":\"", "");
						_token.replace("\"}}]", "");
						println("Token => " + _token);
						setFileContent("lastToken", _token);
						return true;
					} else if (answer.indexOf("link button not pressed") != -1) {
						println("Error => Link button not pressed");
					}
				}
			}
		}
	}
	return false;
}

uint8_t HueBridge::verify() {
	println("Verify connection and token");
	String content;
	if (GET("state", content)) {
		if (content.indexOf("unauthorized user") != -1) {
			println("Unauthorized user!");
			_token = "";
			if (deleteFile("lastToken")) {
				println("Token deleted");
			}
			println("Please request a new token...");
			return TOKEN_INVALID;
		}
		println("Authorized user");
		setFileContent("lastKnownAddress", _ip);
		_verify = true;
		return OK;
	}
	println("Connection refused");
	_ip = "";
	if (deleteFile("lastKnownAddress")) {
		println("Address deleted");
	}
	return CONNECTION_REFUSED;
}

bool HueBridge::isVerified() {
	return _verify;
}

// ****************************** [ Hue name management ] ****************************** //

// Get all HueLights
// Looking for each HueLight individually - takes longer, but can be processed 
uint8_t HueBridge::getAllLights() {
	println("Search for HueLights");
	light.clear();
	uint8_t count = 0;
	for (uint8_t lightID = 1 ; lightID <= 50 ; lightID++) {
		String content;
		if (GET("lights/" + String(lightID), content)) {
			if (content.indexOf("error") != -1) {
				continue;
			}
			light.add(lightID, getValueFromObject("name", content));
			count++;
		}
	}
	println(String(count) + " HueLights assigned");
	return count;
}

// Get all HueRooms
// Looking for each HueRoom individually - takes longer, but can be processed 
uint8_t HueBridge::getAllRooms() {
	println("Search for HueRooms");
	room.clear();
	uint8_t count = 0;
	for (uint8_t roomID = 1 ; roomID <= 50 ; roomID++) {
		String content;
		if (GET("groups/" + String(roomID), content)) {
			if (content.indexOf("error") != -1) {
				continue;
			}
			if (getValueFromObject("type", content) == "Room") {
				room.add(roomID, getValueFromObject("name", content));
				count++;
			}
		}
	}
	println(String(count) + " HueRooms assigned");
	return count;
}

// ****************************** [ HueLight Control ] ****************************** //

// All Lights
// Turn Light on or off
bool HueBridge::setLightState(uint8_t lightID, bool on) {
	String content;
	String payload = "{\"on\":";
	if (on) {
		payload += "true}";
	} else {
		payload += "false}";
	}
	if (PUT("lights/" + String(lightID) + "/state", payload, content)) {
		if (content.indexOf("success") != -1) {
			return true;
		}
	}
	return false;
}

// Returns light status (on/off)
bool HueBridge::getLightState(uint8_t lightID) {
	String content;
	if (GET("lights/" + String(lightID), content)) {
		String result = getValueFromObject("on", content);
		if (result == "true") {
			return true;
		}
	}
	return false;
}

// Turns light on and set brightness or turns light off
bool HueBridge::setLightBrightness(uint8_t lightID, uint8_t brightness) {
	String content;
	String payload = "{\"on\":";
	if (brightness > 0) {
		payload += "true,";
		payload += "\"bri\":" + String(brightness) + "}";
	} else {
		payload += "false}";
	}
	println("Payload => " + payload);
	if (PUT("lights/" + String(lightID) + "/state", payload, content)) {
		if (content.indexOf("success") != -1) {
			return true;
		}
	}
	return false;
}

// Returns brightness
uint8_t HueBridge::getLightBrightness(uint8_t lightID) {
	String content;
	if (GET("lights/" + String(lightID), content)) {
		String result = getValueFromObject("bri", content);
		return result.toInt();
	}
	return 0;
}

// Turns light on and set colortemperature (kelvin or mired)
bool HueBridge::setLightColorTemperature(uint8_t lightID, uint16_t colorTemperature) {
	if (colorTemperature <= 6500 && colorTemperature >= 2000) { 		// valid kelvin
		colorTemperature = 1000000 / colorTemperature; 					// convert kelvin to mired
	} else if (colorTemperature <= 500 && colorTemperature >= 153) { 	// valid mired
		// nothing to do
	} else {															// invalid
		return false;
	}

	String content;
	String payload = "{\"on\":true,";
	payload += "\"ct\":" + String(colorTemperature) + "}";
	println("Payload => " + payload);
	if (PUT("lights/" + String(lightID) + "/state", payload, content)) {
		if (content.indexOf("success") != -1) {
			return true;
		}
	}
	return false;
}

// Returns colortemperature in mired by default
uint16_t HueBridge::getLightColorTemperature(uint8_t lightID) {
	return getLightColorTemperature(lightID, false);
}

// Returns colortemperature in mired by default or convert to kelvin
uint16_t HueBridge::getLightColorTemperature(uint8_t lightID, bool convertToKelvin) {
	String content;
	if (GET("lights/" + String(lightID), content)) {
		String result = getValueFromObject("ct", content);
		if (convertToKelvin) {
			uint16_t colorTemperature = 1000000 / result.toInt();
			if (colorTemperature < 2000) {
				colorTemperature = 2000;
			} else if (colorTemperature > 6500) {
				colorTemperature = 6500;
			}
			return colorTemperature;
		} else { // default
			return result.toInt();
		}
	}
	return 0;
}

// Set alert mode
bool HueBridge::setLightAlert(uint8_t lightID, uint8_t alert) {
	String content;
	String payload = "{\"alert\":";
	switch(alert) {
		case HueLight::ALERT_NONE:
			payload += "\"none\"}";
			break;
		case HueLight::ALERT_SELECT:
			payload += "\"select\"}";
			break;
		case HueLight::ALERT_LSELECT:
			payload += "\"lselect\"}";
			break;
	}
	if (PUT("lights/" + String(lightID) + "/state", payload, content)) {
		if (content.indexOf("success") != -1) {
			return true;
		}
	}
	return false;
}

// Returns if HueLight is reachable
bool HueBridge::isLightReachable(uint8_t lightID) {
	String content;
	if (GET("lights/" + String(lightID), content)) {
		String result = getValueFromObject("reachable", content);
		Serial.println("Result: " + result);
		if (result == "true") {
			return true;
		}
	}
	return false;
}

// Only Color Lights
// Turns light on and set saturation
bool HueBridge::setLightSaturation(uint8_t lightID, uint8_t saturation) {
	String content;
	String payload = "{\"on\":true,";
	payload += "\"sat\":" + String(saturation) + "}";
	println("Payload => " + payload);
	if (PUT("lights/" + String(lightID) + "/state", payload, content)) {
		if (content.indexOf("success") != -1) {
			return true;
		}
	}
	return false;
}

// Returns saturation
uint8_t HueBridge::getLightSaturation(uint8_t lightID) {
	String content;
	if (GET("lights/" + String(lightID), content)) {
		String result = getValueFromObject("sat", content);
		return result.toInt();
	}
	return 0;
}

// Turns light on and set hue
bool HueBridge::setLightHue(uint8_t lightID, uint16_t hue) {
	String content;
	String payload = "{\"on\":true,";
	payload += "\"hue\":" + String(hue) + "}";
	println("Payload => " + payload);
	if (PUT("lights/" + String(lightID) + "/state", payload, content)) {
		if (content.indexOf("success") != -1) {
			return true;
		}
	}
	return false;
}

// Return hue
uint16_t HueBridge::getLightHue(uint8_t lightID) {
	String content;
	if (GET("lights/" + String(lightID), content)) {
		String result = getValueFromObject("hue", content);
		return result.toInt();
	}
	return 0;
}

// Turns light on and set XY color
bool HueBridge::setLightColor(uint8_t lightID, double x, double y) {
	String content;
	String payload = "{\"on\":true,";
	payload += "\"xy\":[" + String(x, 4) + "," + String(y, 4) + "]}";
	println("Payload => " + payload);
	if (PUT("lights/" + String(lightID) + "/state", payload, content)) {
		if (content.indexOf("success") != -1) {
			return true;
		}
	}
	return false;
}

// Turns light on and set XY color from rgb
bool HueBridge::setLightColor(uint8_t lightID, uint8_t r, uint8_t g, uint8_t b) {
	double x, y;
	light.convertRGBtoXY(x, y, r, g, b);
	return setLightColor(lightID, x, y);
}

// Turns light on and set effect
bool HueBridge::setLightEffect(uint8_t lightID, uint8_t effect) {
	String content;
	String payload = "{\"on\":true,";
	payload += "\"effect\":";
	switch(effect) {
		case HueLight::EFFECT_NONE:
			payload += "\"none\"}";
			break;
		case HueLight::EFFECT_COLORLOOP:
			payload += "\"colorloop\"}";
			break;
	}
	if (PUT("lights/" + String(lightID) + "/state", payload, content)) {
		if (content.indexOf("success") != -1) {
			return true;
		}
	}
	return false;
}

// ****************************** [ HueRoom Control ] ****************************** //

// Turns Lights in room <roomID> on or off
bool HueBridge::setRoomState(uint8_t roomID, bool on) {
	String content;
	String payload = "{\"on\":";
	if (on) {
		payload += "true}";
	} else {
		payload += "false}";
	}
	if (PUT("groups/" + String(roomID) + "/action", payload, content)) {
		if (content.indexOf("success") != -1) {
			return true;
		}
	}
	return false;
}

// Returns  state of all or any HueLight based on query <state>
bool HueBridge::getRoomState(uint8_t roomID, uint8_t state) {
	String content;
	if (GET("groups/" + String(roomID), content)) {
		String result;
		switch(state) {
			case HueRoom::STATE_ALL:
				result = getValueFromObject("all_on", content);
				break;
			case HueRoom::STATE_ANY:
				result = getValueFromObject("any_on", content);
				break;
		}
		if (result == "true") {
			return true;
		}
	}
	return false;
}

// Turns room on and set brightness or turns room off
bool HueBridge::setRoomBrightness(uint8_t roomID, uint8_t brightness) {
	String content;
	String payload = "{\"on\":";
	if (brightness > 0) {
		payload += "true,";
		payload += "\"bri\":" + String(brightness) + "}";
	} else {
		payload += "false}";
	}
	println("Payload => " + payload);
	if (PUT("groups/" + String(roomID) + "/action", payload, content)) {
		if (content.indexOf("success") != -1) {
			return true;
		}
	}
	return false;
}

// Returns brightness
uint8_t HueBridge::getRoomBrightness(uint8_t roomID) {
	String content;
	if (GET("groups/" + String(roomID), content)) {
		String result = getValueFromObject("bri", content);
		return result.toInt();
	}
	return 0;
}

// Turns room on and set colortemperature (kelvin or mired)
bool HueBridge::setRoomColorTemperature(uint8_t roomID, uint16_t colorTemperature) {
	if (colorTemperature <= 6500 && colorTemperature >= 2000) { 		// valid kelvin
		colorTemperature = 1000000 / colorTemperature; 					// convert kelvin to mired
	} else if (colorTemperature <= 500 && colorTemperature >= 153) { 	// valid mired
		// nothing to do
	} else {															// invalid
		return false;
	}

	String content;
	String payload = "{\"on\":true,";
	payload += "\"ct\":" + String(colorTemperature) + "}";
	println("Payload => " + payload);
	if (PUT("groups/" + String(roomID) + "/action", payload, content)) {
		if (content.indexOf("success") != -1) {
			return true;
		}
	}
	return false;
}

// Returns colortemperature in mired by default
uint16_t HueBridge::getRoomColorTemperature(uint8_t roomID) {
	return getRoomColorTemperature(roomID, false);
}

// Returns colortemperature in mired by default or convert to kelvin
uint16_t HueBridge::getRoomColorTemperature(uint8_t roomID, bool convertToKelvin) {
	String content;
	if (GET("groups/" + String(roomID), content)) {
		String result = getValueFromObject("ct", content);
		if (convertToKelvin) {
			uint16_t colorTemperature = 1000000 / result.toInt();
			if (colorTemperature < 2000) {
				colorTemperature = 2000;
			} else if (colorTemperature > 6500) {
				colorTemperature = 6500;
			}
			return colorTemperature;
		} else { // default
			return result.toInt();
		}
	}
	return 0;
}

// Set alert mode
bool HueBridge::setRoomAlert(uint8_t roomID, uint8_t alert) {
	String content;
	String payload = "{\"alert\":";
	switch(alert) {
		case HueRoom::ALERT_NONE:
			payload += "\"none\"}";
			break;
		case HueRoom::ALERT_SELECT:
			payload += "\"select\"}";
			break;
		case HueRoom::ALERT_LSELECT:
			payload += "\"lselect\"}";
			break;
	}
	if (PUT("groups/" + String(roomID) + "/action", payload, content)) {
		if (content.indexOf("success") != -1) {
			return true;
		}
	}
	return false;
}

// Only Color Lights
// Turns room on and set saturation
bool HueBridge::setRoomSaturation(uint8_t roomID, uint8_t saturation) {
	String content;
	String payload = "{\"on\":true,";
	payload += "\"sat\":" + String(saturation) + "}";
	println("Payload => " + payload);
	if (PUT("groups/" + String(roomID) + "/action", payload, content)) {
		if (content.indexOf("success") != -1) {
			return true;
		}
	}
	return false;
}

// Returns saturation
uint8_t HueBridge::getRoomSaturation(uint8_t roomID) {
	String content;
	if (GET("groups/" + String(roomID), content)) {
		String result = getValueFromObject("sat", content);
		return result.toInt();
	}
	return 0;
}

// Turns room on and set hue
bool HueBridge::setRoomHue(uint8_t roomID, uint16_t hue) {
	String content;
	String payload = "{\"on\":true,";
	payload += "\"hue\":" + String(hue) + "}";
	println("Payload => " + payload);
	if (PUT("groups/" + String(roomID) + "/action", payload, content)) {
		if (content.indexOf("success") != -1) {
			return true;
		}
	}
	return false;
}

// Returns hue
uint16_t HueBridge::getRoomHue(uint8_t roomID) {
	String content;
	if (GET("groups/" + String(roomID), content)) {
		String result = getValueFromObject("hue", content);
		return result.toInt();
	}
	return 0;
}

// Turns room on and set XY color
bool HueBridge::setRoomColor(uint8_t roomID, double x, double y) {
	String content;
	String payload = "{\"on\":true,";
	payload += "\"xy\":[" + String(x, 4) + "," + String(y, 4) + "]}";
	println("Payload => " + payload);
	if (PUT("lights/" + String(roomID) + "/state", payload, content)) {
		if (content.indexOf("success") != -1) {
			return true;
		}
	}
	return false;
}

// Turns room on and set XY color from rgb
bool HueBridge::setRoomColor(uint8_t roomID, uint8_t r, uint8_t g, uint8_t b) {
	double x, y;
	light.convertRGBtoXY(x, y, r, g, b);
	return setRoomColor(roomID, x, y);
}

// Turns room on and set effect
bool HueBridge::setRoomEffect(uint8_t roomID, uint8_t effect) {
	String content;
	String payload = "{\"on\":true,";
	payload += "\"effect\":";
	switch(effect) {
		case HueLight::EFFECT_NONE:
			payload += "\"none\"}";
			break;
		case HueLight::EFFECT_COLORLOOP:
			payload += "\"colorloop\"}";
			break;
	}
	if (PUT("groups/" + String(roomID) + "/action", payload, content)) {
		if (content.indexOf("success") != -1) {
			return true;
		}
	}
	return false;
}

// ****************************** [ DIY JSON Helper ] ****************************** //

String HueBridge::getValueFromObject(String name, String content) {
	name = "\"" + name + "\":";
	if (content.indexOf(name) != -1) {
		content = content.substring(content.indexOf(name) + name.length(), content.length());
		if (content.startsWith("\""))  {
			content = content.substring(0, content.indexOf("\"", 1));
		} else if (content.indexOf(",") != -1) {
			content = content.substring(0, content.indexOf(","));
		} else if (content.indexOf("}") != -1) {
			content = content.substring(0, content.indexOf("}"));
		}
		content.replace("\"", "");
		content.replace("\t", "");
		content.replace("\n", "");
		content.replace("\r", "");
		content.replace("}", "");
		return content;
	}
	return "";
}

// ****************************** [ Connection management ] ****************************** //

int HueBridge::GET(String call, String &content) {
	if (_ip == "" || _token == "") {
		println("Unknown IP or Token!");
		return 0;
	}

	delay(100);

	WiFiClient wifi_client;
	HTTPClient client;
	if (!client.begin(wifi_client, _ip, 80, "/api/" + _token + "/" + call)) {
		println("Can't connect to Bridge!");
		return 0;
	}
	client.addHeader("Host", _ip);
	int httpCode = client.GET();
	if (httpCode > 0) {
		String returnedPayload = client.getString();
		if (returnedPayload.length() > 0) {
			content = returnedPayload;
		} else {
			println("[HTTP-Client][getString] not enough memory to reserve a string!");
		}
		return httpCode;
	}
	return 0;
}

int HueBridge::PUT(String call, String payload, String &content) {
	if (_ip == "" || _token == "") {
		println("Unknown IP or Token!");
		return 0;
	}

	delay(100);

	WiFiClient wifi_client;
	HTTPClient client;
	if (!client.begin(wifi_client, _ip, 80, "/api/" + _token + "/" + call)) {
		println("Can't connect to Bridge!");
		return 0;
	}
	client.addHeader("Host", _ip);
	client.addHeader("Accept", "*/*");
	client.addHeader("Content-Length", String(payload.length()));
	client.addHeader("Content-Type", "application/json;charset=UTF-8");
	int httpCode = client.PUT(payload);
	if (httpCode > 0) {
		String returnedPayload = client.getString();
		if (returnedPayload.length() > 0) {
			content = returnedPayload;
		} else {
			println("[HTTP-Client][getString] not enough memory to reserve a string!");
		}
		return httpCode;
	}
	println("HttpCode => " + String(httpCode));
	return 0;
}

// ****************************** [ File management ] ****************************** //

String HueBridge::getFileContent(String filename) {
	String pathToFile = ROOT_PATH + filename + ".txt";
	if (SPIFFS.exists(pathToFile)) {
		File file = SPIFFS.open(pathToFile, "r");			// Open file for reading
		String content = "";
		while (file.available()) {							// Read while data available
			char c = file.read();
			if (c != '\n') {
				content += c;								// Add char to String while != \n
			}
		}
		content = content.substring(0, content.length() - 1);
		file.close();
		return content;
	}
	return "";
}

bool HueBridge::setFileContent(String filename, String content) {
	String pathToFile = ROOT_PATH + filename + ".txt";
	File file = SPIFFS.open(pathToFile, "w");
	if (!file) {
		println("Can't open \"" + filename + "\"!");
		return false;
	}
	file.println(content);
	file.close();
	return true;
}

bool HueBridge::deleteFile(String filename) {
	String pathToFile = ROOT_PATH + filename + ".txt";
	if (SPIFFS.exists(pathToFile)) {
		return SPIFFS.remove(pathToFile);
	}
	return false;
}
