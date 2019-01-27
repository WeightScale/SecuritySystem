#pragma once
#include <Arduino.h>
#define DEFAULT_INT_PIN 13

class AlarmClass{
private:
	bool _isInterrupt = false;
	bool _isSafe = true;
	volatile byte interruptCounter = 0;
	const byte interruptPin = DEFAULT_INT_PIN;
public:
	AlarmClass(void);
	bool isInterrupt(){return _isInterrupt;};
	void interrupt(){_isInterrupt = true;};
	bool isSafe(){return _isSafe;};
	void setSafe(bool safe){_isSafe = safe;};
	void handle();
	
};

extern AlarmClass Alarm;
extern void handleInterrupt();