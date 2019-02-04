//#include <GSMSim.h>
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

//#define RX  5
//#define TX  4
//#define RESET 12
//#define LED 2



int numberOfInterrupts = 0;
void handleInterrupt();
TaskController taskController = TaskController();

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
	Alarm._addClient(new AlarmClient("+380500784234",true,true));
	Alarm._addClient(new AlarmClient("+380950950102",true,false));
	//Alarm._addClient(new AlarmClient("0500784076"));
	BATTERY = new BatteryClass();
	BATTERY->onDischaged([](int charge) {
		String str = "Заряд батареи низкий: " + String(charge) + "%";
		//Alarm.textAll(str);
		//if (GsmModem.sendSMS("+380500784234", str.c_str())){
		//	BATTERY->setSMS(true);		
		//}
	});
	
	GsmModem.start();	
	
	server.setup();
	taskController.add(&taskConnectWiFi);
	taskController.add(BATTERY);
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
		}else if (str.startsWith(F("NO CARRIER"))){
			Alarm._msgDTMF = "";
			count_ring = 0;
		}else{
			if (str.indexOf(F("SMS ready"))!=-1){
				digitalWrite(DEFAULT_LED_PIN, HIGH);	
			}
			str = "";
		}
		
	}	
}
