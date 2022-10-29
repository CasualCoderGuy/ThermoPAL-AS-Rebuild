#include "grbl.h"

bool AddComponent(uint8_t comp_no, float* coordinate_zero, float* offset, uint8_t* dimensions) //check if valid
{
  comp_no-=1;
  for (uint8_t i = 0; i < 2; i++) {
    components[comp_no].coordinate_zero[i]=coordinate_zero[i];
    components[comp_no].offset[i]=offset[i];
    components[comp_no].dimensions[i]=dimensions[i];
  }
  components[comp_no].coordinate_zero[2]=coordinate_zero[2];
  components[comp_no].offset[2]=offset[2];
  write_component_settings(comp_no);
  return true;
}

bool CopyComponent(uint8_t copy, uint8_t paste, uint8_t data) // y_dim, x_dim, max_ndl_pen, y_offset, x_offset, z_zero, y_zero, x_zero
{
  for (uint8_t i = 0; i < 3; i++)
  {
    if (data & (1<<i))
      components[paste-1].coordinate_zero[i] = components[copy-1].coordinate_zero[i];
    if (data & (1<<(i+3)))
      components[paste-1].offset[i] = components[copy-1].offset[i];
  }
  if (data & (1<<6))
      components[paste-1].dimensions[0] = components[copy-1].dimensions[0];
  if (data & (1<<7))
      components[paste-1].dimensions[1] = components[copy-1].dimensions[1];
  write_component_settings(paste);
  return true;
}

bool CheckCoordinatesIsInRange(float* coordinate_zero, float* offset, uint8_t* dim)
{
//+limits! x+- y0 z0 a?; ratio + needleoffset!
  return true;
}

void TeachZero(uint8_t comp_no)
{
  for (uint8_t idx = 0; idx < 3; idx++)
  {
    components[comp_no-1].coordinate_zero[idx] = sys_position[idx]/settings.steps_per_mm[idx];
  }
  write_component_settings(comp_no);
}
void TeachOffset(uint8_t comp_no)
{
  for (uint8_t idx = 0; idx < 2; idx++)
  {
    components[comp_no-1].offset[idx] = sys_position[idx]/settings.steps_per_mm[idx]-components[comp_no-1].coordinate_zero[idx];
  }
  write_component_settings(comp_no);
}
void TeachNeedlePenetration(uint8_t comp_no) //max
{
  components[comp_no-1].offset[2] = sys_position[2]/settings.steps_per_mm[2]-components[comp_no-1].coordinate_zero[2];
  write_component_settings(comp_no);
}

void report_components(uint16_t compToReport)
{
  for (uint8_t comp = 0; comp < MAX_COMPONENTS; comp++) {
    if (!(compToReport & (1<<comp))) continue;
    printString("comp="); print_uint8_base10(comp);
    printString("\rflg="); print_uint8_base10(components[comp].flags);
    printString("\rtype="); print_uint8_base10(components[comp].type);
    printString("\rst.pos.X="); printFloat(components[comp].coordinate_zero[0], N_DECIMAL_SETTINGVALUE); 
    printString("\rst.pos.Y="); printFloat(components[comp].coordinate_zero[1], N_DECIMAL_SETTINGVALUE); 
    printString("\rst.pos.Z="); printFloat(components[comp].coordinate_zero[2], N_DECIMAL_SETTINGVALUE);
    printString("\rlst.pos.offs.X="); printFloat(components[comp].offset[0], N_DECIMAL_SETTINGVALUE);
    printString("\rlst.pos.offs.Y="); printFloat(components[comp].offset[1], N_DECIMAL_SETTINGVALUE);
    printString("\rmin.pos.offs.Z="); printFloat(components[comp].offset[2], N_DECIMAL_SETTINGVALUE);
    printString("\rdim.X="); print_uint8_base10(components[comp].dimensions[0]);
    printString("\rdim.Y="); print_uint8_base10(components[comp].dimensions[1]);
    printString("\r\n");
  }
}

