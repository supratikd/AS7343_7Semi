#include <AS7343_7Semi.h>

AS7343_7Semi sensor;

void setup() {
	Serial.begin(115200);
	while (!Serial) {
	}

	if (!sensor.begin()) {
		Serial.println("AS7343 initialization failed");
		while (true) {
			delay(1000);
		}
	}

	sensor.powerOn();

	// VIS is read from the six VIS slots in 18-channel Auto-SMUX mode. Without
	// this mode the raw slots won't follow the mapping used by getVIS().
	sensor.configureAutoSMUX();

	// Fix integration time and gain to make readings comparable between runs.
	// These are optional if you have already configured the sensor and want to
	// keep that existing setup; choose lower gain for bright conditions.
	sensor.setIntegration();
	sensor.setGain(AS7343_7Semi::Gain::X1);
	Serial.println("VIS measurement ready");
}

void loop() {
	if (!sensor.sensorPresent()) {
		Serial.println("SENSOR DISCONNECTED");
		delay(1000);
		return;
	}

	if (sensor.read()) {
		Serial.print("VIS (average of six Auto-SMUX VIS readings): ");
		Serial.println(sensor.getVIS());
	} else {
		Serial.println("VIS measurement failed");
	}

	delay(200);
}
