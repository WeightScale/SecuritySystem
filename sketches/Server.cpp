#include "Server.h"
#include <SPIFFSEditor.h>
#include <AsyncJson.h>
#include <functional>
#include <ESP8266NetBIOS.h>
#include "Memory.h"
#include "HttpUpdater.h"
#include "Battery.h"
#include "Alarm.h"
#include "tools.h"
#include "alarm_config.h"

ServerClass server(80);
AsyncWebSocket ws("/ws");
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);
IPAddress lanIp; 			// Надо сделать настройки ip адреса
IPAddress gateway;

ServerClass::ServerClass(uint16_t port)	: AsyncWebServer(port) {}
ServerClass::~ServerClass() {}
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);
void onStationModeConnected(const WiFiEventStationModeConnected& evt);
void onStationModeDisconnected(const WiFiEventStationModeDisconnected& evt);
void scanResultForConnect(int networksFound);
void connectWifi();
WiFiEventHandler stationModeConnectedHandler;
WiFiEventHandler stationModeDisconnectedHandler;
Task taskConnectWiFi([]() {
		connectWifi();
	},30000); /* Пытаемся соедениться с точкой доступа каждые 30 секунд */

void ServerClass::setup() {
	_settings = &Memory.eeprom.settings;
	_apssid = &Memory.eeprom.settings.apSSID[0];
	_hostname = String(_settings->apSSID);
	_hostname.toLowerCase();
	_httpAuth.wwwUsername = "sa";
	_httpAuth.wwwPassword = "343434";
	
	stationModeConnectedHandler = WiFi.onStationModeConnected(&onStationModeConnected);	
	stationModeDisconnectedHandler = WiFi.onStationModeDisconnected(&onStationModeDisconnected);
	
	WiFi.persistent(false);	
	//WiFi.smartConfigDone();
	WiFi.mode(WIFI_AP_STA);
	WiFi.hostname(_hostname);
	WiFi.softAPConfig(apIP, apIP, netMsk);
	WiFi.softAP(_apssid, SOFT_AP_PASSWORD);
	
	if (!_settings->autoIp) {
		if (lanIp.fromString(_settings->lanIp) && gateway.fromString(_settings->gateway)) {
			WiFi.config(lanIp, gateway, netMsk);    									// Надо сделать настройки ip адреса
		}
	}
	WiFi.softAPConfig(apIP, apIP, netMsk);
	WiFi.softAP(getApSSID(), SOFT_AP_PASSWORD);
	
	ws.onEvent(onWsEvent);
	addHandler(&ws);
	addHandler(new SPIFFSEditor(_httpAuth.wwwUsername.c_str(), _httpAuth.wwwPassword.c_str()));	
	addHandler(new HttpUpdaterClass("sa", "654321"));
	on("/settings.html", HTTP_ANY, handleSettings);
	on("/settings.json", HTTP_ANY, handleSettingsJson);
	on("/rc",HTTP_GET,[](AsyncWebServerRequest *request) {
		AsyncWebServerResponse *response = request->beginResponse_P(200, PSTR("text/html"), PSTR("RECONNECT..."));
		response->addHeader("Connection", "close");
		request->onDisconnect([]() {
			SPIFFS.end();
			ESP.restart();
		 });
		request->send(response); }); /* Пересоединиться по WiFi. */
	on("/sv",[](AsyncWebServerRequest *request) {/* Получить значения. */
		if (!server.isAuthentified(request))
			return request->requestAuthentication();
		AsyncJsonResponse * response = new AsyncJsonResponse();
		JsonObject& root = response->getRoot();
		//root["id_date"] = getDateTime();
		root["id_local_host"] = WiFi.hostname();
		root["id_ap_ssid"] = String(server.getApSSID());
		root["id_ap_ip"] = toStringIp(WiFi.softAPIP());
		root["id_ip"] = toStringIp(WiFi.localIP());				
		root["chip_v"] = String(ESP.getCpuFreqMHz());
		root["id_mac"] = WiFi.macAddress();
		root["id_vr"] = SKETCH_VERSION;
		response->setLength();
		request->send(response);
	}); 	
	on("/heap",	HTTP_GET,[](AsyncWebServerRequest *request) {
			String str = String("Heap: ");
			str += String(ESP.getFreeHeap());
			str += " client: ";
			//str += String(ws.count());
			request->send(200, F("text/plain"), str);
	});
	on("/rst",HTTP_ANY,[this](AsyncWebServerRequest * request) {
			if (!isAuthentified(request)) {
				return request->requestAuthentication();
			}
			if (Memory.doDefault())
			request->send_P(200, PSTR("text/html"), PSTR("Установлено!"));
			else
				request->send(400);
	});
	on("/binfo.html", std::bind(&BatteryClass::handleBinfo, BATTERY, std::placeholders::_1));
	on("/net",[this](AsyncWebServerRequest * request) {
		/* Открыть настройки сети.*/
		if (!checkAdminAuth(request))
			return request->requestAuthentication();
		if (request->args() > 0) {
			String message = " ";
			if (request->hasArg("ssid")) {
				_settings->autoIp = true;				
				request->arg("ssid").toCharArray(_settings->wSSID, request->arg("ssid").length() + 1);			
				if (String(_settings->wSSID).length() > 0) {
					taskConnectWiFi.resume();
				}else {
					taskConnectWiFi.pause();
				}
				request->arg("key").toCharArray(_settings->wKey, request->arg("key").length() + 1);
				goto save;
			}		
			save:
			if (Memory.save())
				goto url;
			return request->send(400);
		}
		url:
		#ifdef HTML_PROGMEM
			request->send_P(200, F("text/html"), settings_html);
		#else
			if (request->url().equalsIgnoreCase("/net"))
				request->send_P(200, F("text/html"), netIndex);
			else
				request->send(SPIFFS, request->url());
		#endif
	});
#ifdef HTML_PROGMEM
	on("/", [](AsyncWebServerRequest * reguest){	reguest->send_P(200, F("text/html"), index_html); }); /* Главная страница. */	 
	on("/global.css", [](AsyncWebServerRequest * reguest){	reguest->send_P(200, F("text/css"), global_css); }); /* Стили */
	/*on("/favicon.png",[](AsyncWebServerRequest * request){
		AsyncWebServerResponse *response = request->beginResponse_P(200, "image/png", favicon_png, favicon_png_len);
		request->send(response);
	});*/	
	on("/bat.png", handleBatteryPng);
	on("/scales.png", handleScalesPng);	
	on("/und.png",
		[](AsyncWebServerRequest * request) {
			AsyncWebServerResponse *response = request->beginResponse_P(200, F("image/png"), und_png, und_png_len);
			request->send(response);
		});
	on("/set.png",
		[](AsyncWebServerRequest * request) {
			AsyncWebServerResponse *response = request->beginResponse_P(200, F("image/png"), set_png, set_png_len);
			request->send(response);
		});
	on("/zerow.png",
		[](AsyncWebServerRequest * request) {
			AsyncWebServerResponse *response = request->beginResponse_P(200, F("image/png"), zerow_png, zerow_png_len);
			request->send(response);
		});
	serveStatic("/", SPIFFS, "/");
#else
	on("/index.html",[](AsyncWebServerRequest * request) {
		if (!request->authenticate(Memory.eeprom.settings.login_alarm, Memory.eeprom.settings.password_alarm))
			if (!server.checkAdminAuth(request)) {
				return request->requestAuthentication();
			}		
		request->send(SPIFFS, request->url()); 
	}); /* Главная страница. */
	rewrite("/sn", "/settings.html");
	rewrite("/", "/index.html");
	serveStatic("/", SPIFFS, "/")/*.setDefaultFile("index.html")*/;	
#endif
	if (BATTERY->isDischarged()) {
		on("/ds",[](AsyncWebServerRequest *request) {
			AsyncWebServerResponse *response = request->beginResponse(SPIFFS, PSTR("/balert.html"));
			response->addHeader(PSTR("Connection"), PSTR("close"));
				request->onDisconnect([]() {
					ESP.deepSleep(20000);
				});
				request->send(response);	
			});
		rewrite("/", "/index.html");
		rewrite("/index.html", "/ds");
	}
	//serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");					
	onNotFound([](AsyncWebServerRequest *request) {
		request->send(404);
	});
	AsyncWebServer::begin();
	connectWifi();
};

