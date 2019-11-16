// Only modify this file to include
// - function definitions (prototypes)
// - include files
// - extern variable definitions
// In the appropriate section

#ifndef _tomatowater_H_
#define _tomatowater_H_

#include "Arduino.h"

struct Plant {
	const uint8_t moisture_pin;
	const uint8_t pump_pin;
	const String name;
	const uint8_t min_moisture_percent;
	const uint8_t max_moisture_percent;
	const uint8_t pump_time_seconds;
	const uint8_t wait_time_seconds;
	const uint8_t moisture_low;
	const uint8_t moisture_high;
	int moisture_level;
	long lastCheck;
	long lastWater;
	int sensorSwitchVal;
};

struct Environment {
	int temperatureCelsius = 0;
	int airHumidity = -1;
};


void waterPlant(int pump_trigger_pin, int waterTimeSeconds);
void waterPlant(Plant plant);
void waitForWater(int seconds);
int getMoistureLevel(Plant &plant);
int getMoisturePercentage(Plant &plant);
String getColorFromMoisture(const int moisturePercentage);
String getColorFromTemperature(const int temperatureCelsius);
Environment getEnvoirementData();
String getPageHeader();
String getPageFooter();
void handleRoot();
void handlePump();
void handleEnv();


#endif /* _tomatowater_H_ */
