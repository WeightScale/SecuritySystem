#pragma once
#include <Arduino.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "Task.h"
#include "Memory.h"

const char netIndex[] PROGMEM = R"(<html lang='en'><meta name='viewport' content='width=device-width, initial-scale=1, maximum-scale=1'/><body><form method='POST'><input name='ssid'><br/><input type='password' name='key'><br/><input type='submit' value='ÑÎÕÐÀÍÈÒÜ'></form></body></html>)";

#define SOFT_AP_PASSWORD "12121212"

typedef struct {
	String wwwUsername;
	String wwwPassword;
} strHTTPAuth;


class ServerClass : public AsyncWebServer{
private:
	strHTTPAuth _httpAuth;
	String _hostname;
	char *_apssid;
	settings_t *_settings;
public:	
	ServerClass(uint16_t port);
	~ServerClass();
	void setup();
	bool checkAdminAuth(AsyncWebServerRequest * r);
	bool isAuthentified(AsyncWebServerRequest * request);
	char* getSSID(){return _settings->wSSID;};
	char* getPASS(){return _settings->wKey;};
	char* getApSSID() { return _settings->apSSID;};
	String& hostname() {return _hostname;};
};

extern void handleSettings(AsyncWebServerRequest * request);
void handleSettingsJson(AsyncWebServerRequest *request);
extern ServerClass server;
extern AsyncWebSocket ws;
extern IPAddress apIP;
extern IPAddress netMsk;
extern IPAddress lanIp; 			// 
extern IPAddress gateway;
extern Task taskConnectWiFi;