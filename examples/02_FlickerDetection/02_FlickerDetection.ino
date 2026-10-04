#include <AS7343_7Semi.h>

AS7343_7Semi sensor;

constexpr uint32_t FLICKER_WAIT_TIMEOUT_MS = 1500;

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

	// Flicker detection uses the dedicated FD block, not the spectral channel
	// conversions. Auto-SMUX, spectral integration time and spectral gain are
	// therefore not required for this example.
	if (!sensor.enableFlickerDetection()) {
		Serial.println("Failed to enable flicker detection");
		while (true) {
			delay(1000);
		}
	}

	Serial.println("Flicker detection enabled");
}

void loop() {
	if (!sensor.sensorPresent()) {
		Serial.println("SENSOR DISCONNECTED");
		delay(1000);
		return;
	}

	const uint32_t startTime = millis();
	while (!sensor.flickerDetectionValid() &&
	       static_cast<uint32_t>(millis() - startTime) < FLICKER_WAIT_TIMEOUT_MS) {
		delay(10);
	}

	if (!sensor.flickerDetectionValid()) {
		Serial.println("Flicker measurement timeout");
		delay(500);
		return;
	}

	if (sensor.flickerDetectionSaturated()) {
		Serial.println("Flicker measurement saturated");
	} else {
		const uint16_t frequencyHz = sensor.flickerFrequencyHz();
		if (frequencyHz == 0) {
			Serial.println("No 100 Hz or 120 Hz flicker detected");
		} else {
			Serial.print("Detected flicker: ");
			Serial.print(frequencyHz);
			Serial.println(" Hz");
		}
	}

	delay(500);
}
