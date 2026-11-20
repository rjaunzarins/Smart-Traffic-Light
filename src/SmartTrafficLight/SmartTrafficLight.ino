#include <Wire.h>		// For PCF8574 I2C Expander used in hardware FSM validation


// Traffic State Enum
// Total of 8 states therefore underlying int is uint8
enum class TrafficState : uint8_t {
  	STATE_000 = 0,	// All Red
  	STATE_001 = 1,	// Advanced Green Left
  	STATE_010 = 2,	// Green Left/Right
  	STATE_011 = 3,	// Yellow Left/Right
  	STATE_100 = 4,	// All red
	STATE_101 = 5,	// Green Forward
  	STATE_110 = 6,	// Yellow Forward
  	STATE_111 = 7,	// Unused state
};



/**
 * I2C Expander
 * Expand A4 and A5 on the Arduino to 8 external digital I/O 
 * ports through I2C bus.
 */
class PCF8574 {
  	private:
  		uint8_t address = 0x20;
  	public:
  		// Configure inital port state
  		bool initializePorts() {
      		return writePorts(0xFF);
    	}
  		// Write all eaigh port states
  		bool writePorts(uint8_t value) {
          	Wire.beginTransmission(address);
          	Wire.write(value);
          	if(Wire.endTransmission() != 0)
              	return false;
          	return true;
        }
  		// Read the current logic levels of all ports
  		bool readPorts(uint8_t& value) {
          	if(Wire.requestFrom(address, (uint8_t)1) != 1)
              	return false;
          	value = Wire.read();
          	return true;
        }
};


/**
 * Buzzer Class
 */
class Buzzer {
	private:
  		int buzzerPin;
  		bool active = false;
  		bool outputState = false;
  		unsigned long lastToggle = 0;
  		unsigned long lastCycle = 0;
  
  	public:
  		// Constructor
  		Buzzer(int pin) : buzzerPin(pin) {}
  		
  		// Configure buzzer
  		void configure() {
            pinMode(buzzerPin, OUTPUT);
            digitalWrite(buzzerPin, LOW);
        }
  		// Activate sound
        void activate() {
          	if(!active) {
              	active = true;
              	lastToggle = micros();
              	lastCycle = millis();
            }
        }
  		// Deactivate sound
	  	void deactivate() {
        	if(!active) return;
          	active = false;
          	outputState = false;
          	digitalWrite(buzzerPin, LOW);
        }
  		// Update - Play the sound
  		// Buzzer is pulsed due to it slowing Tinkercad down dramatically
  		void update() {
          	if(!active) return;
          
          	unsigned long currentTime = millis();
          	unsigned long cycleTime = currentTime - lastCycle;
          
          	// Cycle every 1 second
          	if(cycleTime >= 1000) {
              	lastCycle = currentTime;
              	cycleTime = 0;
            }
          	// Stop after 100ms
          	if(cycleTime >= 100) {
              	if(outputState) {
              		outputState = false;
              		digitalWrite(buzzerPin, LOW);
                }
              	return;
            }
          	// When on toggle every 750us (~670Hz)
          	unsigned long currentMicros = micros();
          	if(currentMicros - lastToggle >= 750) {
              	lastToggle = currentMicros;
              	outputState = !outputState;
              	digitalWrite(buzzerPin, outputState);
            }
        } 	
};


/**
 * Light Sensor Class
 * Detect the ambiet light levels to detect nighttime mode
 */
class LightSensor {
	private:
  		const uint8_t sensorPin = A0;
  		const uint8_t lightingPin = 13;
		int lightLevel = 0;
  		bool nighttime = false;
  		// Light sampling interval: 10 sec
  		const unsigned long samplingInterval = 10000;
  		unsigned long lastSampleTime = 0;
  		// Two thresholds to avoid flickering at threshold values
  		const int nightThreshold = 350;
  		const int dayThreshold = 450;
  	
  	public:
  		// Configure sensor
  		void configure() {
          	pinMode(sensorPin, INPUT);
          	pinMode(lightingPin, OUTPUT);
          	digitalWrite(lightingPin, LOW);
          	// Sample immediately on configure
          	lastSampleTime = millis() - samplingInterval;
        }
  
