#pragma once
#include <ESPAsyncWebServer.h>

const char successResponse[] PROGMEM = R"(<meta http-equiv='refresh' content='15;URL=/'>���������� �������! �� ���������� ���� �� ������������...)";

const char serverIndex[] PROGMEM = R"(<!DOCTYPE html><html> <head> <meta name='viewport' content='width=device-width, initial-scale=1, maximum-scale=1.0, user-scalable=no'/> <link rel='stylesheet' type='text/css' href='global.css'> </head> <body> <form method='POST' action='' enctype='multipart/form-data'> <input type='file' name='update'> <input type='submit' value='Update'> </form> </body></html>)";

class HttpUpdaterClass: public AsyncWebHandler{
	public:
		HttpUpdaterClass(const String& username=String(), const String& password=String());	
				
		void setAuthenticated(bool a){_authenticated = a;};
		bool getAuthenticated(){return _authenticated;};
		void handleHttpStartUpdate(AsyncWebServerRequest*);
		void setUpdaterError();
		virtual bool canHandle(AsyncWebServerRequest *request) override final;
		virtual void handleRequest(AsyncWebServerRequest *request) override final;
		virtual void handleUpload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) override final;	
		virtual bool isRequestHandlerTrivial() override final {return false;}

	protected:
		String _updaterError;
		int _command;
	private:				
		String _username;
		String _password;
		bool _authenticated;
};