bool ServerClass::checkAdminAuth(AsyncWebServerRequest * r) {	
	return r->authenticate(_httpAuth.wwwUsername.c_str(), _httpAuth.wwwPassword.c_str());
}

bool ServerClass::isAuthentified(AsyncWebServerRequest * request) {
	if (!request->authenticate(_settings->login, _settings->password)) {
		if (!checkAdminAuth(request)) {
			return false;
		}
	}
	return true;
}

void handleSettings(AsyncWebServerRequest *request) {
	if (request->args() > 0) {
		String message = " ";
		if (request->hasArg("ssid")) {
			if (request->hasArg("auto"))
				Memory.eeprom.settings.autoIp = true;
			else
				Memory.eeprom.settings.autoIp = false;
			request->arg("lan_ip").toCharArray(Memory.eeprom.settings.lanIp, request->arg("lan_ip").length() + 1);
			request->arg("gateway").toCharArray(Memory.eeprom.settings.gateway, request->arg("gateway").length() + 1);
			request->arg("subnet").toCharArray(Memory.eeprom.settings.subnet, request->arg("subnet").length() + 1);
			request->arg("ssid").toCharArray(Memory.eeprom.settings.wSSID, request->arg("ssid").length() + 1);			
			if (String(Memory.eeprom.settings.wSSID).length() > 0) {
				taskConnectWiFi.resume();
			}
			else {
				taskConnectWiFi.pause();
			}
			request->arg("key").toCharArray(Memory.eeprom.settings.wKey, request->arg("key").length() + 1);
			request->arg("assid").toCharArray(Memory.eeprom.settings.apSSID, request->arg("assid").length() + 1);
			goto save;
		}		
		/*if(request->hasArg("data")){
			DateTimeClass DateTime(request->arg("data"));
			Rtc.SetDateTime(DateTime.toRtcDateTime());
			request->send(200, TEXT_HTML, getDateTime());
			return;
		}*/
		if (request->hasArg("host")) {
			request->arg("host").toCharArray(Memory.eeprom.settings.hostUrl, request->arg("host").length() + 1);
			Memory.eeprom.settings.hostPin = request->arg("pin").toInt();
			request->arg("n_admin").toCharArray(Memory.eeprom.settings.login, request->arg("n_admin").length() + 1);
			request->arg("p_admin").toCharArray(Memory.eeprom.settings.password, request->arg("p_admin").length() + 1);
			request->arg("n_alarm").toCharArray(Memory.eeprom.settings.login_alarm, request->arg("n_alarm").length() + 1);
			request->arg("p_alarm").toCharArray(Memory.eeprom.settings.password_alarm, request->arg("p_alarm").length() + 1);
			goto save;
		}			
save:
		if (Memory.save()) {
			goto url;
		}
		return request->send(400);
	}
url:
#ifdef HTML_PROGMEM
	request->send_P(200, F("text/html"), settings_html);
#else
	if (request->url().equalsIgnoreCase("/sn"))
		request->send_P(200, F("text/html"), netIndex);
	else
		request->send(SPIFFS, request->url());
#endif
}