  		// Update ambient light measurements
  		void update() {
          	unsigned long currentTime = millis();
          	if(currentTime - lastSampleTime < samplingInterval)
              	return;
          	lastSampleTime = currentTime;
          	lightLevel = analogRead(sensorPin);
          	// Turn on lights and nighttime mode
	        if(lightLevel < nightThreshold && !nighttime) {
              	nighttime = true;
              	digitalWrite(lightingPin, HIGH);
            }
          	// Turn off lights and nighttime mode
          	else if(lightLevel > dayThreshold && nighttime) {
              	nighttime = false;
              	digitalWrite(lightingPin, LOW);
            }
        }
  
  		// Check nightime status
  		bool isNighttime() const {
          	return nighttime;
        }
};


/**
 * Ultrasonic Sensor Class
 * 
 */
class UltrasonicSensor {
  	private:
  		int triggerPin;
  		int echoPin;
 		// Sensor measurements
  		double simulatedDistanceMeters;
  		double measuredDistanceCentimeters;
  		// Sensor scaling
  		const double centimetersPerSimulatedMeter = 4.;
  		// Vehicle detection threshold
  		const double minVehicleDetectionThresholdMeters = 24.;
  		const double maxVehicleDetectionThresholdMeters = 35.;
  		// Max measurement range of sensor
  		const double minSensorDistanceCentimeters = 4.;
  		const double maxSensorDistanceCentimeters = 330.;
  	
  	public:
  		// Constructor
  		UltrasonicSensor(int trig, int echo) 
          	: triggerPin(trig),
  			  echoPin(echo),
  			  simulatedDistanceMeters(-1.),
  			  measuredDistanceCentimeters(-1.) {}
  		// Configure sensor - must be completed after Arduino initialization
  		void configure() {
          	pinMode(triggerPin, OUTPUT);
          	pinMode(echoPin, INPUT);
          	digitalWrite(triggerPin, LOW);
        }
  
  		// Update sensor
  		void update() {
    		// Set trigger LOW for a short period to ensure pulse is clean
          	digitalWrite(triggerPin, LOW);
          	delayMicroseconds(2);
          	// Send a 10us trigger pulse to request a measurement
          	digitalWrite(triggerPin, HIGH);
          	delayMicroseconds(10);
          	// Set trigger LOW again to end pulse
          	digitalWrite(triggerPin, LOW);
          	// Calculate the RTT - Stop after 30ms if no echo is received
          	unsigned long echoDurationMicroseconds = pulseIn(echoPin, HIGH, 30000);
          	// If no valid distance is measured, set to -1.
          	if(echoDurationMicroseconds == 0) {
              	measuredDistanceCentimeters = -1.;
              	simulatedDistanceMeters = -1.0;
              	return;
            } 
          	// Calculate the distance in centimeters
          	measuredDistanceCentimeters = (echoDurationMicroseconds * 0.0343) / 2.;
          	// Reject measurements outside the max range of the sensor
          	if(measuredDistanceCentimeters < minSensorDistanceCentimeters ||
               measuredDistanceCentimeters > maxSensorDistanceCentimeters) {
            	measuredDistanceCentimeters = -1.;
              	simulatedDistanceMeters = -1.;
              	return;
            }
          	// Convert the distance to meters and apply scaling (4cm = 1m)
          	simulatedDistanceMeters = measuredDistanceCentimeters / centimetersPerSimulatedMeter;
      	}
  
  		// Get the simulated intersection distance
	  	double getSimulatedDistanceMeters() const {
          	return simulatedDistanceMeters;
        }
  		// Get the measured distance before applying scaling
	 	double getMeasuredDistanceCentimeters() const {
          	return measuredDistanceCentimeters;
        }
  		// Get vehicle detection status
  		bool isVehicleDetected() const {
          	return simulatedDistanceMeters >= minVehicleDetectionThresholdMeters && 
              	   simulatedDistanceMeters <= maxVehicleDetectionThresholdMeters;
        }
};


/**
 * TrafficController Class
 */
