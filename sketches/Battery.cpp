#include "Battery.h"
#include "Memory.h"
#include "Server.h"

BatteryClass* BATTERY;

BatteryClass::BatteryClass() : Task(60000) {
	/* 20 Обновляем заряд батареи */
	onRun(std::bind(&BatteryClass::fetchCharge, this));
	_settings = &Memory.eeprom.settings;
	_max = _settings->bat_max;
	_min = _settings->bat_min;	
	fetchCharge();
#ifdef DEBUG_BATTERY
	_isDischarged = false;
#endif // DEBUG_BATTERY

}

int BatteryClass::fetchCharge() {
	_charge = _get_adc(1);
	_charge = constrain(_charge, _min, _max);
	_charge = map(_charge, _min, _max, 0, 100);
	_isDischarged = _charge <= 5;
	if (_isDischarged) {
		if (_handleDischarged)
			_handleDischarged(_charge);
		//ws.textAll("{\"cmd\":\"dchg\"}");
	}
	return _charge;
}

int BatteryClass::_get_adc(byte times) {
	long sum = 0;
#ifdef DEBUG_BATTERY
	for (byte i = 0; i < times; i++) {
		sum += random(ADC);
	}	
#else
	for (byte i = 0; i < times; i++) {
		sum += analogRead(A0);
	}
#endif // DEBUG_BATTERY	
	return times == 0 ? sum : sum / times;	
}

size_t BatteryClass::doInfo(JsonObject& json){
	json["id_min"] = CALCULATE_VIN(_min);
	json["id_max"] = CALCULATE_VIN(_max);
	json["id_cvl"] = CALCULATE_VIN(_get_adc(1));
	return json.measureLength();	
}

void BatteryClass::handleBinfo(AsyncWebServerRequest *request){
	if (!request->authenticate(_settings->login, _settings->password))
		if (!server.checkAdminAuth(request)) {
			return request->requestAuthentication();
		}
	if (request->args() > 0) {
		bool flag = false;
		if (request->hasArg("bmax")) {
			float t = request->arg("bmax").toFloat();
			Memory.eeprom.settings.bat_max = _max = CONVERT_V_TO_ADC(t);
			flag = true;
		}
		if (flag){
			if (request->hasArg("bmin")){
				float t = request->arg("bmin").toFloat();
				Memory.eeprom.settings.bat_min = _min = CONVERT_V_TO_ADC(t);
			}else{
				flag = false;
			}				
		}
		if (flag && Memory.save()) {
			goto url;
		}
		return request->send(400);
	}
	url:
//#ifdef HTML_PROGMEM
//	request->send_P(200, F("text/html"), binfo_html);
//#else
	request->send(SPIFFS, request->url());
//#endif
}