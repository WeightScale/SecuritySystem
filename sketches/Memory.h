#pragma once

#include <Arduino.h>
#include <ESP_EEPROM.h>

#define USER_MAX 5

typedef struct {
	char apSSID[16];
	char hostUrl[0xff];
	int hostPin;
	char login[16];
	char password[16];
	int bat_max;
	int bat_min;
} settings_t;

typedef struct {
	
		char name[16];
		char phone[14];
		bool send = false;
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