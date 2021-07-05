#include "PromClient.h"

PromClient::PromClient() {};
PromClient::PromClient(WiFiClientSecure* client) : _client(client) {};
PromClient::~PromClient() {
    if (_httpClient) {
        delete _httpClient;
    }
};

void PromClient::setUrl(const char* url) {
    _url = url;
};
void PromClient::setUser(const char* user) {
    _user = user;
};
void PromClient::setPass(const char* pass) {
    _pass = pass;
};

void PromClient::setRootCA(const char* rootCA) {
    _rootCA = rootCA;
}


void PromClient::setDebug(Stream& stream) {
    _debug = &stream;
};

void PromClient::setClient(WiFiClientSecure* client) {
    _client = client;
}

uint16_t PromClient::getConnectCount() {
    return _connectCount;
}


bool PromClient::begin() {
    errmsg = nullptr;
    if (_url.isEmpty()) {
        errmsg = (char*)"you must set a url with setUrl()";
        return false;
    }

    if (!_client) {
        errmsg = (char*)"you must set a client with setClient() first";
        return false;
    }

    // Create the HttpClient
    _httpClient = new HTTPClient();
    _httpClient->setTimeout(15000);
    // XXX(mem): is this available?
    // _httpClient->setHttpResponseTimeout(15000);
    _httpClient->setReuse(true);
    return true;
};

PromClient::SendResult PromClient::send(WriteRequest& req) {
    errmsg = nullptr;
    uint8_t buff[req.getBufferSize()] = { 0 };
    int16_t len = req.toSnappyProto(buff);
    if (len <= 0) {
        errmsg = req.errmsg;
        return PromClient::SendResult::FAILED_DONT_RETRY;
    }
    return _send(buff, len);
};

PromClient::SendResult PromClient::_send(uint8_t* entry, size_t len) {
    DEBUG_PRINTLN("Sending To Prometheus");

#if 0
    // Make a HTTP request:
    if (_client->connected()) {
        DEBUG_PRINTLN("Connection already open");
    }
    else {
        DEBUG_PRINTLN("Connecting...");
        if (!_client->connect(_url, _port)) {
            DEBUG_PRINTLN("Connection failed");
            if (_client->getWriteError()) {
                DEBUG_PRINT("Write error on client: ");
                DEBUG_PRINTLN(_client->getWriteError());
                _client->clearWriteError();
            }
            errmsg = (char*)"Failed to connect to server, enable debug logging for more info";
            return PromClient::SendResult::FAILED_RETRYABLE;
        }
        else {
            DEBUG_PRINTLN("Connected.")
            _connectCount++;
        }
    }
#endif

    // Do a lot of this manually to avoid sending headers and things we don't want to send
    if (_rootCA[0] != 0) {
        _client->setCACert(_rootCA);
    }
    _httpClient->useHTTP10(false);
    _httpClient->setAuthorization(_user, _pass);
    _httpClient->setUserAgent(PromUserAgent);
    _httpClient->begin(*_client, _url);
    _httpClient->addHeader(F("Content-Type"), F("application/x-protobuf"));
    _httpClient->addHeader(F("Content-Encoding"), F("snappy"));
    int respCode = _httpClient->POST(entry, len);
    String response = _httpClient->getString();

    DEBUG_PRINT("request sent... code = ");
    DEBUG_PRINT(respCode);
    DEBUG_PRINT(" response = ");
    DEBUG_PRINT(response.c_str());
    DEBUG_PRINTLN("");

    _httpClient->end();

#if 0
    uint8_t waitAttempts = 0;
    // The default wait in responseStatusCode is 1s which means the minimum return is at least 1s if data
    // is not immediately available. So instead we will loop for data for the first second allowing us 
    // to check for data much quicker
    while (!_client->available() && waitAttempts < 10) {
        delay(100);
        waitAttempts++;
    }

    int statusCode = _httpClient->responseStatusCode();
    if (statusCode == HTTP_ERROR_TIMED_OUT) {
        errmsg = (char*)"Timed out connecting to Loki";
        return PromClient::SendResult::FAILED_RETRYABLE;
    }
    if (statusCode == HTTP_ERROR_INVALID_RESPONSE) {
        errmsg = (char*)"Invalid response from server, correct address and port?";
        return PromClient::SendResult::FAILED_RETRYABLE;
    }
    int statusClass = statusCode / 100;
    if (statusClass == 2) {
        DEBUG_PRINTLN("Prom Send Succeeded");
        // We don't use the _httpClient->responseBody() method both because it allocates a string
        // and also because it doesn't understand a 204 response code not having a content-length
        // header and will wait until a timeout for additional bytes.
        while (_client->available()) {
            char c = _client->read();
            DEBUG_PRINT(c);
        }
    }
    else if (statusClass == 4) {
        DEBUG_PRINT("Prom Send Failed with code: ");
        DEBUG_PRINTLN(statusCode);
        while (_client->available()) {
            char c = _client->read();
            DEBUG_PRINT(c);
        }
        errmsg = (char*)"Failed to send to prometheus, 4xx response";
        return PromClient::SendResult::FAILED_DONT_RETRY;
    }
    else {
        DEBUG_PRINT("Prom Send Failed with code: ");
        DEBUG_PRINTLN(statusCode);
        while (_client->available()) {
            char c = _client->read();
            DEBUG_PRINT(c);
        }
        errmsg = (char*)"Failed to send to prometheus, 5xx or unexpected status code";
        return PromClient::SendResult::FAILED_RETRYABLE;
    }
#endif


    return PromClient::SendResult::SUCCESS;
};