class TrafficController {
	private:
  		UltrasonicSensor rightSensor{3, 2};
  		UltrasonicSensor forwardSensor{5, 4};
  		UltrasonicSensor leftSensor{7, 6};
	  	Buzzer leftBuzzer{A1};
		Buzzer rightBuzzer{A2};
  		Buzzer forwardBuzzer{A3};
  		// Ultrasonic sensor sampling interval: 500 ms
  		const unsigned long samplingInterval = 500;
  		unsigned long lastSampleTime = 0;
  
  	public:
  		// Set all pins for Left, Right, and Forward sensors
  		void configureSensors() {
          	rightSensor.configure();
          	leftSensor.configure();
          	forwardSensor.configure();
          	rightBuzzer.configure();
          	leftBuzzer.configure();
          	forwardBuzzer.configure();
        }
  		// Update ultrasonic sensor measurements every 500 ms
  		bool updateSensors() {
          	unsigned long currentTime = millis();
          	// Sample after 500 ms from last sample
          	if(currentTime - lastSampleTime < samplingInterval)
              	return false;
          	lastSampleTime = currentTime;
        	// Update sensors
          	leftSensor.update();
          	rightSensor.update();
          	forwardSensor.update();
          	
          	return true;
        }
  		// Request advanced green for right signal
  		bool isAdvancedGreenStatus() const {
          	return isLeftVehicleDetected() && isRightVehicleDetected();
        }
  		// Determine if Left OR Right vehicle detected for daytime L/R green extension
  		bool isLOrRVehicleDetected() const {
          	return isLeftVehicleDetected() || isRightVehicleDetected();
        }
  		// Determine if a Left vehicle is detected
  		bool isLeftVehicleDetected() const {
          	return leftSensor.isVehicleDetected();
        }
  		// Determine if a Right vehicle is detected
  		bool isRightVehicleDetected() const {
          	return rightSensor.isVehicleDetected();
        }
  		// Determine if a Forward vehicle is detected
  		bool isForwardVehicleDetected() const {
          	return forwardSensor.isVehicleDetected();
        }
  
  		// Update buzzers based on vehicle detection
  		void updateBuzzers() {
          	if(leftSensor.isVehicleDetected()) {
              	leftBuzzer.activate();
            }
            else {
              	leftBuzzer.deactivate();
            }
          	if(forwardSensor.isVehicleDetected()) {
              	forwardBuzzer.activate();
            }
            else {
              	forwardBuzzer.deactivate();
            }
          	if(rightSensor.isVehicleDetected()) {
              	rightBuzzer.activate();
            }
            else {
              	rightBuzzer.deactivate();
            }
        }
  
  		// Play buzzer sounds
  		void sendBuzzerSignal() {
          	leftBuzzer.update();
          	rightBuzzer.update();
          	forwardBuzzer.update();
        }
  		// Print the sensor data for debugging - unused
  		void printSensorStatus() const {
        	// Display the simulated distances and detection status (3 decimal places)
          	Serial.print("Left: ");
          	Serial.print(leftSensor.getMeasuredDistanceCentimeters(), 3);
          	Serial.print(" cm | ");
            Serial.print(leftSensor.getSimulatedDistanceMeters(), 3);
            Serial.print(" m (");
            Serial.println(leftSensor.isVehicleDetected() ? "Vehicle detected" : "No vehicle detected");
			
            Serial.print(") Right: ");
            Serial.print(rightSensor.getMeasuredDistanceCentimeters(), 3);
          	Serial.print(" cm | ");
            Serial.print(rightSensor.getSimulatedDistanceMeters(), 3);
            Serial.print(" m (");
            Serial.println(rightSensor.isVehicleDetected() ? "Vehicle detected" : "No vehicle detected");

            Serial.print(") Forward: ");
            Serial.print(forwardSensor.getMeasuredDistanceCentimeters(), 3);
          	Serial.print(" cm | ");
            Serial.print(forwardSensor.getSimulatedDistanceMeters(), 3);
            Serial.print(" m (");
            Serial.print(forwardSensor.isVehicleDetected() ? "Vehicle detected" : "No vehicle detected");
            Serial.println(")");
        }
};


/**
 * Traffic State Controller
 *
 */
class TrafficStateController {
  	private:
  		// All traffic light timings are scaled down 3:1
  		// L/R daytime green durations
  		const unsigned long lrGreenDay = 16000;					// 16 sec    -> Real time: 48 sec
  		const unsigned long lrGreenDayForwardRequest = 10000;	// 10 sec    -> Real time: 30 sec
  		const unsigned long lrGreenDayMaximum = 26000;			// 26 sec    -> Real time: 78 sec
  		
