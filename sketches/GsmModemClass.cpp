#include "Arduino.h"
#include "GsmModemClass.h"
#include <SoftwareSerial.h>

GsmModemClass GsmModem;

GsmModemClass::GsmModemClass(void) : SoftwareSerial(DEFAULT_RX_PIN, DEFAULT_TX_PIN) {
	RX_PIN = DEFAULT_RX_PIN;
	TX_PIN = DEFAULT_TX_PIN;
	RESET_PIN = DEFAULT_RST_PIN;
	LED_PIN = DEFAULT_LED_PIN;
	LED_FLAG = DEFAULT_LED_FLAG;
}

void GsmModemClass::start() {

	pinMode(RESET_PIN, OUTPUT);
	digitalWrite(RESET_PIN, HIGH);

	_baud = DEFAULT_BAUD_RATE;

	this->begin(_baud);

	if (LED_FLAG) {
		pinMode(LED_PIN, OUTPUT);
		digitalWrite(LED_PIN, LOW);
	}

	_buffer.reserve(BUFFER_RESERVE_MEMORY);
	
	reset();
	echoOff();
	while (sendATCommand(F("AT+CLIP=1\r"), true).indexOf("OK") == -1) {};
	while (sendATCommand(F("AT+CMGF=1;&W"), true).indexOf("OK") == -1) {};
	while (sendATCommand(F("AT+DDET=1"), true).indexOf("OK") == -1) {};
	while (sendATCommand(F("AT+CLCC=1"), true).indexOf("OK") == -1) {};
	/*while (!_checkResponse(OK_, 1000)) {
		this->print(F("AT+CLIP=1\r"));
	}
	while (!_checkResponse(OK_, 1000)) {
		this->println("AT+CMGF=1;&W");
	}
	while (!_checkResponse(OK_, 1000)){
		this->println("AT+DDET=1");
	}
	while (!_checkResponse(OK_, 1000)) {
		this->println("AT+CLCC=1");
	}*/
	
	
	
	if (LED_FLAG) {
		digitalWrite(LED_PIN, HIGH);
	}
}

void GsmModemClass::reset() {
	//if (LED_FLAG) {
	//	digitalWrite(LED_PIN, HIGH);
	//}

	digitalWrite(RESET_PIN, LOW);
	delay(1000);
	digitalWrite(RESET_PIN, HIGH);
	delay(1000);
	
	while (sendATCommand(F("AT\r"), true).indexOf("OK") == -1) {};
	
	//while (!_checkResponse(OK_,2000)) {
	//	this->print(F("AT\r"));
	//}	
}

// SIGNAL QUALTY - 0-31 | 0-> poor | 31 - Full | 99 -> Unknown
uint8_t GsmModemClass::signalQuality() {
	this->print(F("AT+CSQ\r"));
	_buffer = _readSerial(10000);
	//_checkResponse(OK_, 5000);
	if((_buffer.indexOf(F("+CSQ:"))) != -1) {
		return _buffer.substring(_buffer.indexOf(F("+CSQ: ")) + 6, _buffer.indexOf(",")).toInt();
	}
	else {
		return 99;
	}
}

String GsmModemClass::moduleModel() {
	this->print(F("AT+CGMM\r\n"));
	_buffer = _readSerialUtil('\n',5000);

	//String veri = _buffer.substring(8, _buffer.indexOf("OK"));
	//veri.trim();
	//veri.replace("_", " ");
	_buffer.trim();
	_buffer.replace("_", " ");
	return _buffer;
}

bool /*ICACHE_RAM_ATTR*/ GsmModemClass::setFullMode() {
	//This set the device to full funcionality - AT+CFUN
	bool nowReady = false;
	String str = sendATCommand(F("AT+CFUN?\r"), true);
	int index = str.indexOf(F("CFUN:"));
	index = str.substring(index + 6, index + 7).toInt();
	if (index == 0) {
		this->print(F("AT+CFUN=1\r"));
		str = _checkResponse(F("OK"), 10000);
		if (str.indexOf(F("READY")) != -1)
			nowReady = true;
		str = _checkResponse(F("Call Ready"), 20000);
	}else
		return true;
	return nowReady;
}