void handleSettingsJson(AsyncWebServerRequest * request) {
	if (!server.isAuthentified(request))
		return request->requestAuthentication();
	AsyncResponseStream *response = request->beginResponseStream("application/json");
	DynamicJsonBuffer jsonBuffer;
	JsonObject &root = jsonBuffer.createObject();
	JsonObject& settings = root.createNestedObject("settings");
	settings["id_auto"] = Memory.eeprom.settings.autoIp;	
	settings["id_lan_ip"] = Memory.eeprom.settings.lanIp;
	settings["id_gateway"] = Memory.eeprom.settings.gateway;
	settings["id_subnet"] = Memory.eeprom.settings.subnet;
	settings["id_ssid"] = String(Memory.eeprom.settings.wSSID);
	settings["id_key"] = String(Memory.eeprom.settings.wKey);
	settings["id_assid"] = Memory.eeprom.settings.apSSID;
	settings["id_host"] = String(Memory.eeprom.settings.hostUrl);
	settings["id_pin"] = Memory.eeprom.settings.hostPin;
	settings["id_n_admin"] = Memory.eeprom.settings.login;
	settings["id_p_admin"] = Memory.eeprom.settings.password;
	settings["id_n_alarm"] = Memory.eeprom.settings.login_alarm;
	settings["id_p_alarm"] = Memory.eeprom.settings.password_alarm;
	settings["bat_max"] = Memory.eeprom.settings.bat_max;
	settings["bat_min"] = Memory.eeprom.settings.bat_min;
	
	JsonObject& users = root.createNestedObject("users");
	
	
	root.printTo(*response);
	request->send(response);
}

void connectWifi() {
	taskConnectWiFi.pause();
	if (String(server.getSSID()).length() == 0) {
		WiFi.setAutoConnect(false);
		WiFi.setAutoReconnect(false);
		return;
	}
	if (WiFi.SSID().equals(server.getSSID())) {
		WiFi.begin();
		return;
	}	
	WiFi.disconnect(false);
	/*!  */
	int n = WiFi.scanComplete();
	if (n == -2) {
		WiFi.scanNetworksAsync(scanResultForConnect, true);
	}
	else if (n > 0) {
		scanResultForConnect(n);
	}
}

