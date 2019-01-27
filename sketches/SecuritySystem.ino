//#include <GSMSim.h>
#include "TaskController.h"
#include "Memory.h"
#include "Battery.h"
#include "GsmModemClass.h"
#include "Server.h"
#include "Alarm.h"

//Comment out the definition below if you don't want to use the ESP8266 gdb stub.
#define ESP8266_USE_GDB_STUB

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
	Serial.begin(115200);
	Serial.println("START SIM800L");
#endif	
	Memory.init();
	BATTERY = new BatteryClass();
	BATTERY->callDischaged([](int charge) {
		String str = "Заряд батареи низкий: " + String(charge) + "%";
		if (GsmModem.sendSMS("+380500784234", str.c_str())){
			BATTERY->setSMS(true);		
		}
	});
	taskController.add(BATTERY);
	
	//pinMode(LED_BUILTIN, OUTPUT);
	
	
	GsmModem.start();	
	
	String str = GsmModem.moduleModel();
	str = GsmModem.signalQuality();	
	//bool f = GsmModem.enterSleepMode();
	//delay(10000);
	//f = GsmModem.disableSleep();
	server.setup();
}

void /*ICACHE_RAM_ATTR*/ loop() {
	taskController.run();
	Alarm.handle();
		
}