		// L/R green extension window time
  		const unsigned long lrGreenDetectionWindow = 2000;		// 2 sec	 -> Real time: 6 sec
  
  		// L/R green extension tracking
  		unsigned long lrGreenStartTime = 0;						// Track time of L/R green start
  		unsigned long lrGreenWindowStartTime = 0;				// Track how long L/R window active
  		bool lrGreenVehicleDetectedInWindow = false;			// Track if vehicle detected in window
  		bool lrGreenNoVehicleDetectedInFullWindow = false;		// True if no vehicle detected in full 2 sec window
  
  		// L/R nighttime minimum green duration
  		const unsigned long lrGreenNightMinimum = 10000;		// 10 sec    -> Real time: 30 sec
  	
  		// Forward green duration
  		const unsigned long forwardGreenDuration = 6000;		// 6 sec     -> Real time: 18 sec
  		
  		// Other durations
  		const unsigned long advancedGreenDuration = 3000;		// 3 sec     -> Real time: 9 sec
  		const unsigned long yellowDuration = 1333;				// 1.333 sec -> Real time: 4 sec
  		const unsigned long allRedDuration = 666;				// 0.666 sec -> Real time 2 sec
  		
  		// Min wait after forward vehicle detected waiting on Forward red
  		const unsigned long nighttimeRequestDelay = 2000;		// 2 sec     -> Real time: 6 sec
  		
  		// Forward light request
  		bool forwardRequestPending = false;
  		unsigned long forwardRequestTime = 0;
  
  	public:
  		// Ensure Forward request is recorded while L/R is green
  		// State is uint8_t due to Tinkercad enum class problem
  		void updateForwardRequest(uint8_t state, bool vehicleDetected) {
          	// If state is not L/R Green do nothing
          	if(static_cast<TrafficState>(state) != TrafficState::STATE_010)
              	return;
          	// Once a vehicle is detected it is latched so a car doesn't wait
          	// indefinitely if there is a sensor reading issue after its detected
          	if(vehicleDetected && !forwardRequestPending) {
              	forwardRequestPending = true;
              	forwardRequestTime = millis();
            }
        }
  
  		// Monitor L/R green vehicle detections within the two second extension window
  		void recordLRGreenVehicleDetectionWindow(bool vehicleDetected, bool sensorUpdated) {
			// Get current time and time elapsed since lrGreen start
          	unsigned long currentTime = millis();
          	unsigned long elapsed = currentTime - lrGreenStartTime;
          	// Begin monitoring a 8 sec (2 seconds before shortest possible end time)
          	if(lrGreenWindowStartTime == 0) {
              	if(elapsed < 8000)
                  	return;
              	// Set window start time explicitly to ensure windows align with signal phase times
              	lrGreenWindowStartTime = lrGreenStartTime + 8000;
            }
          	// Check if the current window has finished
          	if(currentTime - lrGreenWindowStartTime >= lrGreenDetectionWindow) {
              	// Record if no vehicles detected in a full window
              	lrGreenNoVehicleDetectedInFullWindow = !lrGreenVehicleDetectedInWindow;
              	// Begin the next window at scheduled time (not necessarily current time)
              	lrGreenWindowStartTime += lrGreenDetectionWindow;
              	lrGreenVehicleDetectedInWindow = false;
            }
          	// Get new detection results 
          	if(sensorUpdated && vehicleDetected)
              	lrGreenVehicleDetectedInWindow = true;
        }
  
  		// Start L/R green timing for phase length calculation
  		void startLRGreenPhaseTiming() {
          	lrGreenStartTime = millis();
          	lrGreenWindowStartTime = 0;
          	lrGreenVehicleDetectedInWindow = false;
          	lrGreenNoVehicleDetectedInFullWindow = false;
        }
  
  		// Return the min L/R green duration
  		// Cannot pass TrafficState object in Tinkercad so static_cast is used in function
  		unsigned long getCurrentStateDurationLRGreen(bool nighttime) const {
          	// Nighttime
          	if(nighttime)
              	return lrGreenNightMinimum;
          	// Daytime w/Forward request
          	if(forwardRequestPending)
              	return lrGreenDayForwardRequest;
          	// Daytime w/o Forward request
          	return lrGreenDay;
        }
  
