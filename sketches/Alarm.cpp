#include "Alarm.h"
#include "GsmModemClass.h"

AlarmClass Alarm;

AlarmMessageBuffer::AlarmMessageBuffer()
	: _data(nullptr)
	, _len(0)
	, _lock(false)
	, _count(0)
{

}

AlarmMessageBuffer::AlarmMessageBuffer(uint8_t * data, size_t size) 
	: _data(nullptr)
	, _len(size)
	, _lock(false)
	, _count(0)
{

	if (!data) {
		return; 
	}

	_data = new uint8_t[_len + 1];

	if (_data) {
		memcpy(_data, data, _len);
		_data[_len] = 0; 
	}
}

AlarmMessageBuffer::AlarmMessageBuffer(size_t size)
	: _data(nullptr)
	, _len(size)
	, _lock(false)
	, _count(0)
{
	_data = new uint8_t[_len + 1]; 

	if (_data) {
		_data[_len] = 0; 
	}
  
}

AlarmMessageBuffer::AlarmMessageBuffer(const AlarmMessageBuffer & copy)
	: _data(nullptr)
	, _len(0)
	, _lock(false)
	, _count(0)
{
	_len = copy._len;
	_lock = copy._lock;
	_count = 0;

	if (_len) {
		_data = new uint8_t[_len + 1]; 
		_data[_len] = 0; 
	} 

	if (_data) {
		memcpy(_data, copy._data, _len);
		_data[_len] = 0; 
	}

}

AlarmMessageBuffer::AlarmMessageBuffer(AlarmMessageBuffer && copy)
	: _data(nullptr)
	, _len(0)
	, _lock(false)
	, _count(0)
{
	_len = copy._len;
	_lock = copy._lock;
	_count = 0;

	if (copy._data) {
		_data = copy._data; 
		copy._data = nullptr; 
	} 

}

AlarmMessageBuffer::~AlarmMessageBuffer(){
	if (_data) {
		delete[] _data; 
	}
}

AlarmMessageBuffer * AlarmClass::makeBuffer(uint8_t * data, size_t size) {
	AlarmMessageBuffer * buffer = new AlarmMessageBuffer(data, size); 
  
	if (buffer) {
		_buffers.add(buffer);
	}

	return buffer; 
}

AlarmSMSMessage::AlarmSMSMessage(const char * data, size_t len)	: _len(len)
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

AlarmMultiMessage::AlarmMultiMessage(AlarmMessageBuffer * buffer)
	: _ALbuffer(nullptr)
{

	if (buffer) {
		_ALbuffer = buffer; 
		(*_ALbuffer)++; 
		_data = buffer->get(); 
		_len = buffer->length(); 
		_status = AL_MSG_SENDING;
		//ets_printf("M: %u\n", _len);
	}
	else {
		_status = AL_MSG_ERROR;
	}
  
} 


AlarmMultiMessage::~AlarmMultiMessage() {
	if (_ALbuffer) {
		(*_ALbuffer)--;  // decreases the counter. 
	}
}

size_t AlarmMultiMessage::send(AlarmClient *client) {
	if (_status != AL_MSG_SENDING)
		return 0;

	size_t sent /*= webSocketSendFrame(client, final, opCode, _mask, dPtr, toSend)*/;
	GsmModem.flush();
	GsmModem.sendSMS(client->_phone, _data);
	_status = AL_MSG_SENDING;	
	return sent;
}

void AlarmClient::_runQueue() {
	while (!_messageQueue.isEmpty() && _messageQueue.front()->finished()) {
		_messageQueue.remove(_messageQueue.front());
	}

	if (!_messageQueue.isEmpty() /*&& _messageQueue.front()->betweenFrames()*/) {
		_messageQueue.front()->send(this);
	}
}

void AlarmClient::_queueMessage(AlarmMessage *dataMessage) {	
	if (dataMessage == NULL)
		return;		
	if (_messageQueue.length() > AL_MAX_QUEUED_MESSAGES) {
		ets_printf("ERROR: Too many messages queued\n");
		delete dataMessage;
	}
	else {
		_messageQueue.add(dataMessage);
	}
	if (canSend())
		_runQueue();
}

void AlarmClient::text(AlarmMessageBuffer * buffer) {
	_queueMessage(new AlarmMultiMessage(buffer));
}

