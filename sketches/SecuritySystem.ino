#include <ESP8266NetBIOS.h>
#include "TaskController.h"
#include "Memory.h"
#include "Battery.h"
#include "GsmModemClass.h"
#include "Server.h"
#include "Alarm.h"

//Comment out the definition below if you don't want to use the ESP8266 gdb stub.
//#define ESP8266_USE_GDB_STUB

#ifdef ESP8266_USE_GDB_STUB
	#include <GDBStub.h>
	extern "C" int gdbstub_init();
	extern "C" int gdbstub_do_break();
#endif

int numberOfInterrupts = 0;
void handleInterrupt();
TaskController taskController = TaskController();
Task /*ICACHE_RAM_ATTR*/ taskInfo([]() {
	if (BATTERY->charge() < 50)
	{
		taskSleepModem.resume();
	}else{
		if(GsmModem.disableSleep())
			taskSleepModem.resume();
	}		
}, 60000);

void setup(){
#ifdef ESP8266_USE_GDB_STUB
	Serial.begin(921600);
    gdbstub_init();
    gdbstub_do_break();
#else
	//Serial.begin(115200);
	//Serial.println("START SIM800L");
#endif
	Memory.init();
	Alarm.begin();
	//Alarm._addClient(new AlarmClient("0500784076"));
	BATTERY = new BatteryClass();
	BATTERY->onDischaged([](int charge) {
		if (!Alarm.sleep()){
			taskSleepModem.resume();	
		}			
	});
	
	GsmModem.start();	
	
	server.setup();
	taskController.add(&taskConnectWiFi);
	taskController.add(BATTERY);
	taskController.add(&taskInfo);
	taskController.add(&taskSleepModem);
}
String str = "";
byte count_ring = 0;
void /*ICACHE_RAM_ATTR*/ loop() {
	taskController.run();
	Alarm.handle();		
	if (GsmModem.available()){
		str = GsmModem._readSerial();
		if (str.indexOf(F("RING")) != -1) {
			if ((count_ring++) > 1){
				str = str.substring(str.indexOf(F("+CLIP:")) + 6, str.indexOf(","));
				str.replace("\"", "");
				str.trim();
				Alarm.fetchCall(str);	
				count_ring = 0;
			}			
		}
		else if (str.indexOf(F("+DTMF:")) != -1) {
			str.trim();
			Alarm.parseDTMF(str.substring(7, 8));			
		}else if (str.indexOf(F("+CMTI:")) != -1){
			//str = sendATCommand("AT+CMGL=\"REC UNREAD\",1", true);  // Отправляем запрос чтения непрочитанных сообщений
			int i = str.substring(str.indexOf(F("SM")) + 4, str.lastIndexOf("\r")).toInt();
			Alarm.fetchMessage(i);			
			
		}else if (str.indexOf(F("UNDER")) != -1){
			Alarm.textAll(str);
			taskSleepModem.resume();
		}else if (str.startsWith(F("NO CARRIER"))){
			Alarm._msgDTMF = "";
			count_ring = 0;
		}else{
			if (str.indexOf(F("Call Ready")) != -1) {
				//taskController.add(&taskSleepModem);					
			}
			str = "";
		}
		
	}	
}
