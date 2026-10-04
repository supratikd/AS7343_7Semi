#include <AS7343_7Semi.h>

AS7343_7Semi sensor;

template <typename T>
void printChannel(const char* label, T value) {
	Serial.print(label);
	Serial.print(value);
}

void printSpectralData() {
	printChannel("F1_405 Violet:", sensor.violet_F1());
	printChannel("\tF2_425 Violet-Blue:", sensor.violetBlue_F2());
	printChannel("\tFZ_450 Blue:", sensor.blue_FZ());
	printChannel("\tF3_475 Blue-Cyan:", sensor.blueCyan_F3());
	printChannel("\tF4_515 Green:", sensor.green_F4());
	printChannel("\tF5_550 Green-Yellow:", sensor.greenYellow_F5());
	printChannel("\tFY_555 Yellow-Green:", sensor.yellowGreen_FY());
	printChannel("\tFXL_600 Orange:", sensor.orange_FXL());
	printChannel("\tF6_640 Red:", sensor.red_F6());
	printChannel("\tF7_690 DeepRed:", sensor.deepRed_F7());
	printChannel("\tF8_745 Red-Near-IR:", sensor.redNearInfrared_F8());
	printChannel("\tNIR_855 Near-IR:", sensor.nearInfrared_NIR());
	Serial.println();
}

void setup() {
	Serial.begin(115200);
	while (!Serial) {
	}

	const bool ok = sensor.begin();
	// Use custom I2C pins on ESP32:
	// ok = sensor.begin(21, 22);
	// Use a specific I2C bus object:
	// ok = sensor.begin(Wire);
	// Use custom pins on a specific I2C bus:
	// ok = sensor.begin(21, 22, Wire);

	if (!ok) {
		Serial.println("AS7343_7Semi begin failed");
		while (true) {
			delay(1000);
		}
	}

	sensor.powerOn();
	
	// Turn on/off led
	// sensor.ledOn();
	// sensor.ledOff();

	// read() uses the 18-slot Auto-SMUX ordering (including the three VIS pairs),
	// so enable 18-channel mode before reading spectral channels.
	sensor.configureAutoSMUX();

	// Use a fixed integration time and gain for repeatable readings. Change gain
	// for the light level: lower gain helps avoid saturation in bright light;
	// higher gain helps in dim light. These settings are optional if the sensor
	// has already been configured elsewhere and you intentionally want to retain
	// that configuration.
	sensor.setIntegration();
	sensor.setGain(AS7343_7Semi::Gain::X32);
}

void loop() {
	if (!sensor.sensorPresent()) {
		Serial.println("SENSOR DISCONNECTED");
		delay(1000);
		return;
	}

	if (sensor.read()) {
		printSpectralData();
	} else {
		Serial.println("FAILED TO READ SPECTRAL DATA");
	}

	delay(200);
}