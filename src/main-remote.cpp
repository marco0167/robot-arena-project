#ifdef REMOTE
#include <math.h>
#include <esp_now.h>
#include "esp_wifi.h"
#include <WiFi.h>
#include <batteryMonitor.h>
#include <ledUtility.h>
#include "esp_log.h"
#include "mac.h"

static const char *TAG = "MAIN";
//------------ turn on generic serial printing

#define DEBUG_PRINTS
// data that will be sent to the receiver

typedef struct
{
	int16_t speedmotorLeft;
	int16_t speedmotorRight;
	int16_t packetArg1;
	int16_t packetArg2;
	int16_t packetArg3;
} packet_t;

packet_t sentData;
packet_t recData;

//---------------------------------------ESP_NOW Variables

String success;
esp_now_peer_info_t peerInfo;
// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
	if (status == 0)
	{
		success = "Delivery Success :)";
	}
	else
	{
		success = "Delivery Fail :(";
	}
}
int prova = 0;
// Callback when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
	memcpy(&recData, incomingData, sizeof(recData));
	prova = recData.packetArg3;
	Serial.println(recData.packetArg3);
}

//---------------------------------------HARDWARE DEPENDANT Variables
// one ifdef case per hardware to speed up modularity of the code

// RAC standard remote
const int steerPot = 7;
const int accPot = 10;
const int leverPot = 8;
// const int trimPot = 39;

const int rightBtn = 2;
const int leftBtn = 4;
const int topBtn = 5;
// const int lowSwitch = 32;
// const int topSwitch = 25;
LedUtility Led(21);

// customisable vars
int analogRes = 10;
int analogReadMax = (1 << analogRes) - 1;

// variables for the sketch
int leverValue = 0;

unsigned long current_time = 0;

void setup()
{
	// store_values(); // uncomment only to initialize mem
	analogReadResolution(analogRes);
	analogSetAttenuation(ADC_11db);
	pinMode(rightBtn, INPUT_PULLUP);
	pinMode(leftBtn, INPUT_PULLUP);
	pinMode(topBtn, INPUT_PULLUP);
	// pinMode(lowSwitch, INPUT_PULLUP);
	// pinMode(topSwitch, INPUT_PULLUP);
	Led.init();
	Led.setBlinks(1, 150);
	delay(2000);
#ifdef DEBUG_PRINTS
	Serial.begin(115200);
	Serial.println("RAC GENERIC BOT");
#endif

	//---------------------------------------ESP NOW setup
	WiFi.mode(WIFI_STA);
	esp_wifi_set_channel(10, WIFI_SECOND_CHAN_NONE);
	if (esp_now_init() != ESP_OK)
	{
		Serial.println("Error initializing ESP-NOW");
		return;
	}

	esp_now_register_send_cb(OnDataSent);

	// Set the MAC address in peerInfo.peer_addr
	uint8_t mac[6] = {0x4A, 0x4A, 0x2E, 0x6E, 0x8C, 0x66};
	memcpy(peerInfo.peer_addr, mac, sizeof(mac));

	peerInfo.channel = 0;
	peerInfo.encrypt = false;

	if (esp_now_add_peer(&peerInfo) != ESP_OK)
	{
		Serial.println("Failed to add peer");
		return;
	}

	char macStr[18];
	snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
			 peerInfo.peer_addr[0], peerInfo.peer_addr[1], peerInfo.peer_addr[2], peerInfo.peer_addr[3], peerInfo.peer_addr[4], peerInfo.peer_addr[5]);
	Serial.print("sending to: ");
	Serial.println(macStr);
	esp_now_register_recv_cb(OnDataRecv);
	Led.setBlinks(0);
	Led.ledOn();
}

void	danceFunction()
{
	esp_err_t result = -1;

	// mezzo giro a sinistra
	sentData.speedmotorLeft = map(100, -100, 100, -512, 512);
	sentData.speedmotorRight = map(-100, -100, 100, -512, 512);
	sentData.packetArg1 = 512;
	result = esp_now_send(robotAddress, (uint8_t *)&sentData, sizeof(sentData));

	// delay(100);
	// sentData.packetArg1 = -512;
	// result = esp_now_send(robotAddress, (uint8_t *)&sentData, sizeof(sentData));
	delay(200);

	// mezzo giro a sinistra (essendo speedmotorLeft & speedmotorRight gia assegnati, non serve riassegnarli)
	sentData.packetArg1 = -512;
	result = esp_now_send(robotAddress, (uint8_t *)&sentData, sizeof(sentData));
	delay(200);

	sentData.packetArg1 = 512;
	result = esp_now_send(robotAddress, (uint8_t *)&sentData, sizeof(sentData));
	delay(200);

	sentData.packetArg1 = -512;
	result = esp_now_send(robotAddress, (uint8_t *)&sentData, sizeof(sentData));
	delay(200);

	// mezzo giro a destra
	sentData.speedmotorLeft = map(-100, -100, 100, -512, 512);
	sentData.speedmotorRight = map(100, -100, 100, -512, 512);
	sentData.packetArg1 = 0;
	result = esp_now_send(robotAddress, (uint8_t *)&sentData, sizeof(sentData));
	delay(400);

	sentData.speedmotorLeft = 0;
	sentData.speedmotorRight = 0;
	sentData.packetArg1 = 0;
	result = esp_now_send(robotAddress, (uint8_t *)&sentData, sizeof(sentData));
}

