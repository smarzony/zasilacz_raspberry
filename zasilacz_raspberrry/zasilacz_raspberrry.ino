/*
 Name:		zasilacz_raspberrry.ino
 Created:	06.11.2017 18:15:04
 Author:	Piotr
*/

#include <Button.h>
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

#define DISABLE_EMERGENCY 0

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
bool tx_timer = 0;
int adc, voltage;
int emergency_start = 0;
int shutdown_request_count = 0;
int power_on_count = 0;
int tx_count = 0;


unsigned long lastTimeButton, lastTimeTX, timeNow;

void setup() {
	Serial.begin(9600);
	pinMode(TX_CONTROL, INPUT);
	pinMode(POWER_BUTTON, INPUT_PULLUP);

	pinMode(POWER_PIN, OUTPUT);	
	pinMode(SHUTDOWN_REQ_PIN, OUTPUT);	

	digitalWrite(SHUTDOWN_REQ_PIN, shutdown_request);
	digitalWrite(POWER_PIN, !power_output);

	int serialEvent = mainTimer.every(500, serialOutput);
	int txEvent = mainTimer.every(200, checkTX1);
	int emergencyEvent = mainTimer.every(1000, emergencyHandler);	
	unsigned long lastTimeButton = millis();
	}

void loop() {

	mainTimer.update();
	timeNow = millis();	

	adc = analogRead(OUTPUT_CONTROL);
	voltage = map(adc, 0, 529, 0, 501);
	tx_state = digitalRead(TX_CONTROL);
	push_button_switch_value(POWER_BUTTON, power_state);
	
	if (power_state_last == 1 && power_state == 1)
	{
		power_on_start_count = 1;
	}

	if (power_on_start_count == 1)
	{
		if (power_state == 1)
			power_on_count++;
		else 

		{
			power_on_start_count = 0;
			power_on_count = 0;			
		}
	}

	if (power_on_count > 2000)
	{
		power_output = 1;
		power_on_start_count = 0;
		power_on_count = 0;
	}

	if (power_state_last == 1 && power_state == 0)
	{
		shutdown_request_start_count = 1;
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
		shutdown_request = 1;

	if (power_state == 1)
	{
		shutdown_request = 0;
		power_output = 1;
		
	}

	if (tx_state_last == 1 && tx_state == 0)
	{
		tx_timer = 1;
		//checkTX0();
		checkTX1();
	}
	else
	{
		lastTimeTX = timeNow;
	}

	if (safe_shutdown == 1)
	{
		power_state = 0;
		power_output = 0;
		//safe_shutdown = 0;
	}

	if (emergency_shutdown == 1)
	{
		power_state = 0;
		power_output = 0;
	}

	if (power_output == 0)
	{
		power_output = 0;
		shutdown_request_start_count = 0;
		shutdown_request_count = 0;
		shutdown_request = 0;
		safe_shutdown = 0;
		emergency_shutdown = 0;
		tx_count = 0;
	}
	digitalWrite(SHUTDOWN_REQ_PIN, shutdown_request);
	digitalWrite(POWER_PIN, !power_output);
	power_state_last = power_state;
	tx_state_last = tx_state;
}

void checkTX0()
{
	if (timeNow - lastTimeTX > 5000 && power_output == 1)
	{
		lastTimeTX = timeNow;
		safe_shutdown = 1;
		power_state = 0;
		shutdown_request = 0;
		Serial.println("Safe Shutdown");
	}
	else
	{
		safe_shutdown = 0;
	}	
}

void checkTX1()
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


void serialOutput()
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
	toSerial += safe_shutdown;/*
	toSerial += "  ";
	toSerial += "S_req: ";
	toSerial += shutdown_request;*/
	toSerial += "  ";
	toSerial += "V_out: ";
	toSerial += voltage;
	toSerial += "  ";
	toSerial += "Emer: ";
	toSerial += emergency_shutdown;
	toSerial += "  ";
	toSerial += "Tx_cnt: ";
	toSerial += tx_count;
	/*
	if (tx_state == 0 && power_output == 1)
	{
		toSerial += "  ";
		toSerial += "Time: ";
		toSerial += 5000 - (timeNow - lastTimeTX);
	}
	*/

	Serial.println(toSerial);
}

void emergencyHandler()
{
	if (millis() > 4000 && power_output == 1 && !DISABLE_EMERGENCY)
	{
		
		if (voltage > 520)
		{
			emergency_shutdown = 1;

		}
		if (voltage < 480)
		{
			emergency_shutdown = 1;

		}
		if (voltage >= 480 && voltage <= 520)
		{
			emergency_shutdown = 0;
		}
		if (emergency_shutdown == 1)
			Serial.println("EMERGENCY SHUTDOWN");
	}
}


void push_button_switch_value(byte button, bool &state)
{
	
unsigned long timeNow = millis();

	if (digitalRead(button) == 0) {
		if (timeNow - lastTimeButton > 500)
		{
			lastTimeButton = timeNow;
			if (digitalRead(button) == 0)
			{
				state = !state;
			}			
		}
	}
}