void scanResultForConnect(int networksFound) {
	for (int i = 0; i < networksFound; ++i) {
		if (WiFi.SSID(i).equals(server.getSSID())) {
			WiFi.persistent(true);
			WiFi.begin(server.getSSID(), server.getPASS());
			return;
		}
	}
	WiFi.scanDelete();
	taskConnectWiFi.resume();
}

void onStationModeConnected(const WiFiEventStationModeConnected& evt) {
	taskConnectWiFi.pause();
	WiFi.softAP(server.getApSSID(), SOFT_AP_PASSWORD, evt.channel);   //Устанавливаем канал как роутера
	WiFi.setAutoConnect(true);
	WiFi.setAutoReconnect(true);
	NBNS.begin(server.hostname().c_str());
	//BLINK->onRun(bind(&BlinkClass::blinkSTA, BLINK));
}

void onStationModeDisconnected(const WiFiEventStationModeDisconnected& evt) {
	//WiFi.scanDelete();
	//WiFi.scanNetworks(true);
	taskConnectWiFi.resume();
	//BLINK->onRun(bind(&BlinkClass::blinkAP, BLINK));
	//ws.closeAll();
}

void ICACHE_FLASH_ATTR printScanResult(int networksFound) {
	// sort by RSSI
	int n = networksFound;
	int indices[n];
	int skip[n];
	for (int i = 0; i < networksFound; i++) {
		indices[i] = i;
	}
	for (int i = 0; i < networksFound; i++) {
		for (int j = i + 1; j < networksFound; j++) {
			if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
				std::swap(indices[i], indices[j]);
				std::swap(skip[i], skip[j]);
			}
		}
	}
	DynamicJsonBuffer jsonBuffer;
	JsonObject &root = jsonBuffer.createObject();
	root["cmd"] = "ssl";
	JsonArray &scan = root.createNestedArray("list");
	for (int i = 0; i < 5 && i < networksFound; ++i) {
		JsonObject &item = scan.createNestedObject();
		item["ssid"] = WiFi.SSID(indices[i]);
		item["rssi"] = WiFi.RSSI(indices[i]);
	}
	size_t len = root.measureLength();
	AsyncWebSocketMessageBuffer *buffer = ws.makeBuffer(len);   //  creates a buffer (len + 1) for you.
	if(buffer) {
		root.printTo((char *)buffer->get(), len + 1);
		ws.textAll(buffer);
	}
	WiFi.scanDelete();
}

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) {	
	if (type == WS_EVT_CONNECT) {	
		if (server->count() > 4) {
			client->text("{\"cmd\":\"cls\",\"code\":1111}");
		}
	}
	else if (type == WS_EVT_DISCONNECT) {
		//client->close(true);
	}
	else if (type == WS_EVT_ERROR) {
		client->close(true);
	}
	else if (type == WS_EVT_DATA) {
		String msg = "";
		for (size_t i = 0; i < len; i++) {
			msg += (char) data[i];
		}
		DynamicJsonBuffer jsonBuffer;
		JsonObject &root = jsonBuffer.parseObject(msg);
		if (!root.success()) {
			return;
		}		
		const char *command = root["cmd"]; /* Получить показания датчика*/
		JsonObject& json = jsonBuffer.createObject();
		json["cmd"] = command;
		if (strcmp(command, "scan") == 0)
		{
			WiFi.scanNetworksAsync(printScanResult, true);
			return;
		}
		else if (strcmp(command, "creat") == 0)
		{
			const char *phone = root["phone"];
			Alarm.createClient(phone);	
		}
		else if (strcmp(command, "remove") == 0)
		{
			const char *phone = root["phone"];
			Alarm.removeClient(Alarm.hashClient(phone));
		}
		else if (strcmp(command, "binfo") == 0)
		{
			BATTERY->doInfo(json);
		}
		else if (strcmp(command, "status") == 0)
		{
			Alarm.doStatus(json);	
		}
		else if (strcmp(command, "safe") == 0)
		{
			Alarm.safe(root["val"]);
			return;
		}
		else {
			return;
		}
		size_t lengh = json.measureLength();
		AsyncWebSocketMessageBuffer * buffer = ws.makeBuffer(lengh);
		if (buffer) {
			json.printTo((char *)buffer->get(), lengh + 1);
			if (client) {
				client->text(buffer);
			}
			else {
				delete buffer;
			}
		}
	}
}