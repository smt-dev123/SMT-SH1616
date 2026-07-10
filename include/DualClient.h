#ifndef DUAL_CLIENT_H
#define DUAL_CLIENT_H

#include <Client.h>
#include <WiFiClient.h>
#include <Ethernet.h>

extern bool isEthernetConnected;

class DualClient : public Client {
private:
    WiFiClient wifi;
    EthernetClient eth;

    Client* getActiveClient() {
        if (isEthernetConnected) {
            return &eth;
        }
        return &wifi;
    }

public:
    DualClient() {}

    int connect(IPAddress ip, uint16_t port) override { return getActiveClient()->connect(ip, port); }
    int connect(const char *host, uint16_t port) override { return getActiveClient()->connect(host, port); }
    size_t write(uint8_t b) override { return getActiveClient()->write(b); }
    size_t write(const uint8_t *buf, size_t size) override { return getActiveClient()->write(buf, size); }
    int available() override { return getActiveClient()->available(); }
    int read() override { return getActiveClient()->read(); }
    int read(uint8_t *buf, size_t size) override { return getActiveClient()->read(buf, size); }
    int peek() override { return getActiveClient()->peek(); }
    void flush() override { getActiveClient()->flush(); }
    void stop() override { getActiveClient()->stop(); }
    uint8_t connected() override { return getActiveClient()->connected(); }
    operator bool() override { return getActiveClient()->operator bool(); }
};

#endif
