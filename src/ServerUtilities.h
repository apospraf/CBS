#ifndef ServerUtilities_h
#define ServerUtilities_h

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

class ServerUtilities{

    public:
        ServerUtilities();
        void getSSIDPASS();
        void createWebServer();
        void setupAP();
        void launchWeb();
        bool testWifi(void);
        int i = 0;
        int statusCode;
        String st;
        String content;
        String esid;
        String epass;
        ESP8266WebServer server;
        WiFiClientSecure client;
        
};
#endif