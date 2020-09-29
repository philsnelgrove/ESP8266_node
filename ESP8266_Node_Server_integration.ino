// new improved version, based on the original version by mikdrad
// now with more comments!!
//
// include the library for the wifi adapter
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
//
// include library for WiFi Kit 8 board by Heltec
// board info        - https://heltec.org/project/wifi-kit-8/
// board manager URL - http://resource.heltec.cn/download/package_heltec_esp8266_index.json
// code library URL  - https://github.com/HelTecAutomation/Heltec_ESP8266

#include "heltec.h" // alias for `#include "SSD1306Wire.h"`


// "ifndef" is a neat thing, it's like a
// system-level "if" that ends at the
// "endif" below
#ifndef STASSID
// this is where you define your wifi name and password
// comment the ones you don't need and add your own
//
// Bear's place
// #define STASSID "Radio Gnome"
// #define STAPSK  "wakkawakkawakka"
//
// Phil's place
// #define STASSID "WillowNet Wi-Fi Network"
// #define STAPSK  "titpw4WN!"
//
#define STASSID "NEW_HOME"
#define STAPSK  "Stephanie88"
#endif

// transfer the system constant values
// to ones that have a current context
//
// we do this by creating a pointer
// (that's what the * is for)
// that points at the memory location
// that contains the value of the
// system constant.  In this way we
// pass around an address, and not a value
const char *ssid = STASSID;
const char *password = STAPSK;

// set this to the IP of the server
//
// const char *host = "10.0.0.29";
// const char *host = "192.168.1.26";
const char *host = "192.168.0.150";

// whatever port you like; I put
// the server on 9999 and the nodes
// on 9998
const uint16_t port = 9999;

// the number of milliseconds to pause
// between server polling, this is 5 mins
int max_count = 30000;
int count = max_count;

// create the node socket on the port
// numbered 1 less than the server - in
// this case 9998
WiFiServer wifiServer(port - 1);

// initialize led_status to OFF;
  int led_status = false;
  String led_prefix = "LED Status,";
  String led_message = led_prefix + led_status;

//initialize text data for display_text
  String data = "Hello, World!";
  
 
// the setup function is always
// run first, once, when the device
// is activated
void setup()
{

  // set pinmode for the builtin LED as an Output (13 on most boards, this should accomodate variations)
  // and set it OFF
  pinMode(LED_BUILTIN, OUTPUT);  //set the built-in LED as OUPUT
  
	// start a serial connection for
	// sending telemetry
	Serial.begin(115200);

  // initialize oled display on WiFi Kit 8
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Serial Enable*/);
  Heltec.display->init();
  Heltec.display->flipScreenVertically();
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(0,0,"Hello?");
  Heltec.display->display();


	// this is how we use the serial
	// port to display info on the
	// serial monitor durnig runtime
	Serial.println();
	Serial.println();
	Serial.print("Connecting to ");
	Serial.println(ssid);
	// set the wifi adapter into
	// non-node mode, to be solely
	// client-server
	WiFi.mode(WIFI_STA);
	// connect to wifi
	WiFi.begin(ssid, password);

	// this 'while' loop will loop as
	// long as the "WiFi" connector
	// object reports a non-connected
	// status
	while (WiFi.status() != WL_CONNECTED)
	{
		// for each loop, wait a half second
		// print a dot and check again
		delay(500);
		Serial.print(".");
	}
	// inform the developer
	Serial.println("");
	Serial.println("WiFi connected");
	Serial.println("IP address: ");
	Serial.println(WiFi.localIP());

	request_registration();
	// once we are connected to wifi,
	// start listening on the socket
	wifiServer.begin();
}

void process_command(String command_string)
{
	// for debugging, print it out
	Serial.print(command_string);
	// create a JSON deserialization buffer
	StaticJsonDocument<256> doc;
	// deserialize the 'command_string'
	// into the JSON object
	// this returns an error object, or not
	DeserializationError err = deserializeJson(doc, command_string);
	// if the error object exists, we failed...
	// find out why and quit
	if(err)
	{
		Serial.print("Error:");
		Serial.println(err.c_str());
		return;
	}
	// assuming there was no error object,
	// pull the items from the JSON object
	const char* command = doc["command"];
	const char* data = doc["data"];
	// we now have the 'command' issued to us,
	// and the 'data' passed as (a) parameter(s).
	// our 'data' object will also be an object
	// with whatever parameters were put on it
	// when it was formed on the server.
	// there is no error correction with this,
	// it is assumed that the correct parameters
	// will be there for the command being called.
	if (command == "toggle_led")
	{
		toggle_led();
	}
  if (command == "display_text")
  {
    display_text(data);
  }

	return;
}

