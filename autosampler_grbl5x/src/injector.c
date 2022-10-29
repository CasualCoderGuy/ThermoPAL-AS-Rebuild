#include "grbl.h"

//liquid drawup styles from manual!!!, set as_command before calling: all!!!; vial sensor!; call automatically when fraccol/on ready signal event in
void ModSyringeContent(uint8_t transfer_type) // type bits: 0-no_injection; 3-enable_needle wash; 4-enable_injector_wash; 5-inject_nowaste; 6-inject_partial; 7-inject_full 
{
    GoToTargetCoordinates();
    float target_coord[N_AXIS];
    plan_line_data_t pl_data;
    pl_data.feed_rate = as_command.puncture_speed;
    for (uint8_t i = 0; i < N_AXIS; i++)
        target_coord[i] = sys_position[i]/settings.steps_per_mm[i];
    if (as_command.air_volume != 0)
    {
        pl_data.feed_rate = as_command.volume_speed/syringe.volume_per_step/settings.steps_per_mm[3]*60;
        target_coord[3] = sys_position[3]/settings.steps_per_mm[3]+(as_command.air_volume/syringe.volume_per_step/settings.steps_per_mm[3]);
        delay_ms(as_command.delay_ms);
        move_execute(target_coord, &pl_data);
    }
    target_coord[2] = components[as_command.target_comp].coordinate_zero[2]+as_command.needle_penetration; //lower z
    delay_ms(as_command.delay_ms);
    move_execute(target_coord, &pl_data);
    if (transfer_type & INJECT_TYPE_NOWASTE) { delay_ms(as_command.delay_ms); injector_valve_load(); }
    else if (transfer_type & (INJECT_TYPE_PARTIAL | INJECT_TYPE_FULL)) { delay_ms(as_command.delay_ms); injector_valve_inject(); }
    pl_data.feed_rate = as_command.volume_speed/syringe.volume_per_step/settings.steps_per_mm[3]*60;
    if (transfer_type & (INJECT_TYPE_PARTIAL | INJECT_TYPE_FULL))
        target_coord[3] = sys_position[3]/settings.steps_per_mm[3]+(as_command.excess_volume/2/syringe.volume_per_step/settings.steps_per_mm[3]);
    else
        target_coord[3] = sys_position[3]/settings.steps_per_mm[3]+(as_command.volume/syringe.volume_per_step/settings.steps_per_mm[3]); //draw/eject liquid
    delay_ms(as_command.delay_ms);
    move_execute(target_coord, &pl_data);
    if (transfer_type & INJECT_TYPE_NOWASTE) { delay_ms(as_command.delay_ms); injector_valve_inject(); lc_sig_evt_out_send(LC_EVT_SIG_OUT_BIT_INJECT, LC_EVT_SIG_OUT_BIT_INJECT, DEFAULT_LC_SIG_PULSE_LENGTH_MS); }
    else if (transfer_type & (INJECT_TYPE_PARTIAL | INJECT_TYPE_FULL))
    {
        delay_ms(as_command.delay_ms);
        injector_valve_load();
        target_coord[3] = sys_position[3]/settings.steps_per_mm[3]+(as_command.volume/syringe.volume_per_step/settings.steps_per_mm[3]);
        delay_ms(as_command.delay_ms);
        move_execute(target_coord, &pl_data);
        delay_ms(as_command.delay_ms);
        injector_valve_inject();
        lc_sig_evt_out_send(LC_EVT_SIG_OUT_BIT_INJECT, LC_EVT_SIG_OUT_BIT_INJECT, DEFAULT_LC_SIG_PULSE_LENGTH_MS);
        target_coord[3] = sys_position[3]/settings.steps_per_mm[3]+(as_command.excess_volume/2/syringe.volume_per_step/settings.steps_per_mm[3]);
        delay_ms(as_command.delay_ms);
        move_execute(target_coord, &pl_data);
    }
    if (transfer_type & ENABLE_INJECTOR_WASH) { delay_ms(as_command.delay_ms); SyringePurge(); delay_ms(as_command.delay_ms);}
    target_coord[2] = components[as_command.target_comp].coordinate_zero[2]; //pull up z
    pl_data.feed_rate = as_command.puncture_speed;
    delay_ms(as_command.delay_ms);
    move_execute(target_coord, &pl_data);
    if (as_command.air_volume != 0)
    {
        pl_data.feed_rate = as_command.volume_speed/syringe.volume_per_step/settings.steps_per_mm[3]*60;
        target_coord[3] = sys_position[3]/settings.steps_per_mm[3]+(as_command.air_volume/syringe.volume_per_step/settings.steps_per_mm[3]);
        delay_ms(as_command.delay_ms);
        move_execute(target_coord, &pl_data);
    }
}