void report_position(uint8_t axisToReport)
{
  printString("curr.pos. in mm (and step):");
  for (uint8_t idx = 0; idx < N_AXIS; idx++) {
    if (!(axisToReport & (1<<idx))) continue;
    printString("\rAXIS"); print_uint8_base10(idx+1); printString("="); 
    printFloat(sys_position[idx]/settings.steps_per_mm[idx], N_DECIMAL_SETTINGVALUE);
    printString("("); printInteger(sys_position[idx]); printString(")");
  }
  printString("\r\n");
}

void report_syringe()
{
  printString("inst.syr:");
  printString("\rflg="); print_uint8_base10(syringe.flags);
  printString("\rndl.offs.(mm)="); printFloat(syringe.needle_offset, N_DECIMAL_SETTINGVALUE);
  printString("\rmax.vol(uL)="); printFloat(syringe.max_volume, N_DECIMAL_SETTINGVALUE);
  printString("\rvol/step(uL/step)="); printFloat(syringe.volume_per_step, N_DECIMAL_SETTINGVALUE);
  printString("\roverfill="); printFloat(syringe.overfill, N_DECIMAL_SETTINGVALUE);
  printString("\r\n");
}

void report_limits()
{
  uint8_t lim_pin_state = limits_get_state(); //bits: 0-3
  printString("limits=");
  print_uint8_base2_ndigit(lim_pin_state, N_AXIS);
  printString("\n");
}

void report_sigs()
{
  uint8_t ctrl_pin_state = system_control_get_state(); //bits: 0-3,4-6
  uint8_t output_state = lc_sig_evt_out_get_state(); //bits: 0-3
  printString("inputs=");
  print_uint8_base2_ndigit(ctrl_pin_state, 7);
  printString("\r");
  printString("outputs=");
  print_uint8_base2_ndigit(output_state, 4);
  printString("\n");
}

void send_aux()
{
  uint8_t ctrl_pin_state = system_control_get_state(); //bits: 0-3,4-6
  uint8_t output_state = lc_sig_evt_out_get_state(); //bits: 0-3
  //output_state & LC_VALVE_STATE, FC_VALVE_STATE, WASH1_STATE, WASH2_STATE
  printString("%");
  print_uint8_base16_ndigit(ctrl_pin_state, 2);
  printString("\r");
  print_uint8_base16_ndigit(output_state, 2);
  printString("\n");
}

void send_pos()
{
  printString("@");
  for (uint8_t idx = 0; idx < N_AXIS; idx++) {
    printString("\r");
    printFloat(sys_position[idx]/settings.steps_per_mm[idx], N_DECIMAL_SETTINGVALUE);
    printString("|"); printInteger(sys_position[idx]);
  }
  printString("\r\n");
}

void send_state()
{
  printString("#");
  printString("\r"); print_uint8_base10(sys.state);
  printString("\r"); print_uint8_base10(sys.as_state);
  printString("\r\n");
}

