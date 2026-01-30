#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <libserialport.h>
#include <stdint.h>
#include "crsf.h"


// CRC8 table-based for performance (poly: 0xD5, init: 0x00)
uint8_t crc8_dvb_s2(uint8_t *data, int len) {
    uint8_t crc = 0x00;
    for (int i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80) crc = (crc << 1) ^ 0xD5;
            else crc <<= 1;
        }
    }
    return crc;
}

// Convert PWM to CRSF ticks
void us_to_ticks(int pwm_values[], uint16_t ticks[]) {
    for (int i = 0; i < 16; i++) {
        ticks[i] = (uint16_t)((pwm_values[i] - 1500) * 1.6 + 992);
    }
}

// Pack 16x11-bit channels into 22 bytes
void pack_channels(uint16_t *channels, uint8_t *packed) {
    memset(packed, 0, 22);
    int bit_index = 0;
    for (int i = 0; i < 16; i++) {
        uint16_t val = channels[i] & 0x07FF;
        int byte_index = bit_index / 8;
        int bit_offset = bit_index % 8;

        packed[byte_index] |= val << bit_offset;
        if (bit_offset > 5) {
            packed[byte_index + 1] = val >> (8 - bit_offset);
            packed[byte_index + 2] = val >> (16 - bit_offset);
        } else if (bit_offset > -1 && bit_offset <= 5) {
            packed[byte_index + 1] |= val >> (8 - bit_offset);
        }
        bit_index += 11;
    }
}

// Fill full CRSF frame with header, payload, CRC
void update_frame(int pwm[], uint8_t *frame_out) {
    uint16_t rc[16];
    uint8_t packed[22];

    us_to_ticks(pwm, rc);
    pack_channels(rc, packed);

    frame_out[0] = 0xC8;  // Device address
    frame_out[1] = 24;    // Payload length (22 + 1 type + 1 CRC)
    frame_out[2] = 0x16;  // RC Channels Packed type
    memcpy(&frame_out[3], packed, 22);
    frame_out[25] = crc8_dvb_s2(&frame_out[2], 23);  // CRC over type+payload
}

struct transmit_args {
    struct sp_port *port;
    uint8_t *message;
};

void *transmit(void *arg) {
    struct transmit_args *args = (struct transmit_args *)arg;
    while (1) {
        sp_blocking_write(args->port, args->message, 26, 100);
        usleep(6667);  // ~150Hz
    }
    return NULL;
}

static inline int16_t to_signed16(const uint8_t *data) {
    return (int16_t)(data[0] | (data[1] << 8));
}

static inline int32_t to_signed32(const uint8_t *data) {
    return (int32_t)(data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24));
}

static inline int8_t to_signed_byte(uint8_t byte) {
    return (int8_t)byte;
}


void *decode_telemetry(void *arg) {
    struct sp_port *port = (struct sp_port *)arg;
    uint8_t buffer[128];
    int buffer_index = 0;

    while (1) {
        uint8_t byte;
        int n = sp_blocking_read(port, &byte, 1, 100);
        if (n <= 0) continue;

        buffer[buffer_index++] = byte;

        if (buffer_index >= 2) {
            uint8_t frame_len = buffer[1];
            if (buffer_index >= frame_len + 2) {
                uint8_t type = buffer[2];
                const uint8_t *data = &buffer[0];  // full frame if needed
                const uint8_t *payload = &buffer[3]; // only payload

                switch (type) {
                    case PACKET_GPS: {
                        int32_t lat = to_signed32(&data[3]);
                        int32_t lon = to_signed32(&data[7]);
                        int16_t gspd = to_signed16(&data[11]);
                        int16_t hdg  = to_signed16(&data[13]);
                        int16_t alt  = to_signed16(&data[15]);
                        uint8_t sats = data[17];

                        printf("GPS: Pos=%.7f %.7f GSpd=%.1f Hdg=%.1f Alt=%dm Sats=%d\n",
                               lat / 1e7, lon / 1e7, gspd / 36.0, hdg / 100.0, alt - 1000, sats);
                        break;
                    }

                    case PACKET_VARIO: {
                        int16_t vspd = to_signed16(&data[3]);
                        printf("Vario: %.1fm/s\n", vspd / 10.0);
                        break;
                    }

                    case PACKET_ATTITUDE: {
                        int16_t pitch = to_signed16(&data[3]);
                        int16_t roll  = to_signed16(&data[5]);
                        int16_t yaw   = to_signed16(&data[7]);
                        printf("Attitude: Pitch=%.2f Roll=%.2f Yaw=%.2f\n",
                               pitch / 10000.0, roll / 10000.0, yaw / 10000.0);
                        break;
                    }

                    case PACKET_BARO_ALT: {
                        int32_t alt = to_signed32(&data[3]);
                        printf("Baro Altitude: %.2fm\n", alt / 100.0);
                        break;
                    }

                    case PACKET_LINK_STATISTICS: {
                        int8_t rssi1 = to_signed_byte(data[3]);
                        int8_t rssi2 = to_signed_byte(data[4]);
                        uint8_t lq   = data[5];
                        int8_t snr   = to_signed_byte(data[6]);
                        printf("Link: RSSI=%d/%d LQ=%d SNR=%d\n", rssi1, rssi2, lq, snr);
                        break;
                    }

                    case PACKET_BATTERY_SENSOR: {
                        float vbat = to_signed16(&data[3]) / 10.0;
                        float curr = to_signed16(&data[5]) / 10.0;
                        int mah = (data[7] << 16) | (data[8] << 7) | data[9];
                        int pct = data[10];
                        printf("Battery: %.2fV %.1fA %dmAh %d%%\n", vbat, curr, mah, pct);
                        break;
                    }

                    default:
                        printf("Unhandled packet type: 0x%02X\n", type);
                        break;
                }

                buffer_index = 0;  // reset for next frame
            }
        }

        if (buffer_index >= sizeof(buffer)) {
            buffer_index = 0;
        }
    }

    return NULL;
}

// Main thread

int main() {

    struct sp_port *port;
    if (sp_get_port_by_name("/dev/ttyS1", &port) != SP_OK || sp_open(port, SP_MODE_READ_WRITE) != SP_OK) {
        fprintf(stderr, "Failed to open /dev/ttyS1\n");
        return 1;
    }

    sp_set_baudrate(port, 420000);
    sp_set_bits(port, 8);
    sp_set_parity(port, SP_PARITY_NONE);
    sp_set_stopbits(port, 1);
    sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE);

    uint8_t *message = malloc(26);
    int arm_pwm[16]    = {1500, 1500, 1500, 1000, 1800, 1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500};
    int disarm_pwm[16] = {1500, 1500, 1500, 1000, 1000, 1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500};

    update_frame(disarm_pwm, message);

    struct transmit_args args = {port, message};
    pthread_t tx_thread, rx_thread;
    pthread_create(&tx_thread, NULL, transmit, &args);
    pthread_create(&rx_thread, NULL, decode_telemetry, port);

    // Example loop. You should really think what you want to do from this point. Good Luck! 
    while (1) {
        sleep(1);
        update_frame(arm_pwm, message);
        sleep(1);
        update_frame(disarm_pwm, message);
    }

    sp_close(port);
    sp_free_port(port);
    free(message);
    return 0;
}
