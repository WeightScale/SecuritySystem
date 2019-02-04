#include "Alarm.h"
#include "GsmModemClass.h"
#include "Battery.h"

AlarmClass Alarm;

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

AlarmSMSMessage::~AlarmSMSMessage() {
	if (_data != NULL)
		free(_data);
}

void AlarmSMSMessage::send(AlarmClient *client) {
	if (_status != AL_MSG_SENDING)
		return ;
	
	//GsmModem.flush();
	GsmModem.sendSMS(client->_phone.c_str(), _data);
	_status = AL_MSG_SENDING;
	//delete this;
}

void AlarmClient::_runQueue() {
	/*while (!_messageQueue.isEmpty() && _messageQueue.front()->finished()) {
		_messageQueue.remove(_messageQueue.front());
	}

	if (!_messageQueue.isEmpty()) {
		_messageQueue.front()->send(this);
	}*/
	if (_message) {
		_message->send(this);
		delete _message;
	}
}

void AlarmClient::_queueMessage(AlarmMessage *dataMessage) {	
	if (dataMessage == NULL)
		return;		
	/*if (_messageQueue.length() > AL_MAX_QUEUED_MESSAGES) {
		ets_printf("ERROR: Too many messages queued\n");
		delete dataMessage;
	}
	else {
		_messageQueue.add(dataMessage);
	}*/
	_message = dataMessage;
	if (canSend())
		_runQueue();
}

void AlarmClient::text(const char * message, size_t len) {
	_queueMessage(new AlarmSMSMessage(message, len));
}
void AlarmClient::text(const String &message) {
	text(message.c_str(), message.length());
}

void AlarmClient::call() {
	GsmModem.onCallAccept([](int value) {
		GsmModem.sendATCommand(F("ATH\n"), false);		
	});
	GsmModem.doCall(_phone, 10000);
	GsmModem.sendATCommand(F("ATH\n"), false);
};

AlarmClass::AlarmClass() :_clients(LinkedList<AlarmClient *>([](AlarmClient *c){ delete c; }))
{
	pinMode(interruptPin, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, CHANGE);
}

void /*ICACHE_RAM_ATTR*/ AlarmClass::handle() {			
	if (!isInterrupt()){
		return;	
	}
	interrupt(false);
	if (!_safe){
		return;	
	}
	detachInterrupt(interruptPin);
	digitalWrite(DEFAULT_LED_PIN, LOW);
	_pinInterrupt = debounce(interruptPin);
	if (_pinInterrupt){
		callAll();
		textAll("Alarm: Open sensor!!!");	
		//digitalWrite(DEFAULT_LED_PIN, LOW);
	}else{
		callAll();
		textAll("Alarm: Closed sensor!!!");
		//digitalWrite(DEFAULT_LED_PIN, HIGH);
	}
	attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, CHANGE);
	digitalWrite(DEFAULT_LED_PIN, HIGH);
}

void AlarmClass::textAll(const String &message) {
	//textAll(message.c_str(), message.length());
	if(!message) return;
	for (const auto& c : _clients) {
		if (c->canSend()) {
			c->text(message);
		}
	}
}

void AlarmClass::callAll() {
	for (const auto& c : _clients) {
		if (c->canSend()) {
			c->call();
		}
	}	
};

void AlarmClass::_addClient(AlarmClient * client) {
	_clients.add(client);
}

void /*ICACHE_RAM_ATTR*/ AlarmClass::_removeClient(AlarmClient *client) {
	if (!client)
		return;
	if(!client->root())
		_clients.remove(client);
	/*_clients.remove_first([=](AlarmClient * c) {
		return c->_phone == client->_phone;
	});*/
}

AlarmClient *AlarmClass::hashClient(String phone){
	if (_clients.length() == 0)
		return NULL;
	for (const auto& c : _clients) {
		if (phone.indexOf(c->_phone)!=-1) {			
			return c;
		}
	}	
	return NULL;
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
	_curentClient = hashClient(phone);	
	if (_curentClient){
		_msgDTMF = "";
		GsmModem.sendATCommand("ATA", true);                  // ...отвечаем (поднимаем трубку)	
	}
	else{
		GsmModem.sendATCommand(F("ATH\n"), false);				//...сбрасываем
		//_curentClient = NULL;
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
	
	_curentClient = hashClient(msgphone);
	if (!_curentClient){			// Если телефон в белом списке, то...				 
		fetchCommand(msgbody);                  // ...выполняем команду
		//_curentClient = NULL;
	}else{
		textAll("In come message from tel:" + msgphone + " "+msgbody);	//отправляем чужие сообщения 
	}	
}

void AlarmClass::parseDTMF(String msg) {
	if (msg.equals("#")) {		
		if (_curentClient)
			if (fetchCommand(_msgDTMF)) {// ...выполняем команду
				GsmModem.sendATCommand(F("ATH\n"), false);	
			}; 
		//_curentClient = NULL;
		_msgDTMF = "";
	}else {
		_msgDTMF += msg;	
	};
};

bool /*ICACHE_RAM_ATTR*/ AlarmClass::fetchCommand(String cmd) {
	int index = cmd.indexOf("*");
	String command="",value="";
	if (index != -1){
		command = cmd.substring(0, index);
		value = cmd.substring(cmd.indexOf("*") + 1);
	}else{
		command = cmd.substring(0);
	}		
	if (command.length() != 3)
		return false;
	switch (command.toInt()){
		case 123:																//добавить клиента
			if(!_curentClient->root())
				return false;
			_addClient(new AlarmClient(value, true));	
		break;
		case 321:																//удалить клиента
			if(!_curentClient->root())
				return false;
			_removeClient(hashClient(value));
			
		break;
		case 111:{																//список клиентов
			if (!_curentClient->root())
				return false;
			if (_clients.length() == 0)
				return false;
			String msg = "";
			for (const auto& c : _clients) {
				msg += c->_phone + ":";					
			}
			GsmModem.sendSMS(_curentClient->_phone.c_str(), msg.c_str());
			break;			
		}
		case 222:																//поставить на сигнализацию
			_safe = true;
		break;
		case 333:																//снять с сигнализации
			_safe = false;
		break;
		case 444: {																//информация состояния модуля																			
			String info = "";
			info += "Battery:" + (String)BATTERY->getCharge() + "%";
			info += " Safe:" + (String)Alarm.isSafe();
			info += " Sensor:" + (String)debounce(interruptPin);
			GsmModem.sendSMS(_curentClient->_phone.c_str(), info.c_str());
			break;
		}
		default:
			return false;
	}	
	return true;
};

bool /*ICACHE_RAM_ATTR*/ debounce(uint8_t pin) {
	bool current = digitalRead(pin);
	if (current != Alarm.getStatusPinInt()) {// Старое значение отличается от полученного		                  
	  delay(10);                                   // Ждем пока состояние стабилизируется - игнорируем дребезг
	  current = digitalRead(pin);             // Считываем стабилизированное значение
	}
	return current;
}

void /*ICACHE_RAM_ATTR*/ handleInterrupt() {
	Alarm.interrupt(true);	               
	//Alarm.setStatusPinInt(debounce());// Получаем стабилизированное значение
	//digitalWrite(DEFAULT_LED_PIN, LOW);	
}