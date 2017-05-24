#ifndef MQTT_H
#define MQTT_H

#include <string_view>
#include <cstddef>
#include <arpa/inet.h>
#include <cstring>

enum MQTT {
    CONNECT = 1,
    CONNACK,
    PUBLISH,
    PUBACK,
    PUBREC,
    PUBREL,
    PUBCOMP,
    SUBSCRIBE,
    SUBACK,
    UNSUBSCRIBE,
    UNSUBACK,
    PINGREQ,
    PINGRESP,
    DISCONNECT
};

size_t getPublishLength(std::string_view topic, std::string_view data) {
    return 2 + 2 + topic.length() + data.length();
}

void formatPublish(unsigned char *dst, std::string_view topic, std::string_view data) {
    unsigned char payloadLength = 2 + topic.length() + data.length();

    dst[0] = 48;
    dst[1] = payloadLength;

    uint16_t topicLength = htons(topic.length());
    memcpy(dst + 2, &topicLength, 2);
    memcpy(dst + 4, topic.data(), topic.length());
    memcpy(dst + 4 + topic.length(), data.data(), data.length());
}

size_t getSubscribeLength(std::string_view topic) {
    return 2 + 2 + 2 + topic.length() + 1;
}

void formatSubscribe(unsigned char *dst, std::string_view topic) {
    unsigned char payloadLength = 2 + 2 + topic.length() + 1;

    dst[0] = 130;
    dst[1] = payloadLength;

    uint16_t packetId = 1234;
    memcpy(dst + 2, &packetId, 2);

    uint16_t topicLength = htons(topic.length());
    memcpy(dst + 4, &topicLength, 2);
    memcpy(dst + 6, topic.data(), topic.length());
    dst[payloadLength + 1] = 0; // qos
}

size_t getConnectLength(std::string_view name) {
    return 14 + name.length();
}

void formatConnect(unsigned char *dst, std::string_view name) {
    dst[0] = 16;
    dst[1] = 12 + name.length(); // payload length
    dst[2] = 0; //version
    dst[3] = 4; // version
    dst[4] = 'M';
    dst[5] = 'Q';
    dst[6] = 'T';
    dst[7] = 'T';

    dst[8] = 4;
    dst[9] = 2;
    dst[10] = 0;
    dst[11] = 60;

    uint16_t nameLength = htons(name.length());
    memcpy(dst + 12, &nameLength, 2);

    memcpy(dst + 14, name.data(), name.length());
}

#endif // MQTT_H
