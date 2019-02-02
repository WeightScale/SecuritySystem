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
	}

	_buffer.reserve(BUFFER_RESERVE_MEMORY);
	
	reset();
	echoOff();
	while (!_checkResponse(OK_, 1000)) {
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
	}
}

void GsmModemClass::reset() {
	if (LED_FLAG) {
		digitalWrite(LED_PIN, HIGH);
	}

	digitalWrite(RESET_PIN, LOW);
	delay(1000);
	digitalWrite(RESET_PIN, HIGH);
	delay(1000);
	
	while (!_checkResponse(OK_,2000)) {
		this->print(F("AT\r"));
	}

	if (LED_FLAG) {
		digitalWrite(LED_PIN, LOW);
	}
}

// SIGNAL QUALTY - 0-31 | 0-> poor | 31 - Full | 99 -> Unknown
uint8_t GsmModemClass::signalQuality() {
	this->print("AT+CSQ\r");
	_buffer = _readSerial(10000);
	//_checkResponse(OK_, 5000);
	if((_buffer.indexOf("+CSQ:")) != -1) {
		return _buffer.substring(_buffer.indexOf("+CSQ: ") + 6, _buffer.indexOf(",")).toInt();
	}
	else {
		return 99;
	}
}

String GsmModemClass::moduleModel() {
	this->print("AT+CGMM\r\n");
	_buffer = _readSerialUtil('\n',5000);

	//String veri = _buffer.substring(8, _buffer.indexOf("OK"));
	//veri.trim();
	//veri.replace("_", " ");
	_buffer.trim();
	_buffer.replace("_", " ");
	return _buffer;
}

bool GsmModemClass::setFullMode() {
	//This set the device to full funcionality - AT+CFUN
	bool nowReady = false;
	this->print(F("AT+CFUN=1\r"));
	if(_checkResponse(OK_,3000))
		nowReady = true;
	// Let's confirm if this was valid
	//_buffer = _readSerial(20000);    // timeout 10s
	//if((_buffer.indexOf("OK") == -1) || (_buffer.indexOf("READY") == -1))
	//	nowReady = true;
	delay(10);
	// sometimes some excess data usually come out
	_buffer = _readSerial(3000);   // timeout 10s 
	return nowReady;
	//if(nowReady)	 
	//	return true;
	//else
	//	return false;
}

bool GsmModemClass::enterSleepMode() {
	// This set the device into a good sleep mode - AT+CFUN=0 and AT+CSCLK
	 this->print("AT+CFUN=0\r\n");
	//_buffer = _readSerial(10000);
	//_buffer = _readSerial(10000);
	//if (_buffer.indexOf("NOT_READY") == -1)	
	//	return false;
	if (!_checkResponse(NOT_READY, 3000)) {
		this->print("AT+CFUN=0\r\n");
		if (!_checkResponse(NOT_READY, 3000))
			return false;
	}	 
	delay(50);  // just chill small	
	// The GSM will send an OK response again
	//result = _checkResponse(5000);
	//if (result != OK)
	//	return false;
	delay(50);
	this->print("AT+CSCLK=2\r\n");  // enable automatic sleep
	if (!_checkResponse(OK_, 3000))
		return false;	 	 
	return true;
}
 
bool GsmModemClass::disableSleep() {
	//This mode disable sleep mode - AT+CSLK=0
	// first we need to send something random for as long as 100ms
	this->print(F("FF\r"));
	delay(120);  // this is between waking charaters and next AT commands
	//_buffer = _readSerial(1000);    // just incase something pops up
	this->print(F("AT+CSCLK=0\r"));
	//_buffer = _readSerial(20000);
	if(!_checkResponse(OK_, 3000))
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

bool /*ICACHE_RAM_ATTR*/ GsmModemClass::sendSMS(const char* number, const char* text) {			
	this->print("AT+CMGS=\"");   // command to send sms
	this->print(number);
	this->print("\"\r\n");	
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
	this->print("AT+CMGS=\"");    // command to send sms
	this->print(number);
	this->print("\"\r\n");
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

int ICACHE_RAM_ATTR GsmModemClass::doCall(String phone, uint16_t timeout) {
	this->print("ATD");     // command to send sms
	this->print(phone);
	this->print(";\n");
	String str = "";
	int status = -1;
	uint64_t timeOld = millis();	
	while ((millis() < timeOld + timeout)){
		str = _waitResponse(1000);
		int index = str.indexOf(F("+CLCC:"));
		if (index != -1){
			status = str.substring(index + 11, index + 12).toInt();			
			switch (status){
			case 3:
				return status;
			default:
				continue;
			}
		}
	}
	return status;	
};

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

String ICACHE_RAM_ATTR GsmModemClass::_readSerial(uint32_t timeout) {	
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