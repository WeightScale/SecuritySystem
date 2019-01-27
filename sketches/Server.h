#pragma once
#include <Arduino.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

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
public:	
	ServerClass(uint16_t port);
	~ServerClass();
	void setup();
};

extern ServerClass server;
extern IPAddress apIP;
extern IPAddress netMsk;