#pragma once
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <functional>
#include "Task.h"
#include "Memory.h"

//#define DEBUG_BATTERY		/*Для теста*/

#define BATTERY_6V				6
#define BATTERY_4V				4
#define R1_KOM					470.0f
#define R2_KOM					75.0f
#define VREF					1.0f
#define ADC						1024.0f

#define CHANEL					A0

/*План питания от батареи*/
//#define PLAN_BATTERY			BATTERY_6V
#define PLAN_BATTERY			BATTERY_4V

#define CONVERT_V_TO_ADC(v)		(((v * (R2_KOM /(R1_KOM + R2_KOM)))*ADC)/VREF)

/*ADC = (Vin * 1024)/Vref  Vref = 1V*/
#if PLAN_BATTERY == BATTERY_6V		
#define MAX_CHG					CONVERT_V_TO_ADC(6.4)		/*Vin 6.4V*/
#define MIN_CHG					CONVERT_V_TO_ADC(5.5)		/*Vin 5.5V*/				
#elif PLAN_BATTERY == BATTERY_4V
#define MAX_CHG					CONVERT_V_TO_ADC(4.2)		/*Vin 4.2V*/
#define MIN_CHG					CONVERT_V_TO_ADC(3.5)		/*Vin 3.3V*/
#endif // PLAN_BATTERY == BATTERY_6V

#define CALCULATE_VIN(v)		((v/ADC)/(R2_KOM /(R1_KOM + R2_KOM)))


class BatteryClass : public Task {	
	typedef std::function<void(int)> BatteryHandleDischarged;
protected:
	BatteryHandleDischarged _handleDischarged;
	bool _isDischarged = false;
	bool _is_sms = false;
	unsigned int _charge;
	settings_t *_settings;
	int _max;	/*Значение ацп максимального заряд*/
	int _min;	/*Значение ацп минимального заряд*/
	int _get_adc(byte times = 1);	
public:
	BatteryClass();
	~BatteryClass() {};
	int fetchCharge();		
	void setCharge(unsigned int ch){_charge = ch; };
	unsigned int getCharge(){return _charge;};
	void setMax(int m){_max = m; };	
	void setMin(int m){_min = m; };	
	int getMax(){return _max;};
	int getMin(){return _min;};
	size_t doInfo(JsonObject& json);
	void handleBinfo(AsyncWebServerRequest *request);
	bool isDischarged(){return _isDischarged;};
	void onDischaged(BatteryHandleDischarged call){_handleDischarged = call;};
	void setSMS(bool sms)
	{
		_is_sms = sms;
	}
};

extern BatteryClass* BATTERY;