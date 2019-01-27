#include "Alarm.h"
#include "GsmModemClass.h"

AlarmClass Alarm;

AlarmSMSMessage::AlarmSMSMessage(const char * data, size_t len)
	: _len(len)
{	
	_data = (uint8_t*)malloc(_len + 1);
	if (_data == NULL) {
		_len = 0;
		_status = AL_MSG_ERROR;
	}
	else {
		_status = AL_MSG_SENDING;
		memcpy(_data, data, _len);
		_data[_len] = 0;
	}
}

AlarmClass::AlarmClass()
	:_clients(LinkedList<AlarmClient *>([](AlarmClient *c){ delete c; }))
	, _buffers(LinkedList<AlarmMessageBuffer *>([](AlarmMessageBuffer *b){ delete b; }))
{
	pinMode(interruptPin, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, RISING);	
}

void AlarmClass::handle(){
	if (!isSafe())
		return;
	if (isInterrupt()){
			
	}
	if (digitalRead(interruptPin)) {
		
		digitalWrite(2, LOW);
		GsmModem.flush();
		if (GsmModem.sendSMS("+380500784234", "test"))
			interruptCounter--;
		digitalWrite(2, HIGH);
	}
}

AlarmMessageBuffer * AlarmClass::makeBuffer(uint8_t * data, size_t size){
	AlarmMessageBuffer * buffer = new AlarmMessageBuffer(data, size); 
  
	if (buffer) {
		_buffers.add(buffer);
	}

	return buffer; 
}

void AlarmClass::_cleanBuffers(){
	for (AlarmMessageBuffer * c : _buffers) {
		if (c && c->canDelete()) {
			_buffers.remove(c);
		}
	}
}

void AlarmClass::textAll(AlarmMessageBuffer * buffer) {
	if (!buffer) return;
	buffer->lock(); 
	for (const auto& c : _clients) {
		if (c->isSend()) {
			c->text(buffer);
		}
	}
	buffer->unlock();
	_cleanBuffers(); 
}

void AlarmClass::textAll(const char * message, size_t len) {
	AlarmMessageBuffer * buffer = makeBuffer((uint8_t *)message, len); 
	textAll(buffer); 
}

void AlarmClass::textAll(const String &message) {
	textAll(message.c_str(), message.length());
}

void AlarmClient::text(AlarmMessageBuffer * buffer)
{
	_queueMessage(new AlarmMultiMessage(buffer));
}

void AlarmClient::text(const char * message, size_t len) {
	_queueMessage(new AlarmSMSMessage(message, len));
}
void AlarmClient::text(const String &message) {
	text(message.c_str(), message.length());
}

void ICACHE_RAM_ATTR handleInterrupt() {
	Alarm.interrupt();
}