void SyringeCycle(uint8_t cycle_type) //set as_command before calling!!!, if purge: OVERFILL?
{
    float target_coord[N_AXIS];
    plan_line_data_t pl_data;
    pl_data.feed_rate = as_command.puncture_speed;
    for (uint8_t i = 0; i < N_AXIS; i++)
        target_coord[i] = sys_position[i]/settings.steps_per_mm[i];
    target_coord[2] = components[as_command.target_comp].coordinate_zero[2]+as_command.needle_penetration; //lower z
    delay_ms(as_command.delay_ms);
    move_execute(target_coord, &pl_data);
    pl_data.feed_rate = as_command.volume_speed/syringe.volume_per_step/settings.steps_per_mm[3]*60;
    coolant_set_state(syrCycle_data.station); //mode = COOLANT_FLOOD_ENABLE / COOLANT_MIST_ENABLE / 0
    for (uint8_t i = 0; i < syrCycle_data.cycles; i++)
    {
        target_coord[3] = sys_position[3]/settings.steps_per_mm[3]+(as_command.volume/syringe.volume_per_step/settings.steps_per_mm[3]); //draw liquid
        delay_ms(as_command.delay_ms);
        move_execute(target_coord, &pl_data);
        target_coord[3] = sys_position[3]/settings.steps_per_mm[3]-(as_command.volume/syringe.volume_per_step/settings.steps_per_mm[3]); //eject liquid
        delay_ms(as_command.delay_ms);
        move_execute(target_coord, &pl_data);
    }
    if (cycle_type & ENABLE_INJECTOR_WASH) { delay_ms(as_command.delay_ms); SyringePurge();}
    if (cycle_type & ENABLE_NEEDLE_WASH) { delay_sec(syrCycle_data.idle_seconds, DELAY_MODE_DWELL); }
    if (syrCycle_data.station) {//turn off wash (coolant)
        coolant_set_state(0);
    }
    target_coord[2] = components[as_command.target_comp].coordinate_zero[2]; //pull up z
    pl_data.feed_rate = as_command.puncture_speed;
    delay_ms(as_command.delay_ms);
    move_execute(target_coord, &pl_data);
}

void SyringePurge()
{
    //turn on pump
    delay_sec(syrCycle_data.syringe_purge_seconds, DELAY_MODE_DWELL);
    //turn off pump
}

void Mix(uint8_t cycles) //M[cycle]...
{
    GoToTargetCoordinates();
    syrCycle_data.cycles = cycles;
    syrCycle_data.station = 0;
    SyringeCycle(0); //set cycle=X wash_time=0 station=0 purgetime=0
}

