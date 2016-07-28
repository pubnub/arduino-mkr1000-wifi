#include <SPI.h>
#include <WiFi101.h>
#include <PubNub.h>

char apssid[] = "MKR1000AP";
int status = WL_IDLE_STATUS;
WiFiServer server(80);
String HTTP_req;
boolean readingNetwork = false;
boolean readingPassword = false;
boolean readingPubkey = false;
boolean readingSubkey = false;
boolean readingChannel = false;
String password = "";
String network = "";
String pubkey = "";
String subkey = "";
String channel = "";
boolean needCredentials = true;
boolean needWiFi = false;
boolean connectPubNub = false;

char *strToChar(String str) {
  int len = str.length() + 1;
  char c[len];
  str.toCharArray(c, len);
  return c;
}

void setup() {
  Serial.begin(9600);
  Serial.println("Access Point Web Server");
  
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    while (true);
  }
  
  Serial.print("Creating access point named: ");
  Serial.println(apssid);
  
  if (WiFi.beginAP(apssid) != WL_CONNECTED) {
    Serial.println("Creating access point failed");
    while (true);
  }

  delay(1000);
  server.begin();
  printAPStatus();  
}

void loop() {
  if (needCredentials) {
    getCredentials();
  }
  if (needWiFi) {
    getWiFi();
  }
  if (connectPubNub) {
    getPubNub();
  }
}


void getCredentials() {
  WiFiClient client = server.available();
  if (client) {                            
    Serial.println("new client");           
    String currentLine = "";               
    while (client.connected()) {            
      if (client.available()) {            
        char c = client.read();           
        Serial.print(c);
        if (c=='?') readingNetwork = true;
        if (readingNetwork) {
          if (c=='!') {
            readingPassword = true;
            readingNetwork = false;
          }
          else if (c!='?') {
            network += c;
          }
        }
        if (readingPassword) {
          if (c==',') {
            readingPubkey = true;
            readingPassword = false; 
          }
          else if (c!='!') {
            password += c;
          }
        }
        if (readingPubkey) {
          if (c=='*') {
            readingSubkey = true;
            readingPubkey = false; 
          }
          else if (c!=',') {
            pubkey += c;
          }
        }
        if (readingSubkey) {
          if (c=='!') {
            readingChannel = true;
            readingSubkey = false; 
          }
          else if (c!='*') {
            subkey += c;
          }
        }
        if (readingChannel) {
          if (c==',') {
            Serial.println();
            Serial.print("Network Name: ");
            Serial.println(network);
            Serial.print("Password: ");
            Serial.println(password);
            Serial.print("Publish Key: ");
            Serial.println(pubkey);
            Serial.print("Subscribe Key: ");
            Serial.println(subkey);
            Serial.print("Channel: ");
            Serial.println(channel);
            Serial.println();
            client.stop();
            WiFi.end();
            readingChannel = false;
            needCredentials = false;
            needWiFi = true;  
          }
          else if (c!='!') {
            channel += c;
          }
        }
        if (c == '\n') {   
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            client.println("<html>");
            client.println("<head>");
            client.println("<style type=\"text/css\"> body {font-family: sans-serif; margin:50px; padding:20px; line-height: 250% } </style>");
            client.println("<title>Arduino Setup</title>");
            client.println("</head>");
            client.println("<body>");

            client.println("<h2>WIFI CREDENTIALS</h2>");
            client.print("NETWORK NAME: ");
            client.print("<input id=\"network\"/><br>");
            client.print("PASSWORD: ");
            client.print("<input id=\"password\"/><br>");


            client.println("<h2>PUBNUB CREDENTIALS</h2>");
            client.print("PUBLISH KEY: ");
            client.print("<input id=\"subkey\"/><br>");
            client.print("SUBSCRIBE KEY: ");
            client.print("<input id=\"pubkey\"/><br>");
            client.print("CHANNEL: ");
            client.print("<input id=\"channel\"/><br>");
            client.print("<br>");
            
            client.print("<button type=\"button\" onclick=\"SendText()\">Enter</button>");
            client.println("</body>");
            client.println("<script>");
            client.println("var network = document.querySelector('#network');");
            client.println("var password = document.querySelector('#password');");
            client.println("var pubkey = document.querySelector('#pubkey');");
            client.println("var subkey = document.querySelector('#subkey');");
            client.println("var channel = document.querySelector('#channel');");
            
            client.println("function SendText() {");
            client.println("nocache=\"&nocache=\" + Math.random() * 1000000;");
            client.println("var request =new XMLHttpRequest();");
            client.println("netText = \"&txt=\" + \"?\" + network.value + \"!\" + password.value + \",\" + pubkey.value + \"*\" + subkey.value + \"!\" + channel.value + \",&end=end\";");
            client.println("request.open(\"GET\", \"ajax_inputs\" + netText + nocache, true);");
            client.println("request.send(null)");
            client.println("network.value=''");
            client.println("password.value=''");
            client.println("pubkey.value=''");
            client.println("subkey.value=''");
            client.println("channel.value=''}");
            client.println("</script>");
            client.println("</html>");
            client.println();
            break;
          }
          else { 
            currentLine = "";
          }
        }
        else if (c != '\r') { 
          currentLine += c;
        }
      }
    }
    client.stop();
    Serial.println("client disconnected");
    Serial.println();
  }
}

void getWiFi () {
  if (network == "" or password == "") {
        Serial.println("Invalid WiFi credentials");
        while (true);
      }
  else if (subkey == "" or pubkey == "" or channel == "") {
        Serial.println("Invalid PubNub credentials");
  }
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(network);
    WiFi.begin(strToChar(network), strToChar(password));
    delay(10000);
    }
  Serial.println("WiFi connection successful");
  printWiFiStatus();
  needWiFi = false;
  connectPubNub = true;
  delay(1000);
}

void getPubNub () {
  PubNub.begin(strToChar(pubkey), strToChar(subkey));
  Serial.println("PubNub connection established");
  WiFiClient *client = PubNub.publish(strToChar(channel), "{\"Arduino\":\"Hello World!\"}");
  connectPubNub = false;
}

void printWiFiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  Serial.print("signal strength (RSSI):");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
}

void printAPStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  Serial.print("signal strength (RSSI):");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
  Serial.print("To connect, open a browser to http://");
  Serial.println(ip);
}