  		// Return the min duration for the current state
  		// State is uint8_t due to Tinkercad enum class problem
  		unsigned long getCurrentStateDuration(uint8_t state, bool nighttime) const {
          	switch(static_cast<TrafficState>(state)) {
              	// All red states
              	case TrafficState::STATE_000:
              	case TrafficState::STATE_100:
              		return allRedDuration;
              	// Advanced green
              	case TrafficState::STATE_001:
              		return advancedGreenDuration;
              	// L/R green
              	case TrafficState::STATE_010:
              		return getCurrentStateDurationLRGreen(nighttime);
              	// L/R or Forward yellow
              	case TrafficState::STATE_011:
              	case TrafficState::STATE_110:
              		return yellowDuration;
              	// Forward green
              	case TrafficState::STATE_101:
              		return forwardGreenDuration;
              	// Unused state in FSM is all red
              	default:
              		return allRedDuration;
            }
        }
  
  		// Determine if the L/R green can advance
  		bool isLRGreenTransitionAllowed(bool nighttime) const {
          	// Nighttime: remain green until Forward request
          	if(nighttime) {
              	if(!forwardRequestPending)
                  	return false;
              	return millis() - forwardRequestTime >= nighttimeRequestDelay;
            }
            // Daytime: max L/R green duration
            unsigned long elapsed = millis() - lrGreenStartTime;
          	if(elapsed >= lrGreenDayMaximum) {
              	return true;
            }
            // Must have full window elapse
            if(lrGreenWindowStartTime == 0)
              	return false;
            // A complete window must end after min green duration
            if(lrGreenWindowStartTime - lrGreenStartTime < getCurrentStateDurationLRGreen(false))
              	return false;
            // A Forward request cant use an earlier completed window
            if(forwardRequestPending && forwardRequestTime > lrGreenWindowStartTime)
              	return false;
            // Transition when the completed window has no L/R vehicles
            return lrGreenNoVehicleDetectedInFullWindow;
        }  
  
  		// Determine whether the current state can advance
  		// State is uint8_t due to Tinkercad enum class problem
  		bool isStateTransitionAllowed(uint8_t state, bool nighttime) const {
          	// States other than LR Green follow set durations
          	if(static_cast<TrafficState>(state) == TrafficState::STATE_010)
              	return isLRGreenTransitionAllowed(nighttime);
          	// Other states follow set times
          	return true;
        }
  
  		// Initialize L/R green timings and clear Forward requests on state changes
  		// newState is uint8_t due to Tinkercad enum class problem
	  	void resetOnStateAdvance(uint8_t newState) {
          	// Cast newState back to intended TrafficState type
          	TrafficState state = static_cast<TrafficState>(newState);
          	// Start timing when entering L/R green phase
          	if(state == TrafficState::STATE_010)
              	startLRGreenPhaseTiming();
          	// Clear the Forward request when entering L/R yellow
          	if(state == TrafficState::STATE_011) {
              	forwardRequestPending = false;
              	forwardRequestTime = 0;
            }
        }
};


/**
 * Software State Monitor
 * Tracks the expected hardware FSM state.
 * Advances only when the Arduino sends a clock pulse
 * to the hardware FSM
 */
class SoftwareStateMonitor {
	private:
  		TrafficState expectedState = TrafficState::STATE_000;
  	public:
  		// Reset the expected state to the hardware reset state
  		void reset() {
          	expectedState = TrafficState::STATE_000;
        }
  
  		// Predict the next state using the FSM transition rules
  		void advance(bool advancedRequest) {
          	switch(expectedState) {
              	case TrafficState::STATE_000:
              		expectedState = advancedRequest ? TrafficState::STATE_001 : TrafficState::STATE_010;
              		break;
             	case TrafficState::STATE_001:
              		expectedState = TrafficState::STATE_010;
              		break;
              	case TrafficState::STATE_010:
              		expectedState = TrafficState::STATE_011;
              		break;
              	case TrafficState::STATE_011:
              		expectedState = TrafficState::STATE_100;
              		break;
              	case TrafficState::STATE_100:
              		expectedState = TrafficState::STATE_101;
              		break;
              	case TrafficState::STATE_101:
              		expectedState = TrafficState::STATE_110;
              		break;
              	case TrafficState::STATE_110:
              		expectedState = TrafficState::STATE_000;
              		break;
				default:
              		expectedState = TrafficState::STATE_000;
              		break;
            }
		}
  
