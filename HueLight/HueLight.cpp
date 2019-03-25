// 
// 
// 

#include "HueLight.h"

// Add struct with id and name to vector
void HueLight::add(uint8_t lightID, String lightName) {
	Light light;
	light.id = lightID;
	light.name = lightName;
	lights.push_back(light);
}

// returns id by name or -1 (not found)
int8_t HueLight::operator [] (String lightName) {
	for (Light &light : lights) {
		if (light.name == lightName) {
			return light.id;
		}
	}
	return -1;
}

// Removes all entries
void HueLight::clear() {
	lights.clear();
}

// Convert any RGB color to a Philips Hue XY values
// The Code is based on:
// https://stackoverflow.com/questions/22564187/rgb-to-philips-hue-hsb
void HueLight::getRGBtoXY(double &x, double&y, uint8_t &r, uint8_t &g, uint8_t &b) {
	 // For the hue bulb the corners of the triangle are:
    // -Red: 0.675, 0.322
    // -Green: 0.4091, 0.518
    // -Blue: 0.167, 0.04
	double *normalizedToOne = new double[3];
	normalizedToOne[0] = ((double) r / 255);
	normalizedToOne[1] = ((double) g / 255);
	normalizedToOne[2] = ((double) b / 255);

	double red, green, blue;

	// Make red more vivid
	if (normalizedToOne[0] > 0.04045) {
		red = pow((normalizedToOne[0] + 0.055) / (1.0 + 0.055), 2.4);
	} else {
		red = (normalizedToOne[0] / 12.92);
	}

	// Make green more vivid
    if (normalizedToOne[1] > 0.04045) {
        green = pow((normalizedToOne[1] + 0.055) / (1.0 + 0.055), 2.4);
    } else {
        green = (normalizedToOne[1] / 12.92);
    }

	// Make blue more vivid
    if (normalizedToOne[2] > 0.04045) {
        blue = pow((normalizedToOne[2] + 0.055) / (1.0 + 0.055), 2.4);
    } else {
        blue = (normalizedToOne[2] / 12.92);
    }

	double X = (red * 0.649926 + green * 0.103455 + blue * 0.197109);
	double Y = (red * 0.234327 + green * 0.743075 + blue * 0.022598);
	double Z = (red * 0.0000000 + green * 0.053077 + blue * 1.035763);

	x = X / (X + Y + Z);
    y = Y / (X + Y + Z);

	delete[] normalizedToOne;
}
