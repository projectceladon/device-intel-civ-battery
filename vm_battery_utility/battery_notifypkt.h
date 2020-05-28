#include <stdio.h>
#include <stdlib.h>

#define INTELIPCID "INTELIPC"

#define MODEL_NAME "model_name"
#define SERIAL_NUMBER "serial_number"
#define MANUFACTURER "manufacturer"
#define TECHNOLOGY "technology"
#define TYPE "type"
#define PRESENT "present"
#define CAPACITY "capacity"
#define CHARGE_FULL_DESIGN "charge_full_design"
#define HEALTH "health"
#define TEMP "temp"
#define CHARGE_NOW "charge_now"
#define TIME_TO_EMPTY_AVG "time_to_empty_avg"
#define CHARGE_FULL "charge_full"
#define TIME_TO_FULL_NOW "time_to_full_now"
#define VOLTAGE_NOW "voltage_now"
#define CHARGE_TYPE "charge_type"
#define CAPACITY_LEVEL "capacity_level"
#define STATUS "status"

struct header {
	uint8_t intelipc[8];
	uint16_t notify_id;
	uint16_t length;
};

struct initial_pkt {
	uint8_t model_name[28];
	uint8_t serial_number[52];
	uint8_t manufacturer[24];
	uint8_t technology[8];
	uint8_t type[8];
	uint8_t present[4];
};

struct monitor_pkt {
	uint32_t capacity;
	uint32_t charge_full_design;
	uint32_t temp;
	uint32_t charge_now;
	uint32_t time_to_empty_avg;
	uint32_t charge_full;
	uint32_t time_to_full_now;
	uint32_t voltage_now;
	uint8_t charge_type[12];
	uint8_t capacity_level[12];
	uint8_t status[12];
	uint8_t health[24];
};