  		// Return the predicted state
  		TrafficState getExpectedState() const {
          	return expectedState;
        }
};
  
/**
 * System Monitor
 * Stores the current hardware FSM state and the status of
 * the NORMAL_ENABLE signal from the hardware supervisor
 * that governs the traffic signals normal operation.
 */
class HardwareStateMonitor {
  	private:
  		TrafficState currentState = TrafficState::STATE_000;
  		bool normalEnable = false;
  		bool validReading = false;
  	public:
  		// Update the system state after a successfully reading hardware
  		// State is uint8_t due to Tinkercad enum class problem
  		bool updateHardwareStatus(uint8_t state, bool enable) {
          	TrafficState typesafeState = static_cast<TrafficState>(state);
          	// Reject the unused state 111
          	//if(state > 6) {
          	if(typesafeState == TrafficState::STATE_111) {  
          		invalidate();
               	return false;
            }
          	// cast back to TrafficState
          	currentState = typesafeState;
          	normalEnable = enable;
          	validReading = true;
          
          	return true;
        }
  		//Invalidate the current state if hardware communication fails
  		void invalidate() {
          	validReading = false;
          	normalEnable = false;
        }
  		// Check if normal traffic is allowed
  		bool isNormalEnabled() const {
    		return validReading && normalEnable;
        }
  		// Check if the FSM state matches the expected software state
  		bool matchesExpectedState(TrafficState expected) const {
          	return isNormalEnabled() && currentState == expected;
        }
  
  		// The PCF8574 expands the number of I/O pins on the Arduino and reads all
  		// 3 FSM bits and NORMAL_ENABLE in one I2C transaction.
  		// This reduces the risk of state changes during reading.
  		bool readHardwareState(class PCF8574& expander) {
          	uint8_t portValues;
            // Attempt to read the hardware signals
            if(!expander.readPorts(portValues)) {
              invalidate();
              return false;
            }
            // Get hardware state and NORMAL_ENABLE
            // One byte is read at once from expander, therefore
            // to get individual FSM bits use & on 3 lowest bits
            // P0-P2 on expander is Q_C, Q_B, Q_A
			uint8_t hardwareState = portValues & 0b00000111;
            // NORMAL_ENABLE is on bit 3 and is a boolean
            bool normalEnable = (portValues & 0b00001000) != 0;

            // Store and validate the reading
            return updateHardwareStatus(hardwareState, normalEnable);
        }
  
  		// Verify the hardware FSM with 3 readings if required.
  		// Expected is uint8_t due to Tinkercad enum class problem
  		bool verifyState(class PCF8574& expander, uint8_t expected) {
          	const uint8_t maxAttempts = 3;
          	for(uint8_t attempt = 0; attempt < maxAttempts; ++attempt) {
              	// Read and validate the current hardware state
              	if(!readHardwareState(expander))
                	continue;
              	// Confirm the FSM matches software prediction
              	// Cast required because Tinkercad would not this function to accept
              	// a TrafficState type but it does allow matchesExpectedState to
              	if(matchesExpectedState(static_cast<TrafficState>(expected)))
                   	return true;
			}
			// If verification fails after 3 attempts invalidate and return false
            invalidate();
			return false;
		}
};


/**
 * FSM Signal Controller
 * Controls all output signals to FSM
 */