uint8_t ParseTarget(char* line, uint8_t char_counter, uint8_t types_to_check, uint8_t mandatory_values, bool enable_overfill)
{
  float f_value; char letter; uint8_t mandatory_set; // mandatory: bit0-X; bit1-Y; bit2-vol, bit3-ndl_pen, bit4-speed, bit5-punct_speed, bit6-vol_speed, bit7-air_vol
  uint8_t target_comp; // comp_no-1 !
  uint8_t target_offset[2]; // x, y
  float needle_penetration = DEFAULT_NEEDLE_PENETRATION; // mm
  float volume = 0; // uL
  float air_volume = DEFAULT_AIR_VOLUME; // uL
  float speed = DEFAULT_TRAVEL_SPEED; // mm/s
  float puncture_speed = DEFAULT_PUNCTURE_SPEED; // mm/s
  float volume_speed = DEFAULT_SYRINGE_SPEED; // uL/s
  uint16_t delay_ms = DEFAULT_DELAY_MS;
  if (!read_float(line, &char_counter, &f_value)) return STATUS_BAD_NUMBER_FORMAT;
  target_comp = trunc(f_value);
  if (target_comp > MAX_COMPONENTS || target_comp < 1) return STATUS_GCODE_COMPONENT_OUT_OF_RANGE;
  target_comp -= 1;
  if (bit_isfalse(components[target_comp].flags, COMP_FLAG_INSTALLED)) return STATUS_REQUESTED_COMPONENT_TYPE_NOT_INSTALLED;
  if (bit_istrue(components[target_comp].type, types_to_check)) return STATUS_REQUESTED_COMPONENT_TYPE_NOT_INSTALLED;
  while (line[char_counter] != 0) {
    letter = line[char_counter];
    switch(letter) {
      case AXIS_1_NAME:
        char_counter++;
        if (!read_float(line, &char_counter, &f_value)) return STATUS_BAD_NUMBER_FORMAT;
        target_offset[0] = trunc(f_value);
        if (target_offset[0] < 1 || target_offset[0] > components[target_comp].dimensions[0]) return STATUS_GCODE_INVALID_TARGET;
        mandatory_set |= bit(0);
        break;
      case AXIS_2_NAME:
        char_counter++;
        if (!read_float(line, &char_counter, &f_value)) return STATUS_BAD_NUMBER_FORMAT;
        target_offset[1] = trunc(f_value);
        if (target_offset[1] < 1 || target_offset[1] > components[target_comp].dimensions[1]) return STATUS_GCODE_INVALID_TARGET;
        mandatory_set |= bit(1);
        break;
      case 'V': //volume
        char_counter++;
        if (!read_float(line, &char_counter, &volume)) return STATUS_BAD_NUMBER_FORMAT;
        mandatory_set |= bit(2);
        break;
      case 'N': //needle_penetration
        char_counter++;
        if (!read_float(line, &char_counter, &needle_penetration)) return STATUS_BAD_NUMBER_FORMAT;
        if (needle_penetration < 0 || needle_penetration > components[target_comp].offset[2]) return STATUS_GCODE_COORDINATES_OUT_OF_RANGE;
        mandatory_set |= bit(3);
        break;
      case 'A': //air_volume
        char_counter++;
        if (!read_float(line, &char_counter, &air_volume)) return STATUS_BAD_NUMBER_FORMAT;
        //if (needle_penetration < 0 || needle_penetration > components[target_comp].offset[2]) return STATUS_GCODE_COORDINATES_OUT_OF_RANGE;
        mandatory_set |= bit(7);
        break;
      case 'F': //speed
        char_counter++;
        if (!read_float(line, &char_counter, &speed)) return STATUS_BAD_NUMBER_FORMAT;
        //if (needle_penetration < 0 || needle_penetration > components[target_comp].offset[2]) return STATUS_GCODE_COORDINATES_OUT_OF_RANGE;
        mandatory_set |= bit(4);
        break;
      case 'P': //puncture_speed
        char_counter++;
        if (!read_float(line, &char_counter, &puncture_speed)) return STATUS_BAD_NUMBER_FORMAT;
        //if (needle_penetration < 0 || needle_penetration > components[target_comp].offset[2]) return STATUS_GCODE_COORDINATES_OUT_OF_RANGE;
        mandatory_set |= bit(5);
        break;
      case 'S': //volume_speed
        char_counter++;
        if (!read_float(line, &char_counter, &volume_speed)) return STATUS_BAD_NUMBER_FORMAT;
        //if (needle_penetration < 0 || needle_penetration > components[target_comp].offset[2]) return STATUS_GCODE_COORDINATES_OUT_OF_RANGE;
        mandatory_set |= bit(6);
        break;
      case 'D': //volume_speed
        char_counter++;
        if (!read_float(line, &char_counter, &f_value)) return STATUS_BAD_NUMBER_FORMAT;
        delay_ms = trunc(f_value);
        if (delay_ms < 0 || delay_ms > 5000) return STATUS_GCODE_COORDINATES_OUT_OF_RANGE;
        break;
      // case 'C': //fill_strokes=cycle
      //   break;
      default: return STATUS_INVALID_STATEMENT;
    }
  }
  if ((mandatory_set & mandatory_values) != mandatory_values) return STATUS_GCODE_VALUE_WORD_MISSING;
  //check for volume exceeding limits accounting for air volume, including overfill; starting from current volume, calculating with vol/step
  as_command.target_comp = target_comp;
  as_command.target_offset[0] = target_offset[0];
  as_command.target_offset[1] = target_offset[1];
  as_command.needle_penetration = needle_penetration;
  as_command.volume = volume;
  as_command.air_volume = air_volume;
  as_command.speed = speed;
  as_command.puncture_speed = puncture_speed;
  as_command.volume_speed = volume_speed;
  as_command.delay_ms = delay_ms;
  return STATUS_OK;
}

