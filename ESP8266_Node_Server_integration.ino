// include the library for the wifi adapter
#include <ESP8266WiFi.h>

// "ifndef" is a neat thing, it's like a
// system-level "if" that ends at the
// "endif" below
#ifndef STASSID
// this is where you define your wifi name and password
// comment the ones you don't need and add your own
//
// Bear's place
#define STASSID "Radio Gnome"
#define STAPSK  "wakkawakkawakka"
//
// Phil's place
// #define STASSID "WillowNet Wi-Fi Network"
// #define STAPSK  "titpw4WN!
#endif

// transfer the system constant values
// to ones that have a current context
const char *ssid = STASSID;
const char *password = STAPSK;

// set this to the IP of the server
//
const char *host = "10.0.0.29";
// const char* host = "192.168.1.24";

// whatever port you like; I put
// the server on 9999 and the nodes
// on 9998
const uint16_t port = 9999;

// the number of milliseconds to pause
// between server polling, this is 5 mins
int count = 300000;

// create the node socket on the port
// numbered 1 less than the server - in
// this case 9998
WiFiServer wifiServer(port - 1);

// the setup function is always
// run first, once, when the device
// is activated
void setup()
{
	// start a serial connection for
	// sending telemetry
	Serial.begin(115200);

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

	// once we are connected to wifi,
	// start listening on the socket
	wifiServer.begin();

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
		return;
	}
	// if we connected to the server alright,
	// we send the handshake message
	if (client.connected())
	{
		String handshake_header = "IP:";
		IPAddress ip = WiFi.localIP(); // @suppress("Invalid arguments")
		String handshake_ip = String(ip[0]);
		for (byte octet = 1; octet < 4; ++octet)
		{
			handshake_ip += '.' + String(ip[octet]);
		}

		String handshake = handshake_header + handshake_ip;
		client.println(handshake);
	}
	// then we process the response from the server as
	// part 2 of the handshake
	while (client.available())
	{
		char ch = static_cast<char>(client.read());
		Serial.print(ch);
	}
	// be done with this communication but
	// leave all connections active
	client.stop();
}

// this loop will repeat until the device
// dies, on an arduino
void loop()
{
	// this is a server push with a data message
	// only do these things if we've hit 5 mins
	if (count > 299999)
	{
		Serial.print("connecting to ");
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
			return;
		}
		// inform us of success
		Serial.println("sending data to server");
		// assuming that we are not only connected,
		// but our connection is listened to
		if (client1.connected())
		{
			// send it a message
			client1.println("hello from ESP8266");
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
				return;
			}
		}
		// inform us of success
		Serial.println("receiving from remote server");
		// as long as we are receiving, display the contents
		// of the read buffer 1 character at a time
		while (client1.available())
		{
			char ch = static_cast<char>(client1.read());
			Serial.print(ch);
		}

		// Close the connection
		Serial.println();
		Serial.println("closing connection");
		client1.stop();
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
		// loops as long as the connection persists
		while (client2.connected())
		{
			// loop as long as we are recieving data
			while (client2.available() > 0)
			{
				// process the next character
				// in the 'read' buffer and
				// print it out
				char c = client2.read();
				Serial.write(c);
			}
			// sync this operation with the 5 minute count
			count += 10;
			// ensure that the correct amount of time has passed
			// to complete the message reception and to sync
			// with the 5 minute count
			delay(10);
		}
		// close the current connection on the socket
		client2.stop();
		Serial.println("Client disconnected");
	}
}
