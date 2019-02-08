#pragma once

#include <Arduino.h>
#include <ESP_EEPROM.h>

#define USER_MAX 5

typedef struct {
	bool autoIp;
	char lanIp[16];
	char gateway[16];
	char subnet[16];
	char wSSID[33];
	char wKey[33];
	char apSSID[16];
	char hostUrl[0xff];
	int hostPin;
	char login[16];
	char password[16];
	char login_alarm[16];
	char password_alarm[16];
	int bat_max;
	int bat_min;
} settings_t;

typedef struct {	
	char name[16];
	char phone[14];
	bool send = false;
	bool root = false;
}user_t;

class MemoryClass : protected EEPROMClass {
public:
	struct MyEEPROMStruct {
		user_t users[USER_MAX];
		settings_t settings;
	} eeprom;


public:
	void init();
	bool save();
	bool doDefault();
	
};



extern MemoryClass Memory;