void AlarmClient::text(const char * message, size_t len) {
	_queueMessage(new AlarmSMSMessage(message, len));
}
void AlarmClient::text(const String &message) {
	text(message.c_str(), message.length());
}

AlarmClass::AlarmClass()
	:_clients(LinkedList<AlarmClient *>([](AlarmClient *c){ delete c; }))
	, _buffers(LinkedList<AlarmMessageBuffer *>([](AlarmMessageBuffer *b){ delete b; }))
{
	pinMode(interruptPin, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, RISING);	
}

void AlarmClass::handle() {
	if (!isSafe())
		return;
	if (isInterrupt()){
		textAll("Alarm sensor!!!");
		interrupt(false);
	}
	/*if (digitalRead(interruptPin)) {
		
		digitalWrite(2, LOW);
		GsmModem.flush();
		if (GsmModem.sendSMS("+380500784234", "test"))
			interruptCounter--;
		digitalWrite(2, HIGH);
	}*/
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
		if (c->canSend()) {
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

void AlarmClass::_addClient(AlarmClient * client) {
	_clients.add(client);
}

bool AlarmClass::hashClient(String phone){
	if (_clients.length() == 0)
		return false;
	for (const auto& c : _clients) {
		if (phone.indexOf(c->_phone)) {
			return true;
		}
	}	
	return false;
}

void AlarmClass::fetchMessage(uint8_t index) {
	String str = "";
	bool hasmsg = false;
	str = GsmModem.getSMS(index);
	str.trim();                                         // Убираем пробелы в начале/конце
    if(str.endsWith("OK")) {// Если ответ заканчивается на "ОК"
		if(!hasmsg) 
			hasmsg = true;                            // Ставим флаг наличия сообщений для удаления
        //GsmModem.sendATCommand("AT+CMGR=" + (String)index, true);     // Делаем сообщение прочитанным
		//GsmModem.sendATCommand("\n", true);                              // Перестраховка - вывод новой строки
		parseSMS(str);                                   // Отправляем текст сообщения на обработку            
	}else {		                                                  // Если сообщение не заканчивается на OK	  
		GsmModem.sendATCommand("\n", true);                              // Отправляем новую строку и повторяем попытку
	}
};

void AlarmClass::fetchCall(String phone) {
	//static unsigned int count_call;
	if (Alarm.hashClient(phone)) {
		//count_call++;
		//if (count_call > 2){
			GsmModem.sendATCommand("ATA", true);                // ...отвечаем (поднимаем трубку)	
		//}
	}	
};

void AlarmClass::parseSMS(String msg) {
	String msgheader  = "";
	String msgbody    = "";
	String msgphone   = "";

	msg = msg.substring(msg.indexOf("+CMGR: "));
	msgheader = msg.substring(0, msg.indexOf("\r"));             // Выдергиваем телефон

	msgbody = msg.substring(msgheader.length() + 2);
	msgbody = msgbody.substring(0, msgbody.lastIndexOf("OK"));   // Выдергиваем текст SMS
	msgbody.trim();

	int firstIndex = msgheader.indexOf("\",\"") + 3;
	int secondIndex = msgheader.indexOf("\",\"", firstIndex);
	msgphone = msgheader.substring(firstIndex, secondIndex);

	if(Alarm.hashClient(msgphone)) {// Если телефон в белом списке, то...		 
		fetchCommand(msgbody, msgphone);                            // ...выполняем команду
	}	
}

void AlarmClass::parseDTMF(String msg) {
	if (msg.equals("#")) {
		if (_msgDTMF.indexOf(_codeOnAlarm)){
			_safe = true;	
		}else if (_msgDTMF.indexOf(_codeOffAlarm)){
			_safe = false;	
		}
		GsmModem.sendATCommand(F("ATH\n"), false);		
		_msgDTMF = "";
	}else {
		_msgDTMF += msg;	
	};
};

void AlarmClass::fetchCommand(String cmd, String phone) {
	if (cmd.indexOf(_codeOnAlarm)){
		_safe = true;	
	}else if (cmd.indexOf(_codeOffAlarm)){
		_safe = false;
	}	
};

void ICACHE_RAM_ATTR handleInterrupt() {
	Alarm.interrupt(true);
}