//+through ndl wash //W + (S1)/(1) station/comp + C cycle + P purge + I idle + M motor
// void WashInVial() 
// {
//     GoToTargetCoordinates();
//     SyringeCycle(); //set cycle=0 wash_time=X station=0  purgetime=?
// }
// void WashInStation()
// {
//     //set target wash station
//     GoToTargetCoordinates();
//     SyringeCycle(); //set cycle=0 wash_time=X station=X  purgetime=?
// }
// void PurgeInStation()
// {
//     //set target wash station
//     GoToTargetCoordinates();
//     SyringeCycle(); //set cycle=X wash_time=X station=X  purgetime=?
// }
// void NeedleWash(uint8_t seconds, uint8_t station) //set as_command before calling: all
// {
//     GoToTargetCoordinates();
//     float target_coord[N_AXIS];
//     plan_line_data_t pl_data;
//     pl_data.feed_rate = as_command.puncture_speed;
//     for (uint8_t i = 0; i < N_AXIS; i++)
//         target_coord[i] = sys_position[i]/settings.steps_per_mm[i];
//     target_coord[2] = components[as_command.target_comp].coordinate_zero[2]+as_command.needle_penetration; //lower z
//     delay_ms(as_command.delay_ms);
//     move_execute(target_coord, &pl_data);
//     coolant_set_state(station);
//     delay_sec(seconds, DELAY_MODE_DWELL);
//     coolant_set_state(0);
//     target_coord[2] = components[as_command.target_comp].coordinate_zero[2]; //pull up z
//     delay_ms(as_command.delay_ms);
//     move_execute(target_coord, &pl_data);
// }

void Aspirate()
{
    float target_coord[N_AXIS];
    plan_line_data_t pl_data;
    pl_data.feed_rate = as_command.volume_speed/syringe.volume_per_step/settings.steps_per_mm[3]*60;
    for (uint8_t i = 0; i < N_AXIS; i++)
        target_coord[i] = sys_position[i]/settings.steps_per_mm[i];
    target_coord[3] = sys_position[3]/settings.steps_per_mm[3]+(as_command.volume/syringe.volume_per_step/settings.steps_per_mm[3]);
    move_execute(target_coord, &pl_data);
}

uint8_t Inject(uint8_t transfer_type)
{
    uint8_t inject_comp = -1;
    for (uint8_t i = 0; i < MAX_COMPONENTS; i++) {
        if((components[i].flags & COMP_FLAG_INSTALLED) && (components[i].type == COMP_TYPE_INJECTOR)) {
        inject_comp = i;
        break;
        }
    }
    if (inject_comp == -1) return STATUS_REQUESTED_COMPONENT_TYPE_NOT_INSTALLED;
    ModSyringeContent(0);
    if (transfer_type & ENABLE_NEEDLE_WASH)
    //wash
    as_command.target_comp = inject_comp;
    as_command.target_offset[0] = 1;
    as_command.target_offset[1] = 1;
    as_command.needle_penetration = components[inject_comp].offset[2];
    //as_command.delay_ms = 200;
    //as_command.puncture_speed = DEFAULT_PUNCTURE_SPEED;
    //as_command.volume_speed = DEFAULT_SYRINGE_SPEED;
    ModSyringeContent(transfer_type);
    return STATUS_OK;
}

uint8_t PickUpTip(uint16_t tip_no)
{
    if ((syringe.flags ^ SYR_FLAG_REMOVABLE_TIP) & (SYR_FLAG_REMOVABLE_TIP | SYR_FLAG_HAS_TIP)) { return STATUS_SETTING_DISABLED; } //STATUS_SYRINGE_TIP_ERROR
    uint8_t tip_holder_comp = -1;
    for (uint8_t i = 0; i < MAX_COMPONENTS; i++) {
        if((components[i].flags & COMP_FLAG_INSTALLED) && (components[i].type == COMP_TYPE_TIP_HOLDER)) {
        tip_holder_comp = i;
        break;
        }
    }
    if (tip_holder_comp == -1) return STATUS_REQUESTED_COMPONENT_TYPE_NOT_INSTALLED;
    as_command.target_comp = tip_holder_comp;
    if (tip_no == 0) 
    { 
        tip_no = current_tip; 
        if (tip_no > components[tip_holder_comp].dimensions[0]*components[tip_holder_comp].dimensions[1]) { current_tip = 1; tip_no = 1; }
        else { current_tip++; }
    }
    else if (tip_no > components[tip_holder_comp].dimensions[0]*components[tip_holder_comp].dimensions[1]) { return STATUS_GCODE_COMPONENT_OUT_OF_RANGE; }
    tip_no--;
    as_command.target_offset[0] = (tip_no % components[tip_holder_comp].dimensions[0])+1;
    as_command.target_offset[1] = trunc(tip_no/components[tip_holder_comp].dimensions[0])+1;
    as_command.delay_ms = DEFAULT_DELAY_MS;
    as_command.needle_penetration = components[tip_holder_comp].offset[2];
    as_command.puncture_speed = DEFAULT_TIP_PICKUP_SPEED;
    as_command.speed = DEFAULT_TRAVEL_SPEED;

    as_command.volume = 0;
    as_command.air_volume = 0;
    ModSyringeContent(0);

    syringe.flags |= SYR_FLAG_HAS_TIP;
    return STATUS_OK;
}

