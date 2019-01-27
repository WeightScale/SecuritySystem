#pragma once
#include <Arduino.h>
#define DEFAULT_INT_PIN 13

typedef enum { AL_MSG_SENDING, AL_MSG_SENT, AL_MSG_ERROR } AlarmMessageStatus;
class AlarmClient;

class AlarmMessage{
protected:
	AlarmMessageStatus _status;
public:
	virtual ~AlarmMessage() {};
	virtual size_t send(AlarmClient *client __attribute__((unused))){ return 0; }
	virtual bool finished(){ return _status != AL_MSG_SENDING; }
	virtual bool betweenFrames() const { return false; }
};

class AlarmSMSMessage : AlarmMessage{
private:
	uint8_t * _data;
public:
	virtual ~AlarmSMSMessage() override;
	virtual size_t send(AlarmClient *client) override;
};

class AlarmCallMessage : AlarmMessage{
public:
	virtual ~AlarmCallMessage() override;
	virtual size_t send(AlarmClient *client) override;
};

class AlarmClient{
private:
	LinkedList<AlarmMessage *> _messageQueue;
	void _queueMessage(AlarmMessage *dataMessage);
	void _runQueue();
public:
	~AlarmClient() {_messageQueue.free();};
	bool canSend(); //ack is not pending
};

class AlarmClass{
private:
	LinkedList<AlarmClient *> _clients;
	bool _isInterrupt = false;
	bool _isSafe = true;
	volatile byte interruptCounter = 0;
	const byte interruptPin = DEFAULT_INT_PIN;
public:
	AlarmClass(void);
	bool isInterrupt(){return _isInterrupt;};
	void interrupt(){_isInterrupt = true;};
	bool isSafe(){return _isSafe;};
	void setSafe(bool safe){_isSafe = safe;};
	void handle();
	
};

extern AlarmClass Alarm;
extern void handleInterrupt();