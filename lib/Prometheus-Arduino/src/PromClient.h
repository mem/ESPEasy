#ifndef promclient_h
#define promclient_h

#include <HTTPClient.h>
#include "WriteRequest.h"
#include "PromDebug.h"

static const char PromUserAgent[] PROGMEM = "prom-arduino/0.2.2";

class PromClient {
public:
    PromClient();
    PromClient(WiFiClientSecure * client);
    ~PromClient();

    enum SendResult {
        SUCCESS,
        FAILED_RETRYABLE,
        FAILED_DONT_RETRY
    };

    void setUrl(const char* url);
    void setUser(const char* user);
    void setPass(const char* pass);
    void setRootCA(const char* rootCA);

    void setDebug(Stream& stream);

    void setClient(WiFiClientSecure* client);
    uint16_t getConnectCount();

    bool begin();
    SendResult send(WriteRequest& req);

    char* errmsg;

protected:
    Stream* _debug = nullptr;
    WiFiClientSecure* _client = nullptr;
    HTTPClient* _httpClient = nullptr;

    String _url;
    const char* _user;
    const char* _pass;
    const char* _rootCA;
    uint16_t _connectCount = 0;

    SendResult _send(uint8_t* entry, size_t len);
};


#endif