uint8_t RemoveTip()
{
    if ((syringe.flags ^ (SYR_FLAG_REMOVABLE_TIP | SYR_FLAG_HAS_TIP)) & (SYR_FLAG_REMOVABLE_TIP | SYR_FLAG_HAS_TIP)) { return STATUS_SETTING_DISABLED; } //STATUS_SYRINGE_TIP_ERROR
    uint8_t tip_remover_comp = -1;
    for (uint8_t i = 0; i < MAX_COMPONENTS; i++) {
        if((components[i].flags & COMP_FLAG_INSTALLED) && (components[i].type == COMP_TYPE_TIP_REMOVE)) {
        tip_remover_comp = i;
        break;
        }
    }
    if (tip_remover_comp == -1) return STATUS_REQUESTED_COMPONENT_TYPE_NOT_INSTALLED;
    if (components[as_command.target_comp].dimensions[0] < 2) return STATUS_GCODE_COORDINATES_OUT_OF_RANGE;
    as_command.target_comp = tip_remover_comp;
    as_command.target_offset[0] = 1;
    as_command.target_offset[1] = 1;
    as_command.delay_ms = DEFAULT_DELAY_MS;
    as_command.speed = DEFAULT_TRAVEL_SPEED;
    GoToTargetCoordinates();
    float target_coord[N_AXIS];
    plan_line_data_t pl_data;
    pl_data.feed_rate = DEFAULT_PUNCTURE_SPEED;
    for (uint8_t i = 0; i < N_AXIS; i++)
        target_coord[i] = sys_position[i]/settings.steps_per_mm[i];
    target_coord[0] = components[as_command.target_comp].coordinate_zero[0]+components[as_command.target_comp].offset[0]/(components[as_command.target_comp].dimensions[0]-1);
    delay_ms(as_command.delay_ms);
    move_execute(target_coord, &pl_data);
    pl_data.feed_rate = DEFAULT_TIP_PICKUP_SPEED;
    for (uint8_t i = 0; i < N_AXIS; i++)
        target_coord[i] = sys_position[i]/settings.steps_per_mm[i];
    target_coord[2] = components[as_command.target_comp].coordinate_zero[2]+components[as_command.target_comp].offset[2]; //pull up z
    delay_ms(as_command.delay_ms);
    move_execute(target_coord, &pl_data);
    syringe.flags &= ~SYR_FLAG_HAS_TIP;
    return STATUS_OK;
}

void NeedlePark()
{
    uint8_t comp = -1;
    for (uint8_t i = 0; i < MAX_COMPONENTS; i++) {
        if((components[i].flags & COMP_FLAG_INSTALLED) && (components[i].type == COMP_TYPE_WASTE)) {
            comp = i;
            break;
        }
    }
    sys.state = STATE_ALARM; //system_set_exec_alarm(); / STATE_SLEEP
    send_state();
    if (comp != -1)
    {
        as_command.target_comp = comp;
        as_command.target_offset[0] = 1;
        as_command.target_offset[1] = 1;
        as_command.delay_ms = 200;
        as_command.puncture_speed = DEFAULT_PUNCTURE_SPEED;
        GoToTargetCoordinates();
        float target_coord[N_AXIS];
        plan_line_data_t pl_data;
        pl_data.feed_rate = as_command.puncture_speed;
        for (uint8_t i = 0; i < N_AXIS; i++)
            target_coord[i] = sys_position[i]/settings.steps_per_mm[i];
        target_coord[2] = components[as_command.target_comp].offset[2]; //lower z
        delay_ms(as_command.delay_ms);
        move_execute(target_coord, &pl_data);
        target_coord[3] = 0; //empty syringe
        move_execute(target_coord, &pl_data);
    }
    else
    {
        GoHome(0x07);
    }
    sys.as_state = STATE_AS_PARK;
    send_state();
}

