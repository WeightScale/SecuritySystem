#pragma once
#include <Arduino.h>
#include <StringArray.h>
#include <ArduinoJson.h>
#include <functional>
#include "Task.h"
#define DEFAULT_INT_PIN 13
#define AL_MAX_QUEUED_MESSAGES 8

typedef enum { AL_MSG_SENDING, AL_MSG_SENT, AL_MSG_ERROR } AlarmMessageStatus;
class AlarmMessage;
class AlarmClient;
class AlarmClass;

class AlarmMessage{
protected:
	AlarmMessageStatus _status;
public:
	AlarmMessage() : _status(AL_MSG_ERROR) {}
	virtual ~AlarmMessage() {};
	virtual void send(AlarmClient *client __attribute__((unused))){ return; }
	virtual bool finished(){ return _status != AL_MSG_SENDING; }
	virtual bool betweenFrames() const { return false; }
};

class AlarmSMSMessage : public AlarmMessage {
private:
	size_t _len;
	uint8_t * _data;
public:
	AlarmSMSMessage(const char * data, size_t len);
	virtual ~AlarmSMSMessage() override;
	virtual void send(AlarmClient *client) override;
};

class AlarmCallMessage : public AlarmMessage {
public:
	virtual ~AlarmCallMessage() override;
	virtual void send(AlarmClient *client) override;
};

class AlarmClient{
private:
	//LinkedList<AlarmMessage *> _messageQueue;
	AlarmMessage * _message;
	void _queueMessage(AlarmMessage *dataMessage);
	void _runQueue();	
	String _filename;
	bool _send;
	bool _safe;
	bool _root = false;
public:
	AlarmClient(const String filename, const String phone, bool send = true, bool safe = false, bool root = false)
		: _filename(filename),_phone(phone),_send(send),_safe(safe),_root(root) {}
	AlarmClient(char * p, bool s=true, bool r=false): _phone(p),_send(s),_root(r){}
	AlarmClient(const String p, bool s = true) : _phone(p), _send(s) {}
	~AlarmClient() {if (_message != NULL)free(_message); };
	const String _phone;
	bool canSend(){return _send;}; //ack is not pending
	void text(const char * message, size_t len);
	void text(const String &message);
	void call();
	bool root() {return _root;};
	void root(bool root) {_root = root;};
	bool safe() {return _safe;};
	String& filename() {return _filename;};
};

typedef std::function<void(String)> AlarmHandleCommand;
class AlarmClass{	
private:
	AlarmHandleCommand _handleCommand;
	bool _isInterrupt = false;
	bool _safe = true;
	bool _sleep = false;
	bool _pinInterrupt;
	volatile byte interruptCounter = 0;
	const byte _interruptPin = DEFAULT_INT_PIN;
	LinkedList<AlarmClient *> _clients;
	String _codeOnAlarm = "1234";
	String _codeOffAlarm = "4321";
	AlarmClient *_curentClient;
	
public:
	AlarmClass();
	~AlarmClass();
	String _msgDTMF = "";
	AlarmClient *hashClient(String phone);
	//AlarmClient getClient(String phone);
	void begin();
	AlarmClient *curentClient() {return _curentClient;};
	bool isInterrupt(){return _isInterrupt;};
	void interrupt(bool i){_isInterrupt = i;};
	void interrupt();
	bool safe(){return _safe;};
	void safe(bool safe);
	void sleep(bool sleep) {_sleep = sleep;};
	bool sleep() {return _sleep;};
	void handle();
	void textAll(const char * message, size_t len);
	void textAll(const String &message);
	void callAll();
	void _cleanBuffers();
	void _addClient(AlarmClient * client); 
	void removeClient(AlarmClient *client);
	void fetchMessage(uint8_t index);
	void fetchCall(String phone);
	void parseSMS(String msg);
	void parseDTMF(String msg);
	bool fetchCommand(String cmd);
	void setStatusPinInt(bool pin) {_pinInterrupt = pin; };
	bool getStatusPinInt() {return _pinInterrupt;};
	byte interruptPin() {return _interruptPin;};
	bool createClient(String phone);
	size_t doStatus(JsonObject& json);
	void onCommand(AlarmHandleCommand fn){_handleCommand = fn;};
	void listClients(String value);
};

extern AlarmClass Alarm;
extern Task taskSleepModem;
extern void handleInterrupt();
extern bool /*ICACHE_RAM_ATTR*/ debounce(uint8_t pin);