#include <AS7343_7Semi.h>

AS7343_7Semi sensor;

constexpr uint16_t FD_INTEGRATION_TICKS = 200;
constexpr uint16_t SAMPLE_BUFFER_SIZE = 128;
constexpr uint32_t FIFO_CAPTURE_TIMEOUT_MS = 1500;

// Nominal estimate from 1 / (FD_INTEGRATION_TICKS * 2.78 us).
// Replace with a sample rate measured on the target hardware for better Hz accuracy.
constexpr float NOMINAL_SAMPLE_RATE_HZ =
	1.0e6f / (FD_INTEGRATION_TICKS * 2.78f);

uint8_t flickerSamples[SAMPLE_BUFFER_SIZE];

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
	Serial.println("AS7343 flicker FIFO frequency estimate");
	Serial.println("The reported rate is an estimate, not a guaranteed exact frequency.");
}

void loop() {
	if (!sensor.configureFlickerFifo(FD_INTEGRATION_TICKS)) {
		Serial.println("Failed to configure flicker FIFO");
		delay(1000);
		return;
	}

	uint16_t sampleCount = 0;
	const uint32_t startTime = millis();
	while (sampleCount < SAMPLE_BUFFER_SIZE &&
	       static_cast<uint32_t>(millis() - startTime) < FIFO_CAPTURE_TIMEOUT_MS) {
		uint16_t samplesRead = 0;
		if (!sensor.readFlickerFifo(flickerSamples + sampleCount,
		                            SAMPLE_BUFFER_SIZE - sampleCount,
		                            samplesRead)) {
			Serial.println("Failed to read flicker FIFO");
			delay(500);
			return;
		}
		sampleCount += samplesRead;
		if (samplesRead == 0) {
			delay(1);
		}
	}

	bool overflowed = false;
	if (!sensor.flickerFifoOverflowed(overflowed)) {
		Serial.println("Failed to read FIFO overflow status");
		delay(500);
		return;
	}
	if (overflowed) {
		Serial.println("FIFO overflow: samples were lost; discard this estimate");
		delay(500);
		return;
	}

	if (sensor.flickerDetectionSaturated()) {
		Serial.println("Flicker measurement saturated; discard this estimate");
		delay(500);
		return;
	}

	bool classifiedFlicker = false;
	uint16_t classifiedFrequencyHz = 0;
	if (!sensor.getFlickerInfo(classifiedFlicker, classifiedFrequencyHz)) {
		Serial.println("Failed to read built-in flicker classification");
	} else if (classifiedFlicker) {
		Serial.print("Built-in 100/120 Hz classification: ");
		Serial.print(classifiedFrequencyHz);
		Serial.println(" Hz");
	} else {
		Serial.println("No built-in 100/120 Hz classification");
	}

	float estimatedHz = 0.0f;
	if (!AS7343_7Semi::estimateFlickerFrequencyHz(
	        flickerSamples, sampleCount, NOMINAL_SAMPLE_RATE_HZ, estimatedHz)) {
		Serial.println("Not enough clean flicker cycles to estimate frequency");
	} else {
		Serial.print("Estimated flicker frequency: ");
		Serial.print(estimatedHz, 1);
		Serial.println(" Hz");
		Serial.println("Calibrate the FIFO sample rate on this hardware for improved accuracy.");
	}

	delay(500);
}
