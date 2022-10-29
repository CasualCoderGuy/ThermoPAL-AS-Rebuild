#include "grbl.h"
//#define DEBUGGER

#define COMP_FLAG_INSTALLED     bit(1)
#define COMP_FLAG_HEX           bit(2)

#define COMP_TYPE_COLLECT       bit(1)
#define COMP_TYPE_TIP_HOLDER    bit(2)
#define COMP_TYPE_TIP_REMOVE    bit(3)
#define COMP_TYPE_INJECTOR      bit(4)
#define COMP_TYPE_WASH_STATION  bit(5)
#define COMP_TYPE_WASTE         bit(6)

#define SYR_FLAG_REMOVABLE_TIP  bit(1)
#define SYR_FLAG_LOOP           bit(2)
#define SYR_FLAG_PLASTIC        bit(3)
#define SYR_FLAG_LUER           bit(4)
#define SYR_FLAG_LOCK           bit(5)
#define SYR_FLAG_EXTENDED       bit(6)
#define SYR_FLAG_HAS_TIP        bit(7)

#define ENABLE_NEEDLE_WASH      bit(3)
#define ENABLE_INJECTOR_WASH    bit(4)
#define INJECT_TYPE_NOWASTE     bit(5)
#define INJECT_TYPE_PARTIAL     bit(6)
#define INJECT_TYPE_FULL        bit(7)


#define DEFAULT_TRAVEL_SPEED        DEFAULT_AXIS1_MAX_RATE/2 // mm/s
#define DEFAULT_PUNCTURE_SPEED      DEFAULT_TRAVEL_SPEED/5 // mm/s
#define DEFAULT_SYRINGE_SPEED       DEFAULT_AXIS4_MAX_RATE/50/60*DEFAULT_AXIS4_STEPS_PER_UNIT*syringe.volume_per_step // uL/s
#define DEFAULT_TIP_PICKUP_SPEED    DEFAULT_TRAVEL_SPEED/10
#define DEFAULT_DELAY_MS            50
#define DEFAULT_NEEDLE_PENETRATION  0
#define DEFAULT_AIR_VOLUME          0
#define DEFAULT_FILL_STROKES        0

#define MAX_COMPONENTS 12
#define DEFAULT_WASH_STATION_COMP 1
#define DEFAULT_TIP_REMOVE_COMP 2
#define DEFAULT_INJECTOR_COMP 8
#define DEFAULT_WASTE 2
#define DEFAULT_WASH1 1
#define DEFAULT_WASH2 3

#define DEFAULT_LC_SIG_PULSE_LENGTH_MS 200

struct Component
{
    //char* comp_info[20];
    uint8_t flags;
    uint8_t type;
    float coordinate_zero[3]; //first vial x, y, z
    float offset[3]; //last vial offset x, y; needle penetration
    uint8_t dimensions[2]; //no. of vials x, y
} components[MAX_COMPONENTS];

struct Syringe
{
    //char* syr_info[20];
    uint8_t flags; //removable tip
    float max_volume;
    float volume_per_step; //uL/step
    float overfill;
    float needle_offset;
} syringe;


struct AS_block
{
    uint8_t target_comp; // comp_no-1 !
    uint8_t target_offset[2]; // x, y
    float speed; // mm/s

    float needle_penetration; // mm
    float volume; // uL
    float air_volume; // uL
    float excess_volume;
    float puncture_speed; // mm/s
    float volume_speed; // uL/s
    uint16_t delay_ms;
    uint8_t fill_strokes; //(for volatiles)!!!
//  uint8_t flags;
} as_command;

struct FC_block
{
    uint8_t target_comp;
    uint8_t current_vial_offset[2]; //init to 1, 1
    // int signal_delay; // ms
    // float flowrate; // uL/s
    // float max_vol; // uL
} fc_data;

struct SyrCycle_block
{
    uint8_t cycles;
    float idle_seconds;
    uint8_t station;
    float syringe_purge_seconds;
} syrCycle_data;

// struct WashRoutine_block
// {   uint8_t flags;
//     uint8_t target_comp; // comp_no-1 !
//     uint8_t target_offset[2]; // x, y
//     float speed; // mm/s
//     float needle_penetration; // mm
//     float volume; // uL
//     float puncture_speed; // mm/s
//     float volume_speed; // uL/s
//     uint16_t delay_ms;
//     struct SyrCycle_block syrCycleRoutine;
// } washRoutine;

uint16_t current_tip;

bool AddComponent(uint8_t comp_no, float* coordinate_zero, float* offset, uint8_t* dimensions);

bool CopyComponent(uint8_t copy, uint8_t paste, uint8_t data);

bool CheckCoordinatesIsInRange(float* coordinate_zero, float* offset, uint8_t* dim);

void TeachZero(uint8_t comp_no);

void TeachOffset(uint8_t comp_no);

void TeachNeedlePenetration(uint8_t comp_no);

void SetNeedlePenetration(uint8_t comp_no, float* offset);

void report_components(uint16_t compToReport);

void report_position(uint8_t axisToReport);

void report_syringe();

void report_limits();

void report_sigs();

void send_aux();

void send_pos();

void send_state();

uint8_t ParseTarget(char* line, uint8_t char_counter, uint8_t flags_to_check, uint8_t mandatory_values, bool enable_overfill);

void GoToTargetCoordinates();

void FastMoveXY();

void GoHome(uint8_t axisflags);

uint8_t InitFC();

void write_component_settings(uint8_t comp_no);

bool read_component_settings(uint8_t comp_no);

void write_syringe_settings();

bool read_syringe_settings();

void reset_components();

void reset_syringe();

void set_needle_offset(float offset_mm);

uint8_t fc_valve_get_state();

void fc_valve_waste();

void fc_valve_collect();

void fc_valve_toggle();

void injector_valve_load();

void injector_valve_inject();

void injector_valve_toggle();

uint8_t lc_sig_evt_out_get_state();

void lc_sig_evt_out_send(uint8_t pins, uint8_t mask, int pulse_length);

bool init_autosampler();