class FSMSignalController {
 	private:
  		// FSM control pins
  		const uint8_t clockPin = 8;
  		const uint8_t resetPin = 9;
  		const uint8_t xPin = 10;
  		const uint8_t startupCompletePin = 11;
  		const uint8_t startupFlashPin = 12;
        // Clock timing
        unsigned long lastClockTime = 0;
  		// Software fault latch
  		bool faultLatched = false;
  	public:
  		// Configure the FSM Signal Controller
	  	void configureFSMSignals() {
          	// Configure pins
          	pinMode(clockPin, OUTPUT);
          	pinMode(resetPin, OUTPUT);
          	pinMode(xPin, OUTPUT);
          	pinMode(startupCompletePin, OUTPUT);
          	pinMode(startupFlashPin, OUTPUT);
          	// Write appropriate signal
          	digitalWrite(clockPin, LOW);
          	digitalWrite(resetPin, HIGH);
          	digitalWrite(xPin, LOW);
          	digitalWrite(startupCompletePin, LOW);
          	digitalWrite(startupFlashPin, LOW);
        }
  		// Reset the clock timer
  		void resetClockTimer() {
          	lastClockTime = millis();
        }
  
  		// Generate a clock pulse when required duration has elapsed
  		bool updateClockSignal(unsigned long stateDuration, bool transitionAllowed) {
          	// Do not send clock pulse if fault latched or transition is not allowed
          	if(faultLatched || !transitionAllowed)
              	return false;
          	// Do not send clock pulse if time has not elapsed
          	if(millis() - lastClockTime < stateDuration)
              	return false;
          	// Send pulse and reset last clock time
          	digitalWrite(clockPin, HIGH);
          	delayMicroseconds(100);
          	digitalWrite(clockPin, LOW);
          	lastClockTime = millis();
          	return true;
        }
  
  		// Reset the FSM signal
  		void resetFSMSignal() {
          	digitalWrite(resetPin, LOW);
          	delay(100);
          	digitalWrite(resetPin, HIGH);
        }
  		// Set the advanced-green request signal (X)
  		void setAdvancedRequestSignal(bool requested) {
			// If fault, do not send
          	if(faultLatched)
              	return;
          	// Send signal if requested
          	digitalWrite(xPin, requested ? HIGH : LOW);
		}
		// Activate the startup flashing red signal
		void enableStartupFlashSignal() {
          	digitalWrite(startupFlashPin, HIGH);
        }
		// Disable startup flashing red signal
		void disableStartupFlashSignal() {
          	if(faultLatched)
              	return;
          	digitalWrite(startupFlashPin, LOW);
        }
		// Enable normal FSM operation after startup complete
		void enableNormalOperationSignal() {
          	if(faultLatched)
              	return;
          	digitalWrite(startupCompletePin, HIGH);
        }
		// Disable normal FSM operation 
		void disableNormalOperationSignal() {
          	digitalWrite(startupCompletePin, LOW);
        }
  		// Hold clock signal LOW
  		void disableClockSignal() {
          	digitalWrite(clockPin, LOW);
        }
  
		// Latch a software fault and request flashing red signals
		void triggerFaultSignal() {
          	if(faultLatched) return;
          	// Disable normal operation, request flashing red, and ensure clock low
			disableNormalOperationSignal();
          	enableStartupFlashSignal();
         	disableClockSignal();
          	faultLatched = true;
        }
};

// ----------- CLASSES END ----------- //


// Ambient light sensor
LightSensor lightSensor;
// I2C expander for hardware monitoring
PCF8574 ioExpander;
// Monitor the hardware FSM and NORMAL_ENABLE
HardwareStateMonitor hardwareStateMonitor;
// Track the expected hardware state in software
SoftwareStateMonitor softwareStateMonitor;
// Control signals to the hardware FSM
FSMSignalController fsmSignalController;
// Traffic controller
TrafficController trafficController;
// Traffic state controller controls light timings
TrafficStateController trafficStateController;

// Advanced green is requested when vehicles are detected at L/R
bool advancedRequest = false;


