#include "ServerUtilities.h"

ServerUtilities::ServerUtilities(){

}

void ServerUtilities::getSSIDPASS(){
    for (int i = 0; i < 32; ++i)
    {
        this->esid += char(EEPROM.read(i));
    }

    for (int i = 32; i < 96; ++i)
    {
        this->epass += char(EEPROM.read(i));
    }
}

bool ServerUtilities::testWifi(void)
{
  int c = 0;
  Serial.println("Waiting for Wifi to connect");
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED)
    {
      return true;
    }
    delay(500);
    Serial.print("*");
    c++;
  }
  Serial.println("");
  Serial.println("Connect timed out, opening AP");
  return false;
}

void ServerUtilities::launchWeb()
{
  Serial.println("");
  if (WiFi.status() == WL_CONNECTED)
    Serial.println("WiFi connected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  createWebServer();
  // Start the server
  server.begin();
  Serial.println("Server started");
}

void ServerUtilities::setupAP(void)
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(10);
    }
  }
  Serial.println("");
  st = "<ol>";
  for (int i = 0; i < n; ++i)
  {
    // Print SSID and RSSI for each network found
    st += "<li>";
    st += WiFi.SSID(i);
    st += " (";
    st += WiFi.RSSI(i);

    st += ")";
    st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
    st += "</li>";
  }
  st += "</ol>";
  delay(100);
  WiFi.softAP("CBS", "");
  Serial.println("Initializing_softap_for_wifi credentials_modification");
  launchWeb();
  Serial.println("over");
}

void ServerUtilities::createWebServer()
{   
    
    {
        server.on("/", [this]() {
        IPAddress ip = WiFi.softAPIP();
        String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
        this->content = "<!DOCTYPE HTML>\r\n<html>Welcome to Wifi Credentials Update page";
        this->content += "<form action=\"/scan\" method=\"POST\"><input type=\"submit\" value=\"scan\"></form>";
        this->content += ipStr;
        this->content += "<p>";
        this->content += st;
        this->content += "</p><form method='get' action='setting'><label>SSID: </label><input name='ssid' length=32><input name='pass' length=64><input type='submit'></form>";
        this->content += "</html>";
        server.send(200, "text/html", this->content);
    });
    server.on("/scan", [this]() {
        //setupAP();
        IPAddress ip = WiFi.softAPIP();
        String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);

        this->content = "<!DOCTYPE HTML>\r\n<html>go back";
        server.send(200, "text/html", this->content);
    });

    server.on("/setting", [this]() {
        String qsid = server.arg("ssid");
        String qpass = server.arg("pass");
        if (qsid.length() > 0 && qpass.length() > 0) {
        Serial.println("clearing eeprom");
        for (int i = 0; i < 96; ++i) {
            EEPROM.write(i, 0);
        }
        Serial.println(qsid);
        Serial.println("");
        Serial.println(qpass);
        Serial.println("");

        Serial.println("writing eeprom ssid:");
        for (int i = 0; i < qsid.length(); ++i)
        {
            EEPROM.write(i, qsid[i]);

        }
        Serial.println("writing eeprom pass:");
        for (int i = 0; i < qpass.length(); ++i)
        {
            EEPROM.write(32 + i, qpass[i]);
        }
        EEPROM.commit();

        this->content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
        statusCode = 200; 
        
        ESP.reset();
        } else {
        this->content = "{\"Error\":\"404 not found\"}";
        statusCode = 404;
        Serial.println("Sending 404");
        }
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.send(statusCode, "application/json", this->content);

    });
  }
}