void GoToTargetCoordinates() //move z (needle_penetration)
{
  if (components[as_command.target_comp].dimensions[0] < as_command.target_offset[0] || components[as_command.target_comp].dimensions[1] < as_command.target_offset[1] || as_command.target_offset[0] == 0 || as_command.target_offset[1] == 0)
    return;
  float target_coord[N_AXIS];
  plan_line_data_t pl_data;
  pl_data.feed_rate = as_command.speed;
  for (uint8_t i = 0; i < N_AXIS; i++)
    target_coord[i] = sys_position[i]/settings.steps_per_mm[i];
  target_coord[2] = -settings.max_travel[AXIS_3]*(1-settings.ratio[AXIS_3])+syringe.needle_offset;
  move_execute(target_coord, &pl_data);//components[as_command.target_comp].dimensions[0]-1!!!!!
  if (components[as_command.target_comp].dimensions[0] == 1)
    target_coord[0] = components[as_command.target_comp].coordinate_zero[0];
  else
    target_coord[0] = components[as_command.target_comp].coordinate_zero[0]+components[as_command.target_comp].offset[0]/(components[as_command.target_comp].dimensions[0]-1)*(as_command.target_offset[0]-1);
  if (components[as_command.target_comp].dimensions[1] == 1)
    target_coord[1] = components[as_command.target_comp].coordinate_zero[1];
  else
    target_coord[1] = components[as_command.target_comp].coordinate_zero[1]+components[as_command.target_comp].offset[1]/(components[as_command.target_comp].dimensions[1]-1)*(as_command.target_offset[1]-1);
  move_execute(target_coord, &pl_data);
  target_coord[2] = components[as_command.target_comp].coordinate_zero[2];
  move_execute(target_coord, &pl_data);
}

void FastMoveXY()
{
  float target_coord[N_AXIS];
  plan_line_data_t pl_data;
  pl_data.feed_rate = settings.max_rate[AXIS_1] < settings.max_rate[AXIS_2] ? settings.max_rate[AXIS_1] : settings.max_rate[AXIS_2];
  for (uint8_t i = 0; i < N_AXIS; i++)
    target_coord[i] = sys_position[i]/settings.steps_per_mm[i];
  target_coord[0] = components[as_command.target_comp].coordinate_zero[0]+components[as_command.target_comp].offset[0]/components[as_command.target_comp].dimensions[0]*(as_command.target_offset[0]-1);
  target_coord[1] = components[as_command.target_comp].coordinate_zero[1]+components[as_command.target_comp].offset[1]/components[as_command.target_comp].dimensions[1]*(as_command.target_offset[1]-1);
  move_execute(target_coord, &pl_data);
}

