/*
 Name:		zasilacz_raspberrry.ino
 Created:	06.11.2017 18:15:04
 Author:	Piotr
*/

#include <Timer.h>
#include <Event.h>

// PINOUT FOR ARDUINO UNO
// INPUTS
#define POWER_BUTTON 8
#define TX_CONTROL 9

// ANALOG INPUT
#define OUTPUT_CONTROL 0

// OUTPUTS
#define SHUTDOWN_REQ_PIN 6
#define POWER_PIN 7

// SAFETY
#define EMERGENCY_MIN_VOLTAGE 465
#define EMERGENCY_MAX_VOLTAGE 530
#define DISABLE_EMERGENCY 0

// TIMERS PERIODS
#define BUTTON_DELAY 500
#define SERIAL_PERIOD 500
#define TX_PERIOD 200
#define EMERGENCY_PERIOD 1000

Timer mainTimer;

bool power_state = 0;
bool power_state_last = 0;
bool power_on_start_count = 0;
bool shutdown_request = 0;
bool shutdown_request_start_count = 0;
bool safe_shutdown = 0;
bool emergency_shutdown = 0;
bool power_output = 0;
bool tx_state;
bool tx_state_last;
int adc, voltage;
int shutdown_request_count = 0;
int power_on_count = 0;
int tx_count = 0;


unsigned long lastTimeButton, lastTimeTX, timeNow;

void setup() {
	// Inputs
	pinMode(TX_CONTROL, INPUT);
	pinMode(POWER_BUTTON, INPUT_PULLUP);

	// Outputs
	pinMode(POWER_PIN, OUTPUT);	
	pinMode(SHUTDOWN_REQ_PIN, OUTPUT);
	Serial.begin(9600);

	// Pins initializing
	digitalWrite(SHUTDOWN_REQ_PIN, shutdown_request);
	digitalWrite(POWER_PIN, !power_output);

	// Timers initializing
	int serialEvent = mainTimer.every(SERIAL_PERIOD, serialOutput);
	int txEvent = mainTimer.every(TX_PERIOD, checkTX1);
	int emergencyEvent = mainTimer.every(EMERGENCY_PERIOD, emergencyHandler);	
	unsigned long lastTimeButton = millis();
	}

void loop() {

	mainTimer.update();									// keeps all timers going
	timeNow = millis();									// actual time

	adc = analogRead(OUTPUT_CONTROL);					// read voltage from output divided by 2
	voltage = map(adc, 0, 519, 0, 501);					// measure conditioning
	tx_state = digitalRead(TX_CONTROL);					// Rpi_tx checking for safe shutdown
	push_button_switch_value(POWER_BUTTON, power_state, BUTTON_DELAY);	// button handler
	
	if (power_state_last == 1 && power_state == 1)		// power output goes high after some time
	{													// for emergency debouncing
		power_on_start_count = 1;
	}

	if (power_on_start_count == 1)						// if last condition was fulfilled, counting starts
	{
		if (power_state == 1)
			power_on_count++;
		else 
		{
			power_on_start_count = 0;
			power_on_count = 0;			
		}
	}

	if (power_on_count > 2000)							// after 2000 loop cycles power output can go high
	{
		power_output = 1;
		power_on_start_count = 0;
		power_on_count = 0;
	}

	if (power_state_last == 1 && power_state == 0)		// user can push power button during high power output
	{
		shutdown_request_start_count = 1;				// same counting as before, for emergency debouncing
	}

	if (shutdown_request_start_count == 1)
	{
		if (power_state == 0)
			shutdown_request_count++;
		else
		{
			shutdown_request_start_count = 0;
			shutdown_request_count = 0;
			shutdown_request = 0;
		}
	}
	if (shutdown_request_count > 2000)
		shutdown_request = 1;							// Rpi gets shutdown request signal, performs safe system closing

	if (power_state == 1)								// pin setting for safety reasons, no unexpected shutdowns
	{
		shutdown_request = 0;
		power_output = 1;
		
	}

	if (tx_state_last == 1 && tx_state == 0)			// if Rpi_tx line goes down it might be waiting for shutdown
	{
		checkTX1();
	}
	else												// reset counting if Rpi_tx line goes high again
	{	
		lastTimeTX = timeNow;							
	}

	if (safe_shutdown == 1)								// safe shutdown is performed after 5s Rpi_tx is low
	{
		power_state = 0;
		power_output = 0;
	}

	if (emergency_shutdown == 1)						// overvoltage and undervoltage protection, immediate shutdown
	{
		power_state = 0;
		power_output = 0;
	}

	if (power_output == 0)								// reset variables to initial state after shutdown
	{
		power_output = 0;
		shutdown_request_start_count = 0;
		shutdown_request_count = 0;
		shutdown_request = 0;
		safe_shutdown = 0;
		emergency_shutdown = 0;
		tx_count = 0;
	}
	digitalWrite(SHUTDOWN_REQ_PIN, shutdown_request);	// write variables and outputs after every loop
	digitalWrite(POWER_PIN, !power_output);
	power_state_last = power_state;
	tx_state_last = tx_state;
}

void checkTX1()											// checking if Rpi is shutten down
{
	if (digitalRead(TX_CONTROL) == 0 && power_output == 1)
		tx_count++;
	else
		tx_count = 0;
	if (tx_count >= 25)
	{
		safe_shutdown = 1;
		Serial.println("Safe Shutdown");
	}
}

void serialOutput()										// print variables to serial
{
	String toSerial = "PWR_S: ";
	toSerial += power_state;
	toSerial += "  ";
	toSerial += "BTN: ";
	toSerial += digitalRead(POWER_BUTTON);
	toSerial += "  ";
	toSerial += "TX: ";
	toSerial += digitalRead(TX_CONTROL);
	toSerial += "  ";
	toSerial += "Safe: ";
	toSerial += safe_shutdown;
	toSerial += "  ";
	toSerial += "S_req: ";
	toSerial += shutdown_request;	
	toSerial += "  ";
	toSerial += "V_out: ";
	toSerial += voltage;
	/*
	toSerial += "  ";
	toSerial += "adc: ";
	toSerial += adc;
	*/
	toSerial += "  ";
	toSerial += "Emer: ";
	toSerial += emergency_shutdown;
	toSerial += "  ";
	toSerial += "Tx_cnt: ";
	toSerial += tx_count;

	Serial.println(toSerial);
}

void emergencyHandler()									// overvoltage and undervoltage protection, overcurrent protection in future
{
	if (millis() > 4000 && power_output == 1 && !DISABLE_EMERGENCY)
	{
		
		if (voltage > EMERGENCY_MAX_VOLTAGE)
		{
			emergency_shutdown = 1;
		}
		if (voltage < EMERGENCY_MIN_VOLTAGE)
		{
			emergency_shutdown = 1;
		}
		if (voltage >= EMERGENCY_MIN_VOLTAGE && voltage <= EMERGENCY_MAX_VOLTAGE)
		{
			emergency_shutdown = 0;
		}
		if (emergency_shutdown == 1)
			Serial.println("EMERGENCY SHUTDOWN");
	}
}

void push_button_switch_value(byte button, bool &state, int button_delay)
{
	
unsigned long timeNow = millis();

	if (digitalRead(button) == 0) {
		if (timeNow - lastTimeButton > button_delay)
		{
			lastTimeButton = timeNow;
			if (digitalRead(button) == 0)
			{
				state = !state;
			}			
		}
	}
}
