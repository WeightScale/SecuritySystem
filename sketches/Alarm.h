#pragma once
#include <Arduino.h>
#include <StringArray.h>
#define DEFAULT_INT_PIN 13

typedef enum { AL_MSG_SENDING, AL_MSG_SENT, AL_MSG_ERROR } AlarmMessageStatus;
class AlarmMessageBuffer;
class AlarmMessage;
class AlarmClient;
class AlarmClass;

class AlarmMessageBuffer {
private:
	uint8_t * _data;
	size_t _len;
	bool _lock; 
	uint32_t _count;  

public:
	AlarmMessageBuffer();
	AlarmMessageBuffer(size_t size);
	AlarmMessageBuffer(uint8_t * data, size_t size); 
	AlarmMessageBuffer(const AlarmMessage &); 
	AlarmMessageBuffer(AlarmMessageBuffer &&); 
	~AlarmMessageBuffer(); 
	void operator ++(int i) { _count++; }
	void operator --(int i) {  if (_count > 0) { _count--; }
		;  }
	bool reserve(size_t size);
	void lock() { _lock = true; }
	void unlock() { _lock = false; }
	uint8_t * get() { return _data; }
	size_t length() { return _len; }
	uint32_t count() { return _count; }
	bool canDelete() { return (!_count && !_lock); } 

	friend AlarmClass; 

};

class AlarmMessage{
protected:
	AlarmMessageStatus _status;
public:
	virtual ~AlarmMessage() {};
	virtual size_t send(AlarmClient *client __attribute__((unused))){ return 0; }
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
	virtual size_t send(AlarmClient *client) override;
};

class AlarmCallMessage : public AlarmMessage {
public:
	virtual ~AlarmCallMessage() override;
	virtual size_t send(AlarmClient *client) override;
};

class AlarmMultiMessage : public AlarmMessage {
private:
	uint8_t * _data;
	size_t _len;
	AlarmMessageBuffer * _ALbuffer; 
public:
	AlarmMultiMessage(AlarmMessageBuffer * buffer); 
	virtual ~AlarmMultiMessage() override;
	//virtual bool betweenFrames() const override { return _acked == _ack; }
	virtual size_t send(AlarmClient *client) override;
};

class AlarmClient{
private:
	LinkedList<AlarmMessage *> _messageQueue;
	void _queueMessage(AlarmMessage *dataMessage);
	void _runQueue();
	bool _send;
public:
	~AlarmClient() {_messageQueue.free();};
	bool canSend(); //ack is not pending
	void text(AlarmMessageBuffer *buffer);
	void text(const char * message, size_t len);
	void text(const String &message);
	bool isSend() {return _send;};
};

class AlarmClass{
private:
	LinkedList<AlarmClient *> _clients;
	bool _isInterrupt = false;
	bool _isSafe = true;
	volatile byte interruptCounter = 0;
	const byte interruptPin = DEFAULT_INT_PIN;
public:
	AlarmClass();
	bool isInterrupt(){return _isInterrupt;};
	void interrupt(){_isInterrupt = true;};
	bool isSafe(){return _isSafe;};
	void setSafe(bool safe){_isSafe = safe;};
	void handle();
	void textAll(AlarmMessageBuffer * buffer);
	void textAll(const char * message, size_t len);
	void textAll(const String &message);
	AlarmMessageBuffer * makeBuffer(uint8_t * data, size_t size);
	LinkedList<AlarmMessageBuffer *> _buffers;
	void _cleanBuffers();
	
};

extern AlarmClass Alarm;
extern void handleInterrupt();