/*
void HexFastMoveXY(bool zigzag) // zigzag/armchair
{
  float target_coord[N_AXIS];
  plan_line_data_t pl_data;
  pl_data.feed_rate = settings.max_rate[AXIS_1] < settings.max_rate[AXIS_2] ? settings.max_rate[AXIS_1] : settings.max_rate[AXIS_2];
  for (uint8_t i = 0; i < N_AXIS; i++)
    target_coord[i] = sys_position[i]/settings.steps_per_mm[i];
  uint8_t axis_st = 0;
  uint8_t axis_din = 1;
  if (zigzag)
  {
    axis_st = 1;
    axis_din = 0;
  }
  if (components[as_command.target_comp].dimensions[0] < as_command.target_offset[0] || components[as_command.target_comp].dimensions[1] < as_command.target_offset[1] || as_command.target_offset[0] == 0 || as_command.target_offset[1] == 0)
  {
    if (!((as_command.target_offset[axis_din]==components[as_command.target_comp].dimensions[axis_din]+1) && !(as_command.target_offset[axis_st] % 2)))
      return;
  }
  if (as_command.target_offset[axis_st] % 2)
  {
    if (components[as_command.target_comp].dimensions[axis_st] % 2)
      target_coord[axis_din] = components[as_command.target_comp].coordinate_zero[axis_din]+components[as_command.target_comp].offset[axis_din]/components[as_command.target_comp].dimensions[axis_din]*(as_command.target_offset[axis_din]-1);
    else
      target_coord[axis_din] = components[as_command.target_comp].coordinate_zero[axis_din]+components[as_command.target_comp].offset[axis_din]/(components[as_command.target_comp].dimensions[axis_din]+0.5)*(as_command.target_offset[axis_din]-1);
  }
  else
  {
    if (components[as_command.target_comp].dimensions[axis_st] % 2)
      target_coord[axis_din] = components[as_command.target_comp].coordinate_zero[axis_din]+components[as_command.target_comp].offset[axis_din]/components[as_command.target_comp].dimensions[axis_din]*(as_command.target_offset[axis_din]-1.5);
    else
      target_coord[axis_din] = components[as_command.target_comp].coordinate_zero[axis_din]+components[as_command.target_comp].offset[axis_din]/(components[as_command.target_comp].dimensions[axis_din]+0.5)*(as_command.target_offset[axis_din]-1.5);
  }
  target_coord[axis_st] = components[as_command.target_comp].coordinate_zero[axis_st]+components[as_command.target_comp].offset[axis_st]/components[as_command.target_comp].dimensions[axis_st]*(as_command.target_offset[axis_st]-1);
  move_execute(target_coord, &pl_data);
}
*/

void GoHome(uint8_t axisflags)
{
  float target_coord[N_AXIS];
  plan_line_data_t pl_data;
  pl_data.feed_rate = as_command.speed;
  for (uint8_t i = 0; i < N_AXIS; i++)
    target_coord[i] = sys_position[i]/settings.steps_per_mm[i];
  for (uint8_t i = N_AXIS; i > 0; i--) {
    if (axisflags & (1 << i)) {
      if (i == AXIS_3) target_coord[i-1] = syringe.needle_offset;
      else target_coord[i-1] = 0;
      move_execute(target_coord, &pl_data);
    }
  }
}

uint8_t InitFC()
{
  if (sys.state != STATE_IDLE || (sys.as_state != STATE_IDLE && sys.as_state != STATE_AS_COLLECT)) return(STATUS_IDLE_ERROR);
  for (uint8_t i = 0; i < MAX_COMPONENTS; i++) {
    if((components[i].flags & COMP_FLAG_INSTALLED) && (components[i].type == COMP_TYPE_COLLECT)) {
      sys.as_state = STATE_AS_COLLECT;
      send_state();
      fc_data.target_comp = i;
      break;
    }
  }
  if (sys.as_state != STATE_AS_COLLECT) return(STATUS_REQUESTED_COMPONENT_TYPE_NOT_INSTALLED);
  fc_valve_waste();
  GoHome(0x0F);
  return STATUS_OK;
}

// NOTE: This function can only be called in IDLE state.
void write_component_settings(uint8_t comp_no){
  uint32_t addr = comp_no*(sizeof(struct Component)+1) + EEPROM_ADDR_COMP_INFO;
  memcpy_to_eeprom_with_checksum(addr, (char*)&components[comp_no-1], sizeof(struct Component));
}

bool read_component_settings(uint8_t comp_no) {
  // Read settings-record and check checksum
  uint32_t addr = comp_no*(sizeof(struct Component)+1) + EEPROM_ADDR_COMP_INFO;
  if (!(memcpy_from_eeprom_with_checksum((char*)&components[comp_no-1], addr, sizeof(struct Component))))
    return(false);
  return(true);
}

void write_syringe_settings(){
  uint32_t addr = (sizeof(struct Syringe)+1) + EEPROM_ADDR_SYR_INFO;
  memcpy_to_eeprom_with_checksum(addr, (char*)&syringe, sizeof(struct Syringe));
}

