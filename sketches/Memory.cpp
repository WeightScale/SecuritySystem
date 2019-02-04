#include "Memory.h"
#include "Battery.h"

void /*ICACHE_RAM_ATTR*/ MemoryClass::init() {
	SPIFFS.begin();		
	begin(sizeof(MyEEPROMStruct));
	if (percentUsed() >= 0) {
		get(0, eeprom);
	}
	else {
		doDefault();	
	}
}

bool MemoryClass::save() {
	put(0, eeprom);
	return commit();
}

bool MemoryClass::doDefault() {
	String u = "";
	String ap = "SecuritySystem";
	String host = "sdb.net.ua";
	String l = "admin";
	for (int i = 0; i < USER_MAX; i++){
		u.toCharArray(eeprom.users[i].name, u.length() + 1);
		u.toCharArray(eeprom.users[i].phone, u.length() + 1);
		eeprom.users[i].send = false;	
	}
	ap.toCharArray(eeprom.settings.apSSID, ap.length() + 1);
	host.toCharArray(eeprom.settings.hostUrl, host.length() + 1);
	l.toCharArray(eeprom.settings.login, l.length() + 1);
	l.toCharArray(eeprom.settings.password, l.length() + 1);
	eeprom.settings.hostPin = 0;
	eeprom.settings.bat_max = MAX_CHG;
	eeprom.settings.bat_min = MIN_CHG;	
	return save();
}

MemoryClass Memory;