bool /*ICACHE_RAM_ATTR*/ GsmModemClass::enterSleepMode() {	
	String str = sendATCommand(F("AT+CFUN?\r"), true);
	int index = str.indexOf(F("CFUN:"));
	index = str.substring(index + 6, index + 7).toInt();
	if (index == 1){
		this->print(F("AT+CFUN=0\r"));
		str = _checkResponse(F("OK"),10000);
		if (str.indexOf(F("NOT READY")) == -1)
			return false;
	}
	if(sendATCommand(F("AT+CSCLK=2\r\n"), true).indexOf(F("OK")) == -1)
		return false;	 	 
	return true;
}
 
bool /*ICACHE_RAM_ATTR*/ GsmModemClass::disableSleep() {
	//This mode disable sleep mode - AT+CSLK=0
	// first we need to send something random for as long as 100ms
	this->print(F("FF\r"));
	delay(120);  // this is between waking charaters and next AT commands	
	if(sendATCommand(F("AT+CSCLK=0\r"), true).indexOf(F("OK")) == -1)
		return false;
	delay(100);  // just chill for 100ms for things to stablize
	if(!setFullMode())
		return false;
	//NOTE - After disabling sleep a good idea is to make the device to flush the serial
	return true;
}

// ECHO OFF
bool GsmModemClass::echoOff() {
	this->print(F("ATE0\r"));	
	return _checkResponse(OK_, 5000);
}

// ECHO ON
bool GsmModemClass::echoOn() {
	this->print(F("ATE1\r"));	
	return _checkResponse(OK_, 3000);
}

bool /*ICACHE_RAM_ATTR*/ GsmModemClass::isReady() {
	this->print(F("AT+CCALR?\r"));	
	String str = "";
	int status = -1;
	uint64_t timeOld = millis();	
	while (millis() < (timeOld + 2000)) {
		str = _waitResponse(1000);
		int index = str.indexOf(F("CCALR:"));
		if (index == -1)
			continue;
		status = str.substring(index + 7, index + 8).toInt();
		if (status != 1) {
			return false;	
		}
		return true;		
	}
	return false;
}

bool /*ICACHE_RAM_ATTR*/ GsmModemClass::sendSMS(const char* number, const char* text) {			
	this->print(F("AT+CMGS=\""));   // command to send sms
	this->print(number);
	this->print(F("\"\r\n"));	
	if(_checkResponse(READY_TO_RECEIVE, 20000)) {
		this->print(text);
		this->print("\r");
		//delay(1000);
		//_checkResponse(OK_,1000);
		this->print((char)26);		
		return _checkResponse(OK_, 10000);
	}
	return false;
}

bool GsmModemClass::sendSMS(const char* number, uint8_t* text) {	
	this->print(F("AT+CMGS=\""));    // command to send sms
	this->print(number);
	this->print(F("\"\r\n"));
	if(_checkResponse(READY_TO_RECEIVE, 20000)) {
		this->print(String((char*)text));
		this->print("\r");
		//_checkResponse(OK_, 1000);
		this->print((char)26);
		return _checkResponse(OK_, 10000);
	}
	return false;
}

String GsmModemClass::getSMS(uint8_t index){
	return sendATCommand("AT+CMGR=" + (String)index + ",1", true);
}

void /*ICACHE_RAM_ATTR*/ GsmModemClass::doCall(String phone, uint16_t timeout) {
	this->print(F("ATD"));     // command to send sms
	this->print(phone);
	this->print(";\n");
	String str = "";
	int status = -1;
	uint64_t timeOld = millis();	
	while (millis() < (timeOld + timeout)){
		str = _waitResponse(1000);
		int index = str.indexOf(F("+CLCC:"));
		if (index == -1)
			continue;
		status = str.substring(index + 11, index + 12).toInt();
		if (status == 3){
			timeOld = millis();
			while (millis() < (timeOld + 6000)) {delay(1); }
			break;	
		}		
	}
	_handleAccept(status);
};