void toggle_led()
{
  // toggle the LED
  Serial.println("Toggling LED");
  switch (led_status) {

      case true: // LED is on and I want it off
        Serial.println("Changing from ON to OFF");
        digitalWrite(LED_BUILTIN, LOW);
        led_status = false;
        break;
      case false: // LED is off and I want it on
        Serial.println("Changing from OFF to ON");
        digitalWrite(LED_BUILTIN, HIGH);
        led_status = true;
        break;
  }
  return;
}
 void display_text(String data)
 {
  // display specified text on oled screen, code specific to WiFi Kit 8 board
  Serial.print("Display Text,");
  Serial.println(data);
  Heltec.display->drawString(0,0,data);
  Heltec.display->display();

 }

int issue_report(String data)
{
	// note: '\r' is carriage return
	Serial.print("\rconnecting to ");
	Serial.print(host);
	Serial.print(':');
	Serial.println(port);
	// reset the timer count to 0
	count = 0;
	// create a server connection
	WiFiClient client1;
	// inform us if the connection fails
	// and bail on this loop
	if (!client1.connect(host, port))
	{
		Serial.println("connection failed");
		delay(5000);
		// 1 = connection failed
		return 1;
	}
	// inform us of success
	Serial.println("sending data to server");
	// assuming that we are not only connected,
	// but our connection is listened to
	if (client1.connected())
	{
		// send it a message
		DynamicJsonDocument jsonBuffer(128);
		char output[128];
		jsonBuffer["mac"] = WiFi.macAddress();
		jsonBuffer["operation"] = "report";
		// return led_message as data
		jsonBuffer["data"] = led_message;
		serializeJson(jsonBuffer, output);
		client1.println(output);
	}

	// now we prepare to wait for an answer
	unsigned long timeout = millis();
	// loop while the client is not yet]
	// communicating with us - when we recieve
	// an actual message, the 'available' method
	// will return a non-zero value
	while (client1.available() == 0)
	{
		// if we have waited longer than
		// 5 seconds to recieve a response,
		// signal a timeout and bail on this loop
		if (millis() - timeout > 5000)
		{
			Serial.println(">>> Client Timeout !");
			client1.stop();
			delay(60000);
			// status 2 = timeout
			return 2;
		}
	}
	// inform us of succ1ss
	Serial.println("receiving from remote server");
	// as long as we are receiving, display the contents
	// of the read buffer 1 character at a time
	while (client1.available())
	{
		char ch = static_cast<char>(client1.read());
		Serial.print(ch);
	}

	// Close the connection
	Serial.println("closing connection");
	Serial.println("***");
	client1.stop();
	// 0 = success
	return 0;
}

int request_registration()
{
	// now that we are connected to wifi
	// and listening on our port, it's time
	// to attempt to connect and handshake
	// with the server
	WiFiClient client;
	if (!client.connect(host, port))
	{
		// on a failure, quit trying
		// a device reset will be required
		// to try again
		Serial.println("connection failed");
		delay(5000);
		return 1;
	}
	// if we connected to the server alright,
	// we send the handshake message
	if (client.connected())
	{
		DynamicJsonDocument jsonBuffer(128);
		char output[128];
		jsonBuffer["mac"] = WiFi.macAddress();
		jsonBuffer["operation"] = "register_client";
		jsonBuffer["data"] = "null";
		serializeJson(jsonBuffer, output);
		client.println(output);
	}
	// then we process the response from the server as
	// part 2 of the handshake
	while (client.available())
	{
		// TODO: check for an actual response (TBD)
		char ch = static_cast<char>(client.read());
		Serial.print(ch);
	}
	// be done with this communication but
	// leave all connections active
	client.stop();
	return 0;
}

// this loop will repeat until the device
// dies, on an arduino
void loop()
{
	// this is a server push with a data message
	// only do these things if we've hit 5 mins
	if (count > max_count - 1)
	{
		int report_status = issue_report("periodic report with data");
	}
	// increase the count by 1
	count++;
	// ensure that this loop consumes a predictable 1ms
	delay(1);

	// setup for the listen "hit" for
	// this loop
	// this definition will only create a
	// meaningful object if there is a
	// connection active on the listening socket
	WiFiClient client2 = wifiServer.available();

	// therefore, if client2 is a meaningful
	// object, process the inbound content
	if (client2)
	{
		// create the output string...
		// it must be in this scope (IE
		// outside loops or conditions)
		String command_string;
		// loops as long as the connection persists
		while (client2.connected())
		{
			// loop as long as we are recieving data
			// adding the characters to an array
			while (client2.available() > 0)
			{
				// process the next character
				// in the 'read' buffer
				char c = client2.read();
				// add the character to
				// the "next spot" in the output string
				command_string += c;
			}
			// sync this operation with the 5 minute count
			count += 10;
			// ensure that the correct amount of time has passed
			// to complete the message reception and to sync
			// with the 5 minute count
			delay(10);
		}
    // override server command_string to toggle every loop for testing purposes
    Serial.println(led_message);
 
		// call the "process command" function with
		// the concatenated output from the connection,
		// aka the JSON command message
		

	  process_command(command_string);
  
		// close the current connection on the socket
		client2.stop();
		Serial.println("Client disconnected");
    
	}
   
}
