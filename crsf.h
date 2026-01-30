// crsf.h
#ifndef CRSF_H
#define CRSF_H

#include <stdint.h>
#include <pthread.h>
#include <libserialport.h>

// Define packet types

enum PacketTypes {
    PACKET_GPS = 0x02,
    PACKET_VARIO = 0x07,
    PACKET_BATTERY_SENSOR = 0x08,
    PACKET_BARO_ALT = 0x09,
    PACKET_HEARTBEAT = 0x0B,
    PACKET_VIDEO_TRANSMITTER = 0x0F,
    PACKET_LINK_STATISTICS = 0x14,
    PACKET_RC_CHANNELS_PACKED = 0x16,
    PACKET_ATTITUDE = 0x1E,
    PACKET_FLIGHT_MODE = 0x21,
    PACKET_DEVICE_INFO = 0x29,
    PACKET_CONFIG_READ = 0x2C,
    PACKET_CONFIG_WRITE = 0x2D,
    PACKET_RADIO_ID = 0x3A
};

// Define structs for each packet type
typedef struct {
    float latitude;
    float longitude;
    float speed;
    float heading;
    int altitude;
    uint8_t satellites;
} CRSF_GPSData;

typedef struct {
    int pitch;
    int roll;
    int yaw;
} AttitudeData;

typedef struct {
    int vspd;
} VarioData;

typedef struct {
    float voltage;
    float current;
    int capacity;
    uint8_t percentage;
} BatteryData;

typedef struct {
    float altitude;
} BaroData;

typedef struct {
    int rssi1;
    int rssi2;
    uint8_t lq;
    int snr;
} LinkStatsData;

int main(void);

#endif