String GsmModemClass::_checkResponse(String ask, uint16_t timeout){
	unsigned long t = millis();
	String tempData = "";
	while (millis() < t + timeout) {
		//count++;
		if(this->available()) {
			tempData += this->_readSerial(timeout);    // reads the response	
			if(tempData.indexOf(ask) != -1)
				break;				  
		}
		delay(1);
	}
	return tempData;
}

bool GsmModemClass::_checkResponse(enum_ask_t ask, uint16_t timeout){	
	unsigned long t = millis();
	while(millis() < t + timeout){
		//count++;
		if(this->available()){
			String tempData = this->_readSerialUtil('\n', timeout);   // reads the response	
			//String tempData = this->_readSerial(timeout);    // reads the response
			if (tempData.indexOf(_responseInfo[ask]) != -1)
				return true;
		/*
		  * Checks for the status response
		  * Response are - OK, ERROR, READY, >, CONNECT OK
		  * SEND OK, DATA ACCEPT, SEND FAIL, CLOSE, CLOSED
		  * note_ LOCAL iP COMMANDS HAS NO ENDING RESPONSE 
		  * ERROR - 0
		  * READY - 1
		  * CONNECT OK - 2
		  * SEND OK - 3
		  * SEND FAIL - 4
		  * CLOSED - 5
		  * > - 6
		  * OK - 7
		  * 
		  * 
		  */		  
		}
		ESP.wdtFeed();
	}
	return false;
}

String /*ICACHE_RAM_ATTR*/ GsmModemClass::_readSerialUtil(char terminator, uint16_t timeout) {

	uint64_t timeOld = millis();
	String tempData = "";
	while (!this->available() && !(millis() > timeOld + timeout)) {
		delay(1);
	}
	if (this->available()){
		
		//String ret;
		int c = timedRead();
		while (c >= 0 && c != terminator && !(millis() > (timeOld + timeout))) {
			tempData += (char) c;
			c = timedRead();
			delay(1);
		}
	return tempData;		
	}
	return tempData;	
}
static int millis_ge(unsigned long ms) {
	long l = (long)(millis() - ms);
	return (l) >= 0;
}
String GsmModemClass::_readSerial() {	
	String tempData = "";
	uint64_t timeOld = millis();
	while (this->available() && !(millis() > (timeOld + 50))) {
		if (this->available()) {	
			tempData += (char) this->read();
			timeOld = millis();
		}
		delay(1);
	}
	return tempData;	
}

String /*ICACHE_RAM_ATTR*/ GsmModemClass::_readSerial(uint32_t timeout) {	
	String tempData = "";
	uint64_t timeOld = millis();
	while (this->available() && !(millis() > (timeOld + timeout))) {
		if (this->available()) {	
			tempData += (char) this->read();
			timeOld = millis();
		}
		delay(1);
	}
	return tempData;
}

String GsmModemClass::sendATCommand(String cmd, bool waiting, uint32_t timeout) {
	String _resp = "";                                                // Переменная для хранения результата	
	GsmModem.println(cmd);                                              // Отправляем команду модулю
	if(waiting) {// Если необходимо дождаться ответа...		
		_resp = _waitResponse(timeout);                                         // ... ждем, когда будет передан ответ
		// Если Echo Mode выключен (ATE0), то эти 3 строки можно закомментировать
		if(_resp.startsWith(cmd)) {// Убираем из ответа дублирующуюся команду			
			_resp = _resp.substring(_resp.indexOf("\r", cmd.length()) + 2);
		}
	}
	return _resp;                                                   // Возвращаем результат. Пусто, если проблема
}

String GsmModemClass::_waitResponse(uint32_t timeout) {
	// Функция ожидания ответа и возврата полученного результата
	String _resp = "";                                                // Переменная для хранения результата
	long _timeout = millis() + timeout;                                 // Переменная для отслеживания таймаута (10 секунд)
	while(!GsmModem.available() && millis() < _timeout) {delay(1);};         // Ждем ответа 10 секунд, если пришел ответ или наступил таймаут, то...
	if(GsmModem.available()) {
		// Если есть, что считывать...
		_resp = GsmModem.readString();                                     // ... считываем и запоминаем
	}	
	return _resp;                                                   // ... возвращаем результат. Пусто, если проблема
}