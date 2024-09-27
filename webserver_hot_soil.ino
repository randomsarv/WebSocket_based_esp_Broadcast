#include <Arduino.h>
#include <WiFi.h>                  // ESP32 WiFi library
#include <WiFiMulti.h>             // ESP32 WiFiMulti library
#include <WebSocketsServer.h>      // WebSockets library (no change needed)
#include <WebServer.h>             // ESP32 Web server library
                 // Hash library (no change needed)

String old_value, value;

// Use WiFiMulti for connecting to multiple WiFi networks
WiFiMulti    WiFiMulti;
WebServer    server(80);              // ESP32 Web server on port 80
WebSocketsServer    webSocket = WebSocketsServer(81);  // WebSockets server on port 81

// HTML template for displaying joystick value
char html_template[] PROGMEM = R"=====( 
<html lang="en">
   <head>
      <meta charset="utf-8">
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <title>Joystick Value Display</title>
      <script>
        socket = new WebSocket("ws:/" + "/" + location.host + ":81");
        socket.onopen = function(e) {  console.log("[socket] Connected"); };
        socket.onerror = function(e) {  console.log("[socket] Error"); };
        socket.onmessage = function(e) {  
            console.log("[socket] " + e.data);
            document.getElementById("joystick_value").innerHTML = e.data;
        };
      </script>
   </head>
   <body style="max-width:400px;margin: auto;font-family:Arial, Helvetica, sans-serif;text-align:center">
      <div><h1><br />Joystick Value</h1></div>
      <div><p id="joystick_value" style="font-size:100px;margin:0"></p></div>
   </body>
</html>
)=====";

// Handle WebSocket events (connected, disconnected, messages)
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;

    case WStype_CONNECTED: {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        // Send initial message to client
        webSocket.sendTXT(num, "0");
      }
      break;

    case WStype_TEXT:
      Serial.printf("[%u] Received Text: %s\n", num, payload);
      // Broadcast received message to all clients
      // webSocket.broadcastTXT(payload);
      break;
      
    case WStype_BIN:
      Serial.printf("[%u] Received binary length: %u\n", num, length);
      // Send binary message to client
      // webSocket.sendBIN(num, payload, length);
      break;
  }

}

void setup() {

  Serial.begin(115200);  // Start serial for debugging
  delay(1000);

  // Connect to WiFi
  WiFiMulti.addAP("BSPROGRAMMER", "12345678");  // Add your SSID and password here

  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(100);
  }

  Serial.println(WiFi.localIP());  // Print local IP address when connected

  // Start WebSocket server and assign event handler
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // Set up HTTP server routes
  server.on("/", handleMain);
  server.onNotFound(handleNotFound);
  server.begin();

}

void handleMain() {
  server.send_P(200, "text/html", html_template );  // Serve the HTML page
}

void handleNotFound() {
  server.send(404, "text/html", "<html><body><p>404 Error: Page Not Found</p></body></html>" );
}

void loop() {
  
  webSocket.loop();   // Handle WebSocket communication
  server.handleClient();  // Handle HTTP clients

  // Read joystick value on analog pin 25 (X-axis)
  value = String(analogRead(32));
  Serial.print(value);

  // Only send data if the value has changed
  if (value != old_value) {
    webSocket.broadcastTXT(value);  // Send the joystick X value to all clients
    old_value = value;  // Update the old value to the new one
  }
  
  delay(50);  // Short delay to prevent spamming updates
}
