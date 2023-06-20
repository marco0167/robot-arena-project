#ifdef ROBOT
#include <esp_now.h>
#include "esp_wifi.h"
#include <WiFi.h>
#include <motorControl.h>
#include <batteryMonitor.h>
#include <ledUtility.h>
#include "esp_log.h"
#include "mac.h"
static const char *TAG = "MAIN";

#define weapPot 7

//------------ turn on generic serial printing
// #define DEBUG_PRINTS

#define MOTOR_A_IN1 8
#define MOTOR_A_IN2 18

#define MOTOR_B_IN1 16
#define MOTOR_B_IN2 17

#define MOTOR_C_IN1 4
#define MOTOR_C_IN2 5

// RIGHT
MotorControl motor1 = MotorControl(MOTOR_B_IN1, MOTOR_B_IN2);
// LEFT
MotorControl motor2 = MotorControl(MOTOR_A_IN1, MOTOR_A_IN2);
// WPN
MotorControl motor3 = MotorControl(MOTOR_C_IN1, MOTOR_C_IN2);

BatteryMonitor Battery = BatteryMonitor();

LedUtility Led = LedUtility();

typedef struct
{
	int16_t speedmotorLeft;
	int16_t speedmotorRight;
	int16_t packetArg1;
	int16_t packetArg2;
	int16_t packetArg3;
} packet_t;
packet_t recData;
// packet_t sendData;


bool failsafe = false;
unsigned long failsafeMaxMillis = 400;
unsigned long lastPacketMillis = 0;

int spdMtrL = 0;
int spdMtrR = 0;
int spdWpn = 0;
int recArg2 = 0;
int recArg3 = 0;
int wpnPot = 0;


// // Callback when data is sent
// String success;
// esp_now_peer_info_t peerInfo;
// // Callback when data is sent
// void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
// {
// 	if (status == 0)
// 	{
// 		success = "Delivery Success :)";
// 	}
// 	else
// 	{
// 		success = "Delivery Fail :(";
// 	}
// }

// Callback when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
	memcpy(&recData, incomingData, sizeof(recData));
	// Access the received data and perform actions
	spdMtrL = recData.speedmotorLeft;
	spdMtrR = recData.speedmotorRight;
	spdWpn = recData.packetArg1;
	recArg2 = recData.packetArg2;
	recArg3 = recData.packetArg3;
	lastPacketMillis = millis();
	failsafe = false;
}

int handle_blink()
{
	if (Battery.isLow())
	{
		Led.setBlinks(1, 1000);
		return 1;
	}
	if (failsafe)
	{
		Led.setBlinks(2, 500);
		return -1;
	}
	Led.setBlinks(0);
	Led.ledOn();
	return 0;
}

void setup()
{
#ifdef DEBUG_PRINTS
	Serial.begin(115200);
	Serial.println("Ready.");
#endif
	analogReadResolution(10);
	Led.init();
	delay(20);
	Led.setBlinks(1, 150);
	delay(2000);
	Battery.init();
	delay(20);

	analogSetAttenuation(ADC_11db);
	motor1.setSpeed(0);
	motor2.setSpeed(0);
	motor3.setSpeed(0);
	delay(500);

	WiFi.mode(WIFI_STA);
	if (esp_wifi_set_mac(WIFI_IF_STA, &robotAddress[0]) != ESP_OK)
	{
		Serial.println("Error changing mac");
		return;
	}
	Serial.println(WiFi.macAddress());
	if (esp_now_init() != ESP_OK)
	{
		Serial.println("Error initializing ESP-NOW");
		return;
	}
	esp_now_register_recv_cb(OnDataRecv);
	// esp_now_register_send_cb(OnDataSent);
	Led.setBlinks(0);
	Led.ledOn();
}

void loop()
{
	Serial.println("loop");
	unsigned long current_time = millis();
	if (current_time - lastPacketMillis > failsafeMaxMillis)
	{
		failsafe = true;
	}
	handle_blink();
	if (failsafe)
	{
		motor1.setSpeed(0); // RESET A ZERO SE GUARDIA
		motor2.setSpeed(0);
		motor3.setSpeed(0);
	}
	else
	{
		// vvvv ----- YOUR AWESOME CODE HERE ----- vvvv //

		// inizierei chiamando un setspeed secco, poi passandogli in rapida sequenza  i due valori estremi
		// poi passandoglieli alternati (variabile if true ( var * -1 a ogni loop) ad esempio)7

		// motor1.setSpeed(spdMtrR);
		// motor2.setSpeed(spdMtrR);
		// wpnPot = analogRead(weapPot);
		// if (wpnPot > 3540)
		// {
		// 	motor2.setSpeed(200);
		// }
		// else
		// {
		// 	motor2.setSpeed(0);
		// }
		motor1.setSpeed(spdMtrL);
		motor2.setSpeed(spdMtrR);
		motor3.setSpeed(spdWpn);
		// sendData.packetArg3 = analogRead(weapPot);
		// esp_err_t result = -1;
		// result = esp_now_send(&robotAddress[0], (uint8_t *)&sendData, sizeof(sendData));
		// -------------------------------------------- //
	}
	delay(2);
}
#endif