// ----------- SETUP ----------- //
void setup() {
    Serial.begin(9600);
  	Serial.println("SYSTEM STARTUP");
  	Serial.println("Press the hardware RESET button to initialize the circuit");
  	Serial.println("Hardware RESET button is located to the left of the Arduino");
  	Serial.println("Click several times until crosswalk LEDs turn off and back on");
	
  	// Initialize I2C bus for expander
  	Wire.begin();
  	// Init for reading hardware signals on expander
	if(!ioExpander.initializePorts()) {
      	Serial.println("ERROR: I2C expander init failed");
      	hardwareStateMonitor.invalidate();
      	fsmSignalController.configureFSMSignals();
      	fsmSignalController.triggerFaultSignal();
      	// Remain in fault conditions until reset.
      	// If I2C expander initialization fails, the HardwareStateMonitor cannot verify 
      	// the hardware's FSM state againgst the SoftwareStateMonitor predicted state.
      	while(true) {}
    }
  
  	// Configure the FSM Signal Controller
	fsmSignalController.configureFSMSignals();

  	// Configure light sensor
  	lightSensor.configure();
  	lightSensor.update();

    // Configure Ultrasonic Sensors and Buzzers through traffic controller
  	trafficController.configureSensors();

  	// Reset the software prediction  to initial state
  	softwareStateMonitor.reset();
  
    // Reset both hardware FSMs to state 000.
	fsmSignalController.resetFSMSignal();
	// Startup: flashing red for 5 seconds.
  	fsmSignalController.enableStartupFlashSignal();
  	delay(5000);
	// Clearance: disable flashing and hold all red for 3 seconds
	fsmSignalController.disableStartupFlashSignal();
	delay(3000);
	// Permit normal operation after startup.
	fsmSignalController.enableNormalOperationSignal();
	// Start the normal FSM timing from this point.
	fsmSignalController.resetClockTimer();
}


// ----------- LOOP ----------- //
void loop() {
  
  	// Update ambient light measurement
  	lightSensor.update();
  
  	// Read sensor inputs and store result
  	bool sensorUpdated = trafficController.updateSensors();
  	// Read sensor inputs and activate or deactivate buzzers based on vehicle detection
	if(sensorUpdated)
    	trafficController.updateBuzzers();
  
    // Set X (advancedRequest) for the current cycle.
	fsmSignalController.setAdvancedRequestSignal(advancedRequest);

  	// Store vehicle requests while green
  	trafficStateController.updateForwardRequest(
      	static_cast<uint8_t>(softwareStateMonitor.getExpectedState()),
      	trafficController.isForwardVehicleDetected()
    );
  
  	// Get the current expected FSM state
  	TrafficState currentState  = softwareStateMonitor.getExpectedState();
  	// Determine if it is nighttime
  	bool nighttime = lightSensor.isNighttime();
  
  	// Record L/R green daytime vehicle detections	
  	if(currentState == TrafficState::STATE_010 && !nighttime) {
      	bool lrVehicleDetected = trafficController.isLOrRVehicleDetected();
      	trafficStateController.recordLRGreenVehicleDetectionWindow(lrVehicleDetected, sensorUpdated);
    }
  	// Get the current state duration - Cast because of Tinkercad enum class issue
  	unsigned long stateDuration = trafficStateController.getCurrentStateDuration(
      	static_cast<uint8_t>(currentState),
      	nighttime
    );
  	// Determine if the state can advance to next state - cast because of Tinkercad
  	bool transitionAllowed = trafficStateController.isStateTransitionAllowed(
      	static_cast<uint8_t>(currentState),
      	nighttime
    );
  
    // Advance only when the state duration has elapsed.
  	if(fsmSignalController.updateClockSignal(stateDuration, transitionAllowed)) {
      
		// Advance the software state prediction after clock pulse
      	softwareStateMonitor.advance(advancedRequest);
      
      	// Start timing on LR green & Clear Forward request on L/R yellow
      	trafficStateController.resetOnStateAdvance(
          	static_cast<uint8_t>(softwareStateMonitor.getExpectedState())
        );
      	// Delay to ensure synchronization between hardware and software
      	delayMicroseconds(100);
      
      	// Verify the hardware FSM state and software FSM state match
      	bool stateVerified = 
          	hardwareStateMonitor.verifyState(
          		ioExpander,
				static_cast<uint8_t>(softwareStateMonitor.getExpectedState())
        );
      	// Activate flashing-red fallback mode if state verification fails
      	if(!stateVerified)
          	fsmSignalController.triggerFaultSignal();
        
      	// Refresh the advanced-green request with the cycle returns to STATE_000
      	if(softwareStateMonitor.getExpectedState() == TrafficState::STATE_000)
          	advancedRequest = trafficController.isAdvancedGreenStatus();
    }
  // Play active buzzer sounds - CURRENTLY DISABLED FOR TESTING
  trafficController.sendBuzzerSignal();
}