bool read_syringe_settings() {
  // Read settings-record and check checksum
  uint32_t addr = (sizeof(struct Syringe)+1) + EEPROM_ADDR_SYR_INFO;
  if (!(memcpy_from_eeprom_with_checksum((char*)&syringe, addr, sizeof(struct Syringe))))
    return(false);
  return(true);
}

void reset_components()
{
  for (uint8_t i = 0; i < MAX_COMPONENTS; i++)
  {
    memset(components[i].coordinate_zero, 0, sizeof components[i].coordinate_zero);
    memset(components[i].offset, 0, sizeof components[i].offset);
    components[i].dimensions[0] = 1;
    components[i].dimensions[1] = 1;
    components[i].flags = 0;
    components[i].type = 0;
    write_component_settings(i);
  }
}

void reset_syringe()
{
  syringe.flags = 0;
  syringe.max_volume = 1;
  syringe.needle_offset = 0;
  syringe.volume_per_step = 1;
  syringe.overfill = 1;
  write_syringe_settings();
}

//set from init and gcode
void set_needle_offset(float offset_mm) 
{
  sys_position[TOOL_LENGTH_OFFSET_AXIS]+=(int32_t)trunc((offset_mm)*settings.steps_per_mm[TOOL_LENGTH_OFFSET_AXIS]);
  gc_sync_position();
  plan_sync_position();
  send_pos();
}

