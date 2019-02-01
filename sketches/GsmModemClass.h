#ifndef _GSMMODEMCLASS_h_
#define _GSMMODEMCLASS_h_
#include <SoftwareSerial.h>
#include "Arduino.h"

// DEFAULT PIN DEFINATIONS IN HERE
#define DEFAULT_RX_PIN 5
#define DEFAULT_TX_PIN 4
#define DEFAULT_RST_PIN 12

#define DEFAULT_LED_FLAG true
#define DEFAULT_LED_PIN 2

#define DEFAULT_BAUD_RATE 9600
#define BUFFER_RESERVE_MEMORY	255
#define TIME_OUT_READ_SERIAL	5000

enum enum_ask_t{	
	ERROR,
	NOT_READY,
	READY,
	CONNECT_OK,
	CONNECT_FAIL,
	ALREADY_CONNECT,
	SEND_OK,
	SEND_FAIL,
	DATA_ACCEPT,
	CLOSED_,
	READY_TO_RECEIVE,
	OK_
};

class GsmModemClass : public SoftwareSerial {
	private:
		uint32_t _baud;
		uint8_t _timeout;
		String _buffer;
		
		String _readSerial(uint32_t timeout);
		bool _checkResponse(enum_ask_t ask, uint16_t timeout);
		String _readSerialUtil(char terminator, uint16_t timeout);
		String _waitResponse(uint32_t timeout);
		static const uint8_t _responseInfoSize = 12; 
		const char* _responseInfo[_responseInfoSize] = {
			 "ERROR",
			"NOT READY",
			"READY",
			"CONNECT OK",
			"CONNECT FAIL",
			"ALREADY CONNECT",
			"SEND OK",
			"SEND FAIL",
			"DATA ACCEPT",
			"CLOSED",
			">",
			"OK"
		};
	public:
		uint8_t	RX_PIN;
		uint8_t TX_PIN;
		uint8_t RESET_PIN;
		uint8_t LED_PIN;
		bool LED_FLAG;
		uint32_t BAUDRATE;
		GsmModemClass(void);
		String _readSerial();
		void start();
		void reset();
		uint8_t signalQuality();
		String moduleModel();
		bool setFullMode();
		bool echoOff();
		bool echoOn();
		bool enterSleepMode();
		bool disableSleep();
		bool sendSMS(const char* number, const char* text);
		bool sendSMS(const char* number, uint8_t* text);
		String getSMS(uint8_t index);
	void processSMS(const String );
		String sendATCommand(String cmd, bool waiting, uint32_t timeout = TIME_OUT_READ_SERIAL);
};

extern GsmModemClass GsmModem;

#endif // !_GSMMODEMCLASS_h_