void loop()
{
	// read pots values
	delay(3);
	int strValue = analogRead(steerPot); // X sinistra 0  X Destra 1023
	delay(3);
	int accValue = analogRead(accPot); // Y Su 0  Y Giu 1023
	delay(3);
	int leverValue = analogRead(leverPot); // Su 676  Giu 274
	delay(3);
	current_time = millis();
	bool rightValue = !digitalRead(rightBtn);
	bool leftValue = !digitalRead(leftBtn);
	bool topValue = !digitalRead(topBtn);

	const int guidabilita = 2; // scegliere un valore tra 1.5 (per maggiore guidabilità) e 2 (per maggiore agilità)

	int rightMotorPercent = 0;
	int leftMotorPercent = 0;

	int yPercent = (((leverValue - 256) * 100 / 400) -50) *2; // sottraggo 274 per avere 0 come valore minimo e 404 come valore massimo, moltiplico per 100 e divido per 404 per avere un valore percentuale da 0 a 100 e sottraggo 50 per avere -50 come valore minimo e 50 come valore massimo e moltiplico per 2 per avere un valore da -100 a 100
	int xPercent = ((strValue * 100 / 1023) - 50) * guidabilita;	 // moltiplico per 100 e divido per 1023 per avere 0 come valore minimo e 100 come massimo, sottraggo 50 per avere -50 come valore minimo e 50 come valore massimo e moltiplico per 2 per avere un valore da -100 a 100

	// vvvv ----- YOUR AWESOME CODE HERE ----- vvvv //

	if (xPercent > -14 && xPercent < 6) // se il valore è compreso tra -14 e 6 lo porto a 0 questo perche il potenziometro non è perfettamente centrato
		xPercent = 0;
//sei uc coglione
	// Serial.print("X: ");
	// Serial.println(xPercent);

	if (yPercent > -12 && yPercent < 16) // se il valore è compreso tra -12 e 16 lo porto a 0 questo perche il potenziometro non è perfettamente centrato
		yPercent = 0;

	// Serial.print("Y: ");
	// Serial.println(yPercent);

	rightMotorPercent = (yPercent + xPercent) ; // calcolo la percentuale del motore destro
	leftMotorPercent = (yPercent - xPercent) ;  // calcolo la percentuale del motore sinistro

	// Serial.print("Right: ");
	// Serial.println(rightMotorPercent);

	if (rightMotorPercent > 100) // se il valore è maggiore di 100 lo porto a 100
		rightMotorPercent = 100;
	else if (rightMotorPercent < -100) // se il valore è minore di -100 lo porto a -100
		rightMotorPercent = -100;

	if (leftMotorPercent > 100) // se il valore è maggiore di 100 lo porto a 100
		leftMotorPercent = 100;
	else if (leftMotorPercent < -100) // se il valore è minore di -100 lo porto a -100
		leftMotorPercent = -100;

	// Example: Set speed values based on potentiometer readings
	sentData.speedmotorLeft = map(leftMotorPercent, -100, 100, -512, 512);
	sentData.speedmotorRight = map(rightMotorPercent, -100, 100, -512, 512);

	// -------------------------------------------- //
	// set weapon movement based on buttons pressed
	if (rightValue)
		sentData.packetArg1 = 512; // move weapon up
	else if (leftValue)
		sentData.packetArg1 = -512; // move weapon down
	else
		sentData.packetArg1 = 0; // stop weapon
	// -------------------------------------------- //

	// -------------------------------------------- //
	// if top button you will press, 🕺 RoboNapoliu  will make DANCE 🕺
	if (topValue)
		danceFunction();
	// -------------------------------------------- //

	// -------------------------------------------- //
	esp_err_t result = -1;
	// Serial.println(prova);
	// Serial.print("Right:");
	// Serial.println(rightMotorPercent);
	// Serial.print("Left:");
	// Serial.println(leftMotorPercent);
	result = esp_now_send(robotAddress, (uint8_t *)&sentData, sizeof(sentData));
	if (result == ESP_OK)
	{
		// Serial.println("Sent with success");
	}
	else
	{
		// Serial.println("Error sending the data");
	}
	delay(10);
}
#endif
