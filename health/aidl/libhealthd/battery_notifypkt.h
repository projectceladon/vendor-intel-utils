#include <stdio.h>
#include <stdlib.h>
#include <healthd/BatteryMonitor.h>

#define BASE_PATH "/sys/class/power_supply/test_battery/"
#define INTELIPCID "INTELIPC"

using namespace android;
bool get_battery_mediation_present(void);
bool get_battery_properties(android::BatteryProperties *props);

struct header {
    uint8_t intelipc[9];
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

/* TODO Below structure is not getting used in this version of code.
 * Next version will be using this.
 *
 * Size of the variable is same as passed on the network buffer.
 * So any change in handling the below structure will not break
 * the client code.
 */
struct battery_sysfs {
    uint8_t intelipc[8];
    uint8_t noftify_id[2];
    uint8_t length[2];
    
    union   {
        struct initial_pkt initpkt;
        struct monitor_pkt monitorpkt;
    };
};
