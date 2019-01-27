#include "Alarm.h"
#include "GsmModemClass.h"

AlarmClass Alarm;

AlarmClass::AlarmClass(void){
	pinMode(interruptPin, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, RISING);	
}

void AlarmClass::handle(){
	if (!isSafe())
		return;
	if (isInterrupt()){
			
	}
	if (digitalRead(interruptPin)) {
		
		digitalWrite(2, LOW);
		GsmModem.flush();
		if (GsmModem.sendSMS("+380500784234", "test"))
			interruptCounter--;
		digitalWrite(2, HIGH);
	}
}

void ICACHE_RAM_ATTR handleInterrupt() {
	Alarm.interrupt();
}