uint8_t fc_valve_get_state()
{
  #ifdef INVERT_FC_VALVE_PIN
    if (bit_isfalse(FC_VALVE_PORT,(1 << FC_VALVE_BIT))) {
  #else
    if (bit_istrue(FC_VALVE_PORT,(1 << FC_VALVE_BIT))) {
  #endif
    return(1);
  }
  return(0);
}

void fc_valve_waste()
{
  #ifdef INVERT_FC_VALVE_PIN
    FC_VALVE_PORT |= (1 << FC_VALVE_BIT);
  #else
    FC_VALVE_PORT &= ~(1 << FC_VALVE_BIT);
  #endif
}

void fc_valve_collect()
{
  #ifdef INVERT_FC_VALVE_PIN
    FC_VALVE_PORT &= ~(1 << FC_VALVE_BIT);
  #else
    FC_VALVE_PORT |= (1 << FC_VALVE_BIT);
  #endif
}

void fc_valve_toggle()
{
  FC_VALVE_PORT ^= (1 << FC_VALVE_BIT);
}

void injector_valve_inject(){}

void injector_valve_load(){}

void injector_valve_toggle(){}

uint8_t lc_sig_evt_out_get_state()
{
  uint8_t sig_state = 0;
  uint8_t pin = (LC_EVT_SIG_OUT_PORT & LC_EVT_SIG_OUT_MASK);
  pin ^= settings.sig_out_invert_mask;
  if (pin) {
    if (bit_isfalse(pin,(1<<LC_EVT_SIG_OUT_BIT_AS_READY))) { sig_state |= LC_EVT_SIG_OUT_INDEX_AS_READY; }
    if (bit_isfalse(pin,(1<<LC_EVT_SIG_OUT_BIT_INJECT))) { sig_state |= LC_EVT_SIG_OUT_INDEX_INJECT; }
    if (bit_isfalse(pin,(1<<LC_EVT_SIG_OUT_BIT_GRAD_START))) { sig_state |= LC_EVT_SIG_OUT_INDEX_GRAD_START; }
    if (bit_isfalse(pin,(1<<LC_EVT_SIG_OUT_BIT_PUMP_STOP))) { sig_state |= LC_EVT_SIG_OUT_INDEX_PUMP_STOP; }
  }
  return(sig_state);
}

//pins: set pins to HIGH or LOW, mask: select pins to set, pulse_length: sig length in ms (0: no setback)
void lc_sig_evt_out_send(uint8_t pins, uint8_t mask, int pulse_length)
{
  pins ^= settings.sig_out_invert_mask;
  LC_EVT_SIG_OUT_PORT = (pins & mask) | (LC_EVT_SIG_OUT_PORT & ~(mask));
  if (pulse_length)
  {
    unsigned long del_start = millis();
    while (millis()-del_start < pulse_length){
      protocol_execute_realtime(); // Check for any run-time commands
      if (sys.abort) { return; } // Bail, if system abort.
      if ( plan_check_full_buffer() ) { protocol_auto_cycle_start(); } // Auto-cycle start when buffer is full.
    }
    LC_EVT_SIG_OUT_PORT = (~pins & mask) | (LC_EVT_SIG_OUT_PORT & ~(mask));
  }
}

bool init_autosampler() 
{
  //init structs to 0?
  FC_VALVE_DDR |= (1<<FC_VALVE_BIT);
  fc_valve_waste();
  for (uint8_t comp = 1; comp <= MAX_COMPONENTS; comp++)
    read_component_settings(comp);
  read_syringe_settings();
  set_needle_offset(syringe.needle_offset);
  current_tip = 1;
  return true;
}

//accuracy test: move w/ diff. speed back-forth - check accuracy by lowering z (accel. effect?)
//speed test: speed range, optimal acceleration
//step test: minimum step(speed, accel, accuracy)
//max travel test: Y,Z,A w/ hall sensors (first go positive powerless) - invert Z!
//homing test: test homing w/ X (direction, distance, speed, accuracy), then Y,Z [homing_seek_rate,homing_dir_mask,homing_feed_rate,homing_pulloff,homing_debounce_delay]
//-
//'teach->home,gotocoord,transferliquid,mix,wash,aspirate,inject,pickuptip,removetip,park
//'FCGoVial,FCNext,FCInit,Ready
//syringes
//wash motors
//FCvalve,LCvalve
//output signals

//check_coordinates
//wash&parse, inject; check volume; FCGoVial
//exec report flag for JOG
//JOG cancel works?

//fc: restart vial count/continue
//(TeachMode), ParkMode(+ALARM?/STATE_AS_PARKING), CollectMode & JobMode (update sys.state checks)
//signals: analog input-fraction collector, dig.output/analog inp.-injector
//! feed hold; ctrlx soft reset(cancel); > skip current job-handle arising problems; clear motion
// IsAtCoordinate/GetCurrentCoordinate(calc OR sys.current_coord, sys.IsLowered)
//+1 AXIS?: Inj Valve

//void coolant_set_state(uint8_t mode) //mode = COOLANT_FLOOD_ENABLE / COOLANT_MIST_ENABLE / 0
//#define COOLANT_FLOOD_ENABLE  PL_COND_FLAG_COOLANT_FLOOD (bit6) pin10
//#define COOLANT_MIST_ENABLE   PL_COND_FLAG_COOLANT_MIST (bit7) pin9
//case CMD_COOLANT_FLOOD_OVR_TOGGLE: system_set_exec_accessory_override_flag(EXEC_COOLANT_FLOOD_OVR_TOGGLE); break;
//case CMD_COOLANT_MIST_OVR_TOGGLE: system_set_exec_accessory_override_flag(EXEC_COOLANT_MIST_OVR_TOGGLE); break;

//template infos?

// valves:
// https://darwin-microfluidics.com/collections/microfluidic-valves?page=2
// https://www.emerson.com/hu-hu/catalog/microfluidic-valves?fetchFacets=true#facet:&partsFacet:&facetLimit:&productBeginIndex:0&partsBeginIndex:0&orderBy:&partsOrderBy:&pageView:grid&minPrice:&maxPrice:&pageSize:&
// https://www.elveflow.com/microfluidic-products/microfluidics-accessories/microfluidics-valves/
// https://www.burkert.com/en/products/microfluidic-valves-and-pumps/2-2-and-3-2-way-micro-solenoids
// https://www.burkert.com/en/type/6724
// https://www.vici.com/vval/vval.php
// https://www.idex-hs.com/store/fluidics/valves.html
// https://www.takasago-fluidics.com/products/stv-2no-1?variant=32520984985732
// http://danyk.cz/hall_en.html

/*
ez fc
type/0127
No. 296659  flange  $129
No. 464641  UNF     $160

type/6724
No. 304270  UNF     $164  5bar  0.8mm
No. 295322  flange  $138  2bar  1.2mm

*/