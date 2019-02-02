#pragma once
#include <Arduino.h>
#include <StringArray.h>
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
	
	bool _send;
public:
	AlarmClient(char * p, bool s=true): _phone(p),_send(s){}	
	~AlarmClient() {if (_message != NULL)free(_message); };
	char *_phone;
	bool canSend(){return _send;}; //ack is not pending
	void text(const char * message, size_t len);
	void text(const String &message);
	void call();
};

class AlarmClass{
private:
	
	bool _isInterrupt = false;
	bool _safe = true;
	bool _pinInterrupt;
	volatile byte interruptCounter = 0;
	const byte interruptPin = DEFAULT_INT_PIN;
	LinkedList<AlarmClient *> _clients;
	String _codeOnAlarm = "1234";
	String _codeOffAlarm = "4321";
	
public:
	AlarmClass();
	String _msgDTMF = "";
	bool hashClient(String phone);
	bool isInterrupt(){return _isInterrupt;};
	void interrupt(bool i){_isInterrupt = i;};
	bool isSafe(){return _safe;};
	void safe(bool safe){_safe = safe;};
	void handle();
	void textAll(const char * message, size_t len);
	void textAll(const String &message);
	void callAll();
	void _cleanBuffers();
	void _addClient(AlarmClient * client); 
	void fetchMessage(uint8_t index);
	void fetchCall(String phone);
	void parseSMS(String msg);
	void parseDTMF(String msg);
	void fetchCommand(String cmd);
	void setStatusPinInt(bool pin) {_pinInterrupt = pin; };
	bool getStatusPinInt() {return _pinInterrupt;};
	byte getInterruptPin() {return interruptPin;};
};

extern AlarmClass Alarm;
extern void handleInterrupt();