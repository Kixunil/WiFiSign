#include <ESP8266WiFi.h>
#include "secrets.h"
#include "localization_en.h"

WiFiServer server(80);
int state;

void sendGET(const char * host, const char *path) {
	WiFiClient client;
	
	if (!client.connect(host, 80)) {
		Serial.print("Connection to ");
		Serial.print(host);
		Serial.println(" failed");
	}

	client.print("GET ");
	client.print(path);
	client.print(" HTTP/1.1\r\nConnection: close\r\n\r\n");
	delay(10);

	while(client.available()){
		String line = client.readStringUntil('\r');
	}

	
}

void setState(int newstate) {
	if(newstate) {
		digitalWrite(13, LOW);
		digitalWrite(14, HIGH);
		sendGET(srv_host, path_green);
		state = newstate;		
	} else {
		digitalWrite(14, LOW);
		digitalWrite(13, HIGH);
		sendGET(srv_host, path_red);
		state = 0;
	}
}

void setup() {
	// put your setup code here, to run once:
	pinMode(12, INPUT_PULLUP);
	pinMode(13, OUTPUT);
	pinMode(14, OUTPUT);

	// Show that it's booting
	digitalWrite(13, HIGH);	
	digitalWrite(14, HIGH);

	Serial.begin(9600);
	Serial.println("Hello world!");
	WiFi.mode(WIFI_STA);
	WiFi.begin(wifi_ssid, wifi_pass);
	do {
		delay(500);
	} while(WiFi.status() != WL_CONNECTED);
	Serial.println("Connected");

	server.begin();
	Serial.println("Server started");
	setState(0);
	Serial.println(WiFi.localIP());
}



inline void toggleState() {
	setState(!state);
}

void sendError(WiFiClient &client) {
	client.print(
		"HTTP/1.1 400 Bad request\r\n"
		"Connection: close\r\n"
		"\r\n"
	);
}

void sendIndex(WiFiClient &client) {
	client.print(
		"HTTP/1.1 200 OK\r\n"
		"Connection: close\r\n"
		"\r\n"
		"<html>"
			"<head>"
				"<meta charset=\"utf-8\">"
				"<title>" TITLE "</title>"
			"</head>"
			"<body style=\"font-size: 300%;\">"
				"<br>"
				"<a href=\"/g\">"
					"<button style=\"width: 100%; height: 400px; font-size: 300%\">"
	);
	if(state) {
		client.print(
						SWITCH_TO " \"" GREEN_STATE "\""
		);
	} else {
		client.print(
						SWITCH_TO " \"" RED_STATE "\""
		);
	}
	client.print(
					"</button>"
				"</a><br>"
				"<br>"
				"<br>"
				CURRENT_STATE " "
	);

	if(state)
		client.print("<span style=\"color: red;\">" RED_STATE "</span>");
	else
		client.print("<span style=\"color: green;\">" GREEN_STATE "</span>");
	client.print(
				"<br><br><br><br>"
				"<a href=\"/d\">"
					"<button style=\"width: 300px; height: 100px; font-size: 120%;\">"
						SHUTDOWN
					"</button>"
				"</a>"
			"</body>"
		 "</html>"
	);
}

void sendOK(WiFiClient &client) {
	client.print(
		"HTTP/1.1 302 Found\r\n"
		"Location: /\r\n"
		"Connection: close\r\n"
		"\r\n"
	);
}

void sendOff(WiFiClient &client) {
	client.print(
		"HTTP/1.1 200 OK\r\n"
		"Connection: close\r\n"
		"\r\n"
		"<html>"
			"<head>"
				"<meta charset=\"utf-8\">"
				"<title>" TITLE "</title>"
			"</head>"
			"<body>"
				"<h1>" TURNED_OFF "</h1>"
			"</body>"
		"</html>"
	);
}

void powerDown() {
	digitalWrite(13, LOW);
	digitalWrite(14, LOW);

	sendGET(srv_host, path_off);
	delay(1000);
	ESP.deepSleep(0); // sleep forever
}

void checkButton() {
	if(digitalRead(12) == LOW) {
		delay(500);
		if(digitalRead(12) == LOW) {
			powerDown();
		} else {
			toggleState();
		}
		while(digitalRead(12) == LOW) {}
	}
}

void checkWiFi() {
	WiFiClient client = server.available();
	if (client) {
		static const char prefix[] = "GET /";
		int readstate = 0;
		while (client.connected()) {
			if (client.available()) {
				char c = client.read();
				if(readstate < sizeof(prefix) - 1) {
					if(c != prefix[readstate]) {
						sendError(client);
						client.flush();
						break;
					}

					++readstate;
				} else {
					if(readstate == sizeof(prefix) - 1) {
						switch(c) {
							case ' ':
								sendIndex(client);
								break;
							case 'r':
								setState(1);
								sendOK(client);
								break;
							case 'g':
								setState(0);
								sendOK(client);
								break;
							case 'd':
								sendOff(client);
								client.flush();
								client.stop();
								client.flush();
								powerDown();								
							default:
								sendError(client);
						}
						client.flush();
						return;
					}
				}
			}			
		}
	}
}


void loop() {
	checkButton();
	checkWiFi();
}
