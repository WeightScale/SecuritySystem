#include "Server.h"
#include <SPIFFSEditor.h>
#include "Memory.h"


ServerClass server(80);
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

ServerClass::ServerClass(uint16_t port)	: AsyncWebServer(port) {}

ServerClass::~ServerClass() {}

void ServerClass::setup() {
	_apssid = &Memory.eeprom.settings.apSSID[0];
	_httpAuth.wwwUsername = "sa";
	_httpAuth.wwwPassword = "343434";
	WiFi.persistent(false);	
	//WiFi.smartConfigDone();
	WiFi.mode(WIFI_AP);
	WiFi.hostname(_hostname);
	WiFi.softAPConfig(apIP, apIP, netMsk);
	WiFi.softAP(_apssid, SOFT_AP_PASSWORD);
	
	addHandler(new SPIFFSEditor(_httpAuth.wwwUsername.c_str(), _httpAuth.wwwPassword.c_str()));	
	addHandler(new HttpUpdaterClass("sa", "654321"));
};