void NeedleUnpark()
{
    GoHome(0x07);
    sys.as_state = STATE_IDLE;
    sys.state = STATE_IDLE;
    send_state();
}

uint8_t FCGoNext()
{
    if ((fc_data.current_vial_offset[0]==components[fc_data.target_comp].dimensions[0] && fc_data.current_vial_offset[1] % 2) || (fc_data.current_vial_offset[0]==1 && !(fc_data.current_vial_offset[1] % 2)))
    {
        if (fc_data.current_vial_offset[1]==components[fc_data.target_comp].dimensions[1])
        {
            fc_valve_waste();
            return STATUS_OUT_OF_COLLECTION_VIALS;            
        }
        as_command.target_offset[0] = fc_data.current_vial_offset[0];
        as_command.target_offset[1] = fc_data.current_vial_offset[1]+1;
        if (fc_valve_get_state()) {
            fc_valve_waste();
            FastMoveXY();
            fc_valve_collect();
        }
        else {FastMoveXY();}
        fc_data.current_vial_offset[1] += 1;
    }
    else if (fc_data.current_vial_offset[1] % 2)          
    {
        as_command.target_offset[0] = fc_data.current_vial_offset[0]+1;
        as_command.target_offset[1] = fc_data.current_vial_offset[1];
        if (fc_valve_get_state()) {
            fc_valve_waste();
            FastMoveXY();
            fc_valve_collect();
        }
        else {FastMoveXY();}
        fc_data.current_vial_offset[0] += 1;
    }
    else
    {
        as_command.target_offset[0] = fc_data.current_vial_offset[0]-1;
        as_command.target_offset[1] = fc_data.current_vial_offset[1];
        if (fc_valve_get_state()) {
            fc_valve_waste();
            FastMoveXY();
            fc_valve_collect();
        }
        else {FastMoveXY();}
        fc_data.current_vial_offset[0] -= 1;
    }
    return STATUS_OK;
}

void FCGoCollectPos()
{
    as_command.target_comp = fc_data.target_comp; // comp_no-1,
    as_command.target_offset[0] = 1;
    as_command.target_offset[1] = 1;
    as_command.needle_penetration = 0.0f; // 0
    //  speed; // fastest or fastest/2
    //  delay_ms; // default
    GoToTargetCoordinates();
    fc_data.current_vial_offset[0] = 1;
    fc_data.current_vial_offset[1] = 1;
}

void FCGoVial(uint16_t vial)
{}

//getTip() removeTip() if hasTip

//on inject signal event in:
//collect mode
//on peaks signal in: (signal properties????), signal delay!
//on container max vol reached:

// void CollectFractions(uint8_t target_x, uint8_t target_y, bool waste, float* speed) // preset speeds, target_comp
// {
//     if (waste)
//     {
//         //wastes currX % 2 && currY % 2
//         //nearest wasteX: currX = currX-currX % 2 (if currX == 1 currX+=1)
//         as_command.
//         GoToTargetCoordinates(WASH_STATION_COMP, 1, WASTE, speed); //or switch valve!
//     }
//     else
//     {
//         GoToTargetCoordinates(COLLECT_COMP, target_x, target_y, speed); //and switch valve!
//         if (target_x == components[COLLECT_COMP-1].dimensions[0])
//             target_y++;
//         else if (target_y < components[COLLECT_COMP-1].dimensions[1])
//             target_x++;
//         else
//             GoToTargetCoordinates(WASH_STATION_COMP, 1, WASTE, speed); //or switch valve!
//     }
// }

//hexagonal fraction collector! x2
//if current fraction volume + total collected volume > max_volume --> switch collection vessel
//changeable needle/pipettor!