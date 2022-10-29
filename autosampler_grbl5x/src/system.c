/*
  system.c - Handles system level commands and real-time processes
  Part of Grbl

  Copyright (c) 2017-2018 Gauthier Briere
  Copyright (c) 2014-2016 Sungeun K. Jeon for Gnea Research LLC

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "grbl.h"


void system_init()
{
  CONTROL_DDR &= ~(CONTROL_MASK | LC_EVT_SIG_IN_MASK); // Configure as input pins
  #ifdef DISABLE_CONTROL_PIN_PULL_UP
    CONTROL_PORT &= ~(CONTROL_MASK | LC_EVT_SIG_IN_MASK); // Normal low operation. Requires external pull-down.
  #else
    CONTROL_PORT |= CONTROL_MASK | LC_EVT_SIG_IN_MASK;   // Enable internal pull-up resistors. Normal high operation.
  #endif
  CONTROL_PCMSK |= CONTROL_MASK | LC_EVT_SIG_IN_MASK;  // Enable specific pins of the Pin Change Interrupt
  PCICR |= (1 << CONTROL_INT);   // Enable Pin Change Interrupt

  LC_EVT_SIG_OUT_DDR |= LC_EVT_SIG_OUT_MASK;
  LC_EVT_SIG_OUT_PORT = (settings.sig_out_invert_mask & LC_EVT_SIG_OUT_MASK) | (LC_EVT_SIG_OUT_DDR & ~LC_EVT_SIG_OUT_MASK);

  //setResetDisable(settings.disable_reset); //output high, input pullup?
  for (uint8_t idx = 0; idx < N_AXIS; idx++)
  {
    prev_pos[idx] = sys_position[idx];
  }
}

// Returns control pin state as a uint8 bitfield. Each bit indicates the input pin state, where
// triggered is 1 and not triggered is 0. Invert mask is applied. Bitfield organization is
// defined by the CONTROL_PIN_INDEX in the header file.
uint8_t system_control_get_state()
{
  uint8_t control_state = 0;
  uint8_t pin = (CONTROL_PIN & (CONTROL_MASK | LC_EVT_SIG_IN_MASK));
  pin ^= settings.sig_in_invert_mask;
  if (pin) {
    if (bit_isfalse(pin,(1<<CONTROL_SAFETY_DOOR_BIT))) { control_state |= CONTROL_PIN_INDEX_SAFETY_DOOR; }
    if (bit_isfalse(pin,(1<<CONTROL_RESET_BIT))) { control_state |= CONTROL_PIN_INDEX_RESET; }
    if (bit_isfalse(pin,(1<<CONTROL_FEED_HOLD_BIT))) { control_state |= CONTROL_PIN_INDEX_FEED_HOLD; }
    if (bit_isfalse(pin,(1<<CONTROL_CYCLE_START_BIT))) { control_state |= CONTROL_PIN_INDEX_CYCLE_START; }
    if (bit_isfalse(pin,(1<<LC_EVT_SIG_IN_BIT_START))) { control_state |= LC_EVT_SIG_IN_OFFSET_START; }
    if (bit_isfalse(pin,(1<<LC_EVT_SIG_IN_BIT_ERROR))) { control_state |= LC_EVT_SIG_IN_OFFSET_ERROR; }
    if (bit_isfalse(pin,(1<<LC_EVT_SIG_IN_BIT_INIT))) { control_state |= LC_EVT_SIG_IN_OFFSET_INIT; }
  }
  return(control_state);
}


// Pin change interrupt for pin-out commands, i.e. cycle start, feed hold, and reset. Sets
// only the realtime command execute variable to have the main program execute these when
// its ready. This works exactly like the character-based realtime commands when picked off
// directly from the incoming serial data stream.
ISR(CONTROL_INT_vect)
{
  uint8_t pin = system_control_get_state();
  if (pin) {
    if (bit_istrue(pin,CONTROL_PIN_INDEX_RESET)) {
      mc_reset();
    } else if (bit_istrue(pin,CONTROL_PIN_INDEX_CYCLE_START)) {
      bit_true(sys_rt_exec_state, EXEC_CYCLE_START);
    } else if (bit_istrue(pin,CONTROL_PIN_INDEX_FEED_HOLD)) {
      bit_true(sys_rt_exec_state, EXEC_FEED_HOLD);
    } else if (bit_istrue(pin,CONTROL_PIN_INDEX_SAFETY_DOOR)) {
      bit_true(sys_rt_exec_state, EXEC_SAFETY_DOOR);
    } else if (bit_istrue(pin,LC_EVT_SIG_IN_OFFSET_START)) {
      ;//start inject
    } else if (bit_istrue(pin,LC_EVT_SIG_IN_OFFSET_ERROR)) {
      fc_valve_waste(); system_set_exec_state_flag(EXEC_FEED_HOLD);
    } else if (bit_istrue(pin,LC_EVT_SIG_IN_OFFSET_INIT)) {
      ;//init_as
    }
  }
}


// Returns if safety door is ajar(T) or closed(F), based on pin state.
uint8_t system_check_safety_door_ajar()
{
    return(system_control_get_state() & CONTROL_PIN_INDEX_SAFETY_DOOR);
}


// Executes user startup script, if stored.
void system_execute_startup(char *line)
{
  uint8_t n;
  for (n=0; n < N_STARTUP_LINE; n++) {
    if (!(settings_read_startup_line(n, line))) {
      line[0] = 0;
      report_execute_startup_message(line,STATUS_SETTING_READ_FAIL);
    } else {
      if (line[0] != 0) {
        uint8_t status_code = gc_execute_line(line);
        report_execute_startup_message(line,status_code);
      }
    }
  }
}


// Directs and executes one line of formatted input from protocol_process. While mostly
// incoming streaming g-code blocks, this also executes Grbl internal commands, such as
// settings, initiating the homing cycle, and toggling switch states. This differs from
// the realtime command module by being susceptible to when Grbl is ready to execute the
// next line during a cycle, so for switches like block delete, the switch only effects
// the lines that are processed afterward, not necessarily real-time during a cycle,
// since there are motions already stored in the buffer. However, this 'lag' should not
// be an issue, since these commands are not typically used during a cycle.
uint8_t system_execute_line(char *line)
{
  uint8_t char_counter = 1;
  uint8_t helper_var = 0;
  float parameter, value;
  uint8_t result = STATUS_OK;
  switch( line[char_counter] ) { // if last line != 0 return STATUS_INVALID_STATEMENT
    case 0 : report_grbl_help(); break;
    case 'F' :
      if (line[2]==0) {return STATUS_INVALID_STATEMENT;}
      if ((sys.state & (STATE_HOLD | STATE_HOMING | STATE_JOG | STATE_SAFETY_DOOR | STATE_SLEEP)) | (sys.as_state & (STATE_AS_PARK | STATE_AS_JOB))) {return(STATUS_IDLE_ERROR);}
      else if (sys.as_state == STATE_AS_COLLECT) {
        float f_value;
        if (read_float(line, &char_counter, &f_value))
        {
          uint8_t vial = trunc(f_value);
          FCGoVial(vial);
          break;
        }
      }
      char_counter++;
      switch(line[char_counter]) {
        case 'N':
          if (sys.as_state != STATE_AS_COLLECT) return(STATUS_IDLE_ERROR);
          FCGoNext();
          break;
        case 'C':
          fc_valve_collect();
          break;
        case 'W': //also E, X?
          fc_valve_waste();
          break;
        case 'I': 
          result = InitFC();
          if (result) {return result;}
          break;
        case 'R': {
          if (sys.as_state != STATE_AS_COLLECT) return(STATUS_IDLE_ERROR);
          char_counter++;
          uint8_t station = 0;
          float wash_time = 0;
          if (line[char_counter] == 'W')
          {
            char_counter++;
            float f_value; uint8_t int_value;
            if (!read_float(line, &char_counter, &f_value)) return STATUS_BAD_NUMBER_FORMAT;
            int_value = trunc(f_value);
            if (int_value > 2 || int_value < 1) return STATUS_GCODE_COMPONENT_OUT_OF_RANGE;
            station = (1 << (int_value-1));
            if (line[char_counter] == 'T')
            {
              char_counter++;
              if (!read_float(line, &char_counter, &f_value)) return STATUS_BAD_NUMBER_FORMAT;
              wash_time = f_value;
            }
            else {wash_time = 5.0;}
          }
          result = ParseTarget(line, char_counter, (COMP_TYPE_TIP_REMOVE | COMP_TYPE_TIP_HOLDER | COMP_TYPE_INJECTOR | COMP_TYPE_WASH_STATION | COMP_TYPE_WASTE | COMP_TYPE_COLLECT), 0b111, false);
          if (result) {return result;}
          fc_valve_waste();
          result = Inject(INJECT_TYPE_PARTIAL | ENABLE_NEEDLE_WASH); //wash, inject type
          if (result) {return result;}
          FCGoCollectPos();
          break;
        }
        case 'Q': // and X(error) --> real time command: feed hold & reset
          if (sys.as_state != STATE_AS_COLLECT) return(STATUS_IDLE_ERROR);
          fc_valve_waste();
          GoHome(0x07);//+empty syringe?
          sys.state = STATE_IDLE;
          sys.as_state = STATE_IDLE;
          send_state();
          break;
        default: return STATUS_INVALID_STATEMENT;
      }
      //: I(init), R(ready), C(collect), W(waste), N(next), %number%(goto vial), E(run end), X(error), Q(quit)
      break;
    case 'M' :
      //set JOB mode on
      if ((sys.state & (STATE_HOLD | STATE_HOMING | STATE_JOG | STATE_SAFETY_DOOR | STATE_SLEEP)) | (sys.as_state & (STATE_AS_PARK | STATE_AS_COLLECT))) {return(STATUS_IDLE_ERROR);}
      char_counter++;
      switch(line[char_counter]) {
        case 'T'://transfer
          char_counter++;
          uint8_t result = ParseTarget(line, char_counter, (COMP_TYPE_TIP_REMOVE | COMP_TYPE_TIP_HOLDER), 0b111, false);
          if (result) return result;
          sys.as_state = STATE_AS_JOB;
          ModSyringeContent(0);
          break;
        case 'M': {//mix
          uint8_t cycles = 1;
          char_counter++;
          float f_value;
          if (!read_float(line, &char_counter, &f_value)) return STATUS_BAD_NUMBER_FORMAT;
          cycles = trunc(f_value);
          if (cycles < 1 || cycles > 20) return STATUS_GCODE_COORDINATES_OUT_OF_RANGE;
          uint8_t result = ParseTarget(line, char_counter, (COMP_TYPE_TIP_REMOVE | COMP_TYPE_TIP_HOLDER | COMP_TYPE_INJECTOR | COMP_TYPE_COLLECT), 0b111, false);
          if (result) return result;
          sys.as_state = STATE_AS_JOB;
          Mix(cycles);
          break;
        }
        case 'W': {//wash
          uint8_t station = 1;
          uint8_t cycles = 1;
          char_counter++;
          float f_value; uint8_t int_value;
          if (!read_float(line, &char_counter, &f_value)) return STATUS_BAD_NUMBER_FORMAT;
          int_value = trunc(f_value);
          if (int_value > 2 || int_value < 1) return STATUS_GCODE_COMPONENT_OUT_OF_RANGE;
          station = (1 << (int_value-1));
          //parse only wash stationNo, time, cycle, fill%
          sys.as_state = STATE_AS_JOB;
          //Wash(cycles, 0, station);
          break;
        }
        case 'V'://lc valve
          char_counter++;
          switch(line[char_counter]) {
            case 'I':
              injector_valve_inject();
              break;
            case 'L':
              injector_valve_load();
              break;
            case 'T':
              injector_valve_toggle();
              break;
            default: return STATUS_INVALID_STATEMENT;
          }
          break;
        case 'C'://fc valve
          char_counter++;
          switch(line[char_counter]) {
            case 'C':
              fc_valve_collect();
              break;
            case 'W':
              fc_valve_waste();
              break;
            case 'T':
              fc_valve_toggle();
              break;
            default: return STATUS_INVALID_STATEMENT;
          }
          break;
        case 'G': {//gotocoord
          char_counter++;
          uint8_t result = ParseTarget(line, char_counter, (COMP_TYPE_TIP_REMOVE | COMP_TYPE_TIP_HOLDER), 0b11, false);
          if (result) return result;
          sys.as_state = STATE_AS_JOB;
          GoToTargetCoordinates();
          break;
        }
        case 'A': {//aspirate
          char_counter++;
          float volume; float speed;
          if (!read_float(line, &char_counter, &volume)) return STATUS_BAD_NUMBER_FORMAT;
          //check volume valid
          if (line[char_counter] != 0)
          {
            if (line[char_counter] != 'S') return STATUS_INVALID_STATEMENT;
            if (!read_float(line, &char_counter, &speed)) return STATUS_BAD_NUMBER_FORMAT;
            //check vol_speed valid
          }
          else { speed = DEFAULT_SYRINGE_SPEED; }
          as_command.volume = volume;
          as_command.volume_speed = speed;
          sys.as_state = STATE_AS_JOB;
          Aspirate();
          break;
        }
        case 'I': //Inject
          break;       
        case 'P': {
          char_counter++;
          float f_value; uint16_t tip_no = 0;
          if (read_float(line, &char_counter, &f_value)) { tip_no = trunc(f_value); }
          if (line[char_counter] != 0) return STATUS_INVALID_STATEMENT;
          sys.as_state = STATE_AS_JOB;
          uint8_t result = PickUpTip(tip_no);
          if (result) return result;
          break;
        }
        case 'R': {
          char_counter++;
          if (line[char_counter] != 0) return STATUS_INVALID_STATEMENT;
          sys.as_state = STATE_AS_JOB;
          uint8_t result = RemoveTip();
          if (result) return result;
          break;
        }
          break;
        //A aspirate, G gotocoord, R remove tip, P pick up tip(memorize), V valve, C collector, I inject
        case 'H':
          char_counter++;
          if (line[char_counter] == 0) GoHome(0x0F);
          else {
            float f_value; uint8_t int_value;
            if (!read_float(line, &char_counter, &f_value)) return STATUS_BAD_NUMBER_FORMAT;
            int_value = trunc(f_value);
            GoHome(int_value);
          }
          break;
        case 'X'://JOB mode off
          sys.as_state = STATE_IDLE;
          break;
        default: return STATUS_INVALID_STATEMENT;
      }
      break;
    case 'J' : // Jogging
      // Execute only if in IDLE or JOG states.
      if ((sys.state != STATE_IDLE && sys.state != STATE_JOG) || (sys.as_state != STATE_IDLE)) { return(STATUS_IDLE_ERROR); }
      char_counter++;
      if(line[char_counter] != '=') { return(STATUS_INVALID_STATEMENT); }
      return(gc_execute_line(line)); // NOTE: $J= is ignored inside g-code parser and used to detect jog motions.
      break;
    case 'P' :
      char_counter++;
      if ( line[char_counter] != 0 ) return(STATUS_INVALID_STATEMENT);
      if (sys.as_state == STATE_AS_PARK) NeedleUnpark();
      else if ((sys.state != STATE_IDLE && sys.state != STATE_SLEEP) || sys.as_state != STATE_IDLE) return(STATUS_IDLE_ERROR);
      else NeedlePark();
      break;
    case 'T' :
      if (sys.state != STATE_IDLE || sys.as_state != STATE_IDLE) return(STATUS_IDLE_ERROR);
      char_counter++;
      if (line[char_counter] == 0) { 
        return STATUS_INVALID_STATEMENT;
      }
      float f_value; uint8_t comp;
      if (!read_float(line, &char_counter, &f_value)) return STATUS_BAD_NUMBER_FORMAT;
      comp = trunc(f_value);
      if (comp > MAX_COMPONENTS || comp < 1) return STATUS_GCODE_COMPONENT_OUT_OF_RANGE;
      switch (line[char_counter]) {
        case 'S': TeachZero(comp); break;
        case 'E': TeachOffset(comp); break;
        case 'P': TeachNeedlePenetration(comp); break;
        default: return STATUS_INVALID_STATEMENT;
      }
      break;
    case 'O':
      char_counter++;
      if(line[char_counter]=='C') {
        char_counter++;
        float f_value;
        if (!read_float(line, &char_counter, &f_value)) return STATUS_BAD_NUMBER_FORMAT; //stop before flag
        uint8_t comps = trunc(f_value);
        uint8_t copy = trunc(f_value/100);
        uint8_t paste = comps & ~0xC;
        if (copy < 1 || copy > MAX_COMPONENTS || paste < 1 || paste > MAX_COMPONENTS) return STATUS_GCODE_COMPONENT_OUT_OF_RANGE;
        uint8_t data = 0;
        while (line[char_counter] != 0) {
          if (char_counter > 12) return STATUS_OVERFLOW; //12-14
          char val = line[char_counter];
          if(val == '1') data |= 1<<(char_counter-5);
          else if(val != '0') return STATUS_INVALID_STATEMENT;
          char_counter++;
        }
        CopyComponent(copy, paste, data);
      }
      else if (line[char_counter]=='S') {
        char_counter++;
        float f_value; uint8_t comp; uint8_t int_value;
        if (!read_float(line, &char_counter, &f_value)) return STATUS_BAD_NUMBER_FORMAT;
        comp = trunc(f_value);
        if (comp > MAX_COMPONENTS || comp < 1) return STATUS_GCODE_COMPONENT_OUT_OF_RANGE;
        char letter = 0;
        float start[3] = {-1.0f};
        float end[3] = {-1.0f};
        uint8_t dim[2] = {0};
        for (uint8_t i = 0; i < 3; i++)
        {
          start[i] = components[comp].coordinate_zero[i];
          end[i] = components[comp].offset[i];
          if (i<2) { dim[i] = components[comp].dimensions[i]; }
        }
        while (line[char_counter] != 0) {
          letter = line[char_counter];
          switch(letter) {
            case 'S':
              char_counter++;
              letter = line[char_counter];
              char_counter++;
              if (!read_float(line, &char_counter, &f_value)) return STATUS_BAD_NUMBER_FORMAT;
              if (letter == AXIS_1_NAME)  start[0] = f_value;
              else if (letter == AXIS_2_NAME)  start[1] = f_value;
              else if (letter == AXIS_3_NAME)  start[2] = f_value;
              else return STATUS_INVALID_STATEMENT;
              break;
            case 'E':
              char_counter++;
              letter = line[char_counter];
              char_counter++;
              if (!read_float(line, &char_counter, &f_value)) return STATUS_BAD_NUMBER_FORMAT;
              if (letter == AXIS_1_NAME)  end[0] = f_value;
              else if (letter == AXIS_2_NAME)  end[1] = f_value;
              else if (letter == AXIS_3_NAME)  end[2] = f_value;
              else return STATUS_INVALID_STATEMENT;
              break;
            case 'D':
              char_counter++;
              letter = line[char_counter];
              char_counter++;
              if (!read_float(line, &char_counter, &f_value)) return STATUS_BAD_NUMBER_FORMAT;
              int_value = trunc(f_value);
              if (letter == AXIS_1_NAME)  dim[0] = int_value;
              else if (letter == AXIS_2_NAME)  dim[1] = int_value;
              else return STATUS_INVALID_STATEMENT;
              break;
            default: return STATUS_INVALID_STATEMENT;
          }
        }
        if (!CheckCoordinatesIsInRange(start, end, dim)) return STATUS_GCODE_COORDINATES_OUT_OF_RANGE;
        AddComponent(comp, start, end, dim);
      }
      else if (line[char_counter]=='I') { //+flags
        char_counter++;
        float f_value; uint8_t comp;
        if (!read_float(line, &char_counter, &f_value)) return STATUS_BAD_NUMBER_FORMAT;
        comp = trunc(f_value);
        if (comp > MAX_COMPONENTS || comp < 1) return STATUS_GCODE_COMPONENT_OUT_OF_RANGE;
        char letter = line[char_counter];
        if (letter == 'U')  components[comp-1].flags &= ~COMP_FLAG_INSTALLED;
        else if (letter == 'I') 
        { 
          if (!CheckCoordinatesIsInRange(components[comp-1].coordinate_zero, components[comp-1].offset, components[comp-1].dimensions)) return STATUS_GCODE_COORDINATES_OUT_OF_RANGE;
          components[comp-1].flags |= COMP_FLAG_INSTALLED;
        }
        else return STATUS_INVALID_STATEMENT;
      }
      else return STATUS_INVALID_STATEMENT;
      //: type:S(set)/T(teach)/C(copy) OBJ_NUM keyword:S(start)/E(end)/D(dim)/I(installed)/L(needle_length) coords(if S, xyz)
      break;
    case 'D' :{//brackets!?
      int toReport;
      char_counter++;
      switch(line[char_counter]) {
        case 'N': report_syringe(); break;
        case 'C':
          if ( (sys.state & (STATE_CYCLE | STATE_HOLD | STATE_JOG)) | (sys.as_state & (STATE_AS_JOB | STATE_AS_COLLECT))) { return(STATUS_IDLE_ERROR); } // Block during cycle. Takes too long to print.
          toReport = 0xFFFF;
          report_components(toReport);
          break;
        case 'S': 
          printString("state\rsys="); print_uint8_base10(sys.state);
          printString("\rAS="); print_uint8_base10(sys.as_state); printString("\r\n");
          break;
        case 'P': 
          toReport = 0xFF;
          report_position(toReport); 
          break;
        case 'L': 
          report_limits();
          break;
        case 'I': 
          report_sigs();
          break;
        case 'G' : // Prints gcode parser state
          // TODO: Move this to realtime commands for GUIs to request this data during suspend-state.
          report_gcode_modes();
          break;
        default: return STATUS_INVALID_STATEMENT;
      }
      // L(tool_length)/C(components-can be specific)/S(status)/P(position)/G(gcodes)/L(limit pin states)/I(I/O pin states)
      break;
    }
    case 'S':
      if (line[2] != 'L')
      {
        char letter = 0;
        float f_value;
        uint8_t flags = syringe.flags;
        float max_volume = syringe.max_volume;
        float volume_per_step = syringe.volume_per_step;
        float overfill = syringe.overfill;
        float needle_offset = syringe.needle_offset;
        char_counter++;
        while (line[char_counter] != 0) {
          letter = line[char_counter];
          switch(letter) {
            case 'O'://4th
              char_counter++;
              if (!read_float(line, &char_counter, &f_value)) return STATUS_BAD_NUMBER_FORMAT;
              if (f_value >= 1)
                overfill = f_value;
              else return STATUS_SETTING_OUT_OF_RANGE;
              break;
            case 'N'://2nd
              char_counter++;
              if (!read_float(line, &char_counter, &f_value)) return STATUS_BAD_NUMBER_FORMAT;
              if (f_value >= 0)
                needle_offset = f_value;
              else return STATUS_SETTING_OUT_OF_RANGE;
              break;
            case 'V'://5th
              char_counter++;
              if (!read_float(line, &char_counter, &f_value)) return STATUS_BAD_NUMBER_FORMAT;
              if (f_value > 0 && (f_value/volume_per_step/settings.steps_per_mm[AXIS_4]*overfill < settings.max_travel[AXIS_4]*0.9))
                max_volume = f_value;
              else return STATUS_SETTING_OUT_OF_RANGE;
              break;
            case 'S'://3rd
              char_counter++;
              if (!read_float(line, &char_counter, &f_value)) return STATUS_BAD_NUMBER_FORMAT;
              if (f_value > 0 )
                volume_per_step = f_value;
              else return STATUS_SETTING_OUT_OF_RANGE;
              break;
            case 'F': //1st
              char_counter++;
              if (!read_float(line, &char_counter, &f_value)) return STATUS_BAD_NUMBER_FORMAT;
              int int_value = trunc(value);
              if (int_value >= 0 && int_value < 256)
                flags = int_value;
              else return STATUS_INVALID_STATEMENT;
              break;
            default: return STATUS_INVALID_STATEMENT;
          }
        }
        if (flags != syringe.flags || max_volume != syringe.max_volume || volume_per_step != syringe.volume_per_step || overfill != syringe.overfill || needle_offset != syringe.needle_offset)
        {
          if (needle_offset != syringe.needle_offset)
          {
            set_needle_offset(-syringe.needle_offset);
            syringe.needle_offset = needle_offset;
            set_needle_offset(syringe.needle_offset);
          }
          syringe.flags = flags;
          syringe.overfill = overfill;
          syringe.volume_per_step = volume_per_step;
          syringe.max_volume = max_volume;
          write_syringe_settings();
        }
        break;
      }
      else if((line[3] != 'P') || (line[4] != 0)) { return(STATUS_INVALID_STATEMENT); }
      system_set_exec_state_flag(EXEC_SLEEP); // Set to execute sleep mode immediately
      break;
    case '$': case 'C': case 'X':
      if ( line[2] != 0 ) { return(STATUS_INVALID_STATEMENT); }
      switch( line[1] ) {
        case '$' : // Prints Grbl settings
          if ( (sys.state & (STATE_CYCLE | STATE_HOLD | STATE_JOG)) | (sys.as_state & (STATE_AS_JOB | STATE_AS_COLLECT))) { return(STATUS_IDLE_ERROR); } // Block during cycle. Takes too long to print.
          else { report_grbl_settings(); }
          break;
        case 'C' : // Set check g-code mode [IDLE/CHECK]
          // Perform reset when toggling off. Check g-code mode should only work if Grbl
          // is idle and ready, regardless of alarm locks. This is mainly to keep things
          // simple and consistent.
          if ( sys.state == STATE_CHECK_MODE ) {
            mc_reset();
            report_feedback_message(MESSAGE_DISABLED);
          } else {
            if (sys.state | (sys.as_state & (STATE_AS_COLLECT | STATE_AS_JOB))) { return(STATUS_IDLE_ERROR); } // Requires no alarm mode.
            sys.state = STATE_CHECK_MODE;
            send_state();
            report_feedback_message(MESSAGE_ENABLED);
          }
          break;
        case 'X' : // Disable alarm lock [ALARM]
          if (sys.state == STATE_ALARM) {
            // Block if safety door is ajar.
            if (system_check_safety_door_ajar()) { return(STATUS_CHECK_DOOR); }
            else if (sys.as_state == STATE_AS_PARK)  { return(STATUS_IDLE_ERROR); }
            report_feedback_message(MESSAGE_ALARM_UNLOCK);
            sys.state = STATE_IDLE;
            send_state();
            // Don't run startup script. Prevents stored moves in startup from causing accidents.
          } // Otherwise, no effect.
          break;
      }
      break;
    default :
      // Block any system command that requires the state as IDLE/ALARM. (i.e. EEPROM, homing)
      if ( !((sys.state == STATE_IDLE || sys.state == STATE_ALARM) && (sys.as_state == STATE_IDLE || sys.as_state == STATE_AS_PARK)) ) { return(STATUS_IDLE_ERROR); }
      switch( line[1] ) {
        case '#' : // Print Grbl NGC parameters
          if ( line[2] != 0 ) { return(STATUS_INVALID_STATEMENT); }
          else { report_ngc_parameters(); }
          break;
        case 'H' : // Perform homing cycle [IDLE/ALARM]
          //if (bit_isfalse(settings.flags,BITFLAG_HOMING_ENABLE)) {return(STATUS_SETTING_DISABLED); }
          if (system_check_safety_door_ajar()) { return(STATUS_CHECK_DOOR); } // Block if safety door is ajar.
          sys.state = STATE_HOMING; // Set system state variable
          send_state();
          if (line[2] == 0) {
            mc_homing_cycle(HOMING_CYCLE_ALL);
          #ifdef HOMING_SINGLE_AXIS_COMMANDS
            } else if (line[3] == 0 && sys.as_state != STATE_AS_PARK) {
              switch (line[2]) {
                case 'X': mc_homing_cycle(axis_X_mask); break;
                case 'Y': mc_homing_cycle(axis_Y_mask); break;
                case 'Z': mc_homing_cycle(axis_Z_mask); break;
                case 'A':
                  if (axis_A_mask != 0) {
                    mc_homing_cycle(axis_A_mask);
                  } else {
                    return(STATUS_INVALID_STATEMENT);
                  }
                  break;
                case 'B':
                  if (axis_B_mask != 0) {
                    mc_homing_cycle(axis_B_mask);
                  } else {
                    return(STATUS_INVALID_STATEMENT);
                  }
                  break;
                case 'C':
                  if (axis_C_mask != 0) {
                    mc_homing_cycle(axis_C_mask);
                  } else {
                    return(STATUS_INVALID_STATEMENT);
                  }
                  break;
                case 'U':
                  if (axis_U_mask != 0) {
                    mc_homing_cycle(axis_U_mask);
                  } else {
                    return(STATUS_INVALID_STATEMENT);
                  }
                  break;
                case 'V':
                  if (axis_V_mask != 0) {
                    mc_homing_cycle(axis_V_mask);
                  } else {
                    return(STATUS_INVALID_STATEMENT);
                  }
                  break;
                case 'W':
                  if (axis_W_mask != 0) {
                    mc_homing_cycle(axis_W_mask);
                  } else {
                    return(STATUS_INVALID_STATEMENT);
                  }
                  break;
                case 'D':
                  if (axis_D_mask != 0) {
                    mc_homing_cycle(axis_D_mask);
                  } else {
                    return(STATUS_INVALID_STATEMENT);
                  }
                  break;
                case 'E':
                  if (axis_E_mask != 0) {
                    mc_homing_cycle(axis_E_mask);
                  } else {
                    return(STATUS_INVALID_STATEMENT);
                  }
                  break;
                case 'H':
                  if (axis_H_mask != 0) {
                    mc_homing_cycle(axis_H_mask);
                  } else {
                    return(STATUS_INVALID_STATEMENT);
                  }
                  break;
                default: return(STATUS_INVALID_STATEMENT);
              }
          #endif // HOMING_SINGLE_AXIS_COMMANDS
          } else { return(STATUS_INVALID_STATEMENT); }
          if (!sys.abort) {  // Execute startup scripts after successful homing.
            sys.state = STATE_IDLE; // Set to IDLE when complete.
            send_state();
            st_go_idle(); // Set steppers to the settings idle state before returning.
            if (line[2] == 0) { system_execute_startup(line); }
          }
          break;
        case 'I' : // Print or store build info. [IDLE/ALARM]
          if ( line[++char_counter] == 0 ) {
            settings_read_build_info(line);
            report_build_info(line);
          #ifdef ENABLE_BUILD_INFO_WRITE_COMMAND
            } else { // Store startup line [IDLE/ALARM]
              if(line[char_counter++] != '=') { return(STATUS_INVALID_STATEMENT); }
              helper_var = char_counter; // Set helper variable as counter to start of user info line.
              do {
                line[char_counter-helper_var] = line[char_counter];
              } while (line[char_counter++] != 0);
              settings_store_build_info(line);
          #endif
          }
          break;
        case 'R' : // Restore defaults [IDLE/ALARM]
          if ((line[2] != 'S') || (line[3] != 'T') || (line[4] != '=') || (line[6] != 0)) { return(STATUS_INVALID_STATEMENT); }
          switch (line[5]) {
            #ifdef ENABLE_RESTORE_EEPROM_DEFAULT_SETTINGS
              case '$': settings_restore(SETTINGS_RESTORE_DEFAULTS); break;
            #endif
            #ifdef ENABLE_RESTORE_EEPROM_CLEAR_PARAMETERS
              case '#': settings_restore(SETTINGS_RESTORE_PARAMETERS); break;
            #endif
            #ifdef ENABLE_RESTORE_EEPROM_WIPE_ALL
              case '*': reset_syringe(); reset_components(); settings_restore(SETTINGS_RESTORE_ALL); break;
              case 'S': reset_syringe(); break;
              case 'C': reset_components(); break;
            #endif
            default: return(STATUS_INVALID_STATEMENT);
          }
          report_feedback_message(MESSAGE_RESTORE_DEFAULTS);
          mc_reset(); // Force reset to ensure settings are initialized correctly.
          break;
        case 'N' : // Startup lines. [IDLE/ALARM]
          if ( line[++char_counter] == 0 ) { // Print startup lines
            for (helper_var=0; helper_var < N_STARTUP_LINE; helper_var++) {
              if (!(settings_read_startup_line(helper_var, line))) {
                report_status_message(STATUS_SETTING_READ_FAIL);
              } else {
                report_startup_line(helper_var,line);
              }
            }
            break;
          } else { // Store startup line [IDLE Only] Prevents motion during ALARM.
            if (sys.state != STATE_IDLE) { return(STATUS_IDLE_ERROR); } // Store only when idle.
            helper_var = true;  // Set helper_var to flag storing method.
            // No break. Continues into default: to read remaining command characters.
          }
        default :  // Storing setting methods [IDLE/ALARM]
          if(!read_float(line, &char_counter, &parameter)) { return(STATUS_BAD_NUMBER_FORMAT); }
          if(line[char_counter++] != '=') { return(STATUS_INVALID_STATEMENT); }
          if (helper_var) { // Store startup line
            // Prepare sending gcode block to gcode parser by shifting all characters
            helper_var = char_counter; // Set helper variable as counter to start of gcode block
            do {
              line[char_counter-helper_var] = line[char_counter];
            } while (line[char_counter++] != 0);
            if (char_counter > EEPROM_LINE_SIZE) { return(STATUS_LINE_LENGTH_EXCEEDED); }
            // Execute gcode block to ensure block is valid.
            helper_var = gc_execute_line(line); // Set helper_var to returned status code.
            if (helper_var) { return(helper_var); }
            else {
              helper_var = trunc(parameter); // Set helper_var to int value of parameter
              settings_store_startup_line(helper_var,line);
            }
          } else { // Store global setting.
            if(!read_float(line, &char_counter, &value)) { return(STATUS_BAD_NUMBER_FORMAT); }
            if((line[char_counter] != 0) || (parameter > 255)) { return(STATUS_INVALID_STATEMENT); }
            return(settings_store_global_setting((uint8_t)parameter, value));
          }
      }
  }
  return(STATUS_OK); // If '$' command makes it to here, then everything's ok.
}



void system_flag_wco_change()
{
  #ifdef FORCE_BUFFER_SYNC_DURING_WCO_CHANGE
    protocol_buffer_synchronize();
  #endif
  sys.report_wco_counter = 0;
}


// Returns machine position of axis 'idx'. Must be sent a 'step' array.
// NOTE: If motor steps and machine position are not in the same coordinate frame, this function
//   serves as a central place to compute the transformation.
float system_convert_axis_steps_to_mpos(int32_t *steps, uint8_t idx)
{
  float pos;
  #ifdef COREXY
    if (idx==AXIS_1) {
      pos = (float)system_convert_corexy_to_x_axis_steps(steps) / settings.steps_per_mm[idx];
    } else if (idx==AXIS_2) {
      pos = (float)system_convert_corexy_to_y_axis_steps(steps) / settings.steps_per_mm[idx];
    } else {
      pos = steps[idx]/settings.steps_per_mm[idx];
    }
  #else
    pos = steps[idx]/settings.steps_per_mm[idx];
  #endif
  return(pos);
}


void system_convert_array_steps_to_mpos(float *position, int32_t *steps)
{
  uint8_t idx;
  for (idx=0; idx<N_AXIS; idx++) {
    position[idx] = system_convert_axis_steps_to_mpos(steps, idx);
  }
  return;
}


// CoreXY calculation only. Returns x or y-axis "steps" based on CoreXY motor steps.
#ifdef COREXY
  int32_t system_convert_corexy_to_x_axis_steps(int32_t *steps)
  {
    return( (steps[A_MOTOR] + steps[B_MOTOR])/2 );
  }
  int32_t system_convert_corexy_to_y_axis_steps(int32_t *steps)
  {
    return( (steps[A_MOTOR] - steps[B_MOTOR])/2 );
  }
#endif


// Checks and reports if target array exceeds machine travel limits.
uint8_t system_check_travel_limits(float *target)
{
  uint8_t idx;
  for (idx=0; idx<N_AXIS; idx++) {
    // Ignore soft limit if AXIS_MAX_TRAVEL == 0 (parameter $130 to $135)
    if (settings.max_travel[idx] != 0) {
      #ifdef HOMING_FORCE_SET_ORIGIN
        // When homing forced set origin is enabled, soft limits checks need to account for directionality.
        // NOTE: max_travel is stored as negative; ratio=pos/total
        if (idx==AXIS_3) {
          if ((target[idx] > (-settings.max_travel[idx]*settings.ratio[idx])) || (target[idx] < (settings.max_travel[idx]*(1-settings.ratio[idx])+syringe.needle_offset))) { return(true); }
        }
        else {
          if ((target[idx] > (-settings.max_travel[idx]*settings.ratio[idx])) || (target[idx] < (settings.max_travel[idx]*(1-settings.ratio[idx])))) { return(true); }
        }
      #else
        // NOTE: max_travel is stored as negative
        if (target[idx] > 0 || target[idx] < settings.max_travel[idx]) { return(true); }
      #endif
    }
  }
  return(false);
}

void setResetDisable(bool state)
{
  if(state)
    SOFTWARE_RESET_DDR |= (1 << SOFTWARE_RESET_BIT); //Set PD5 as output to put 5V on reset line
  else
    SOFTWARE_RESET_DDR &= ~(1 << SOFTWARE_RESET_BIT); //set back to input mode
}

// Special handlers for setting and clearing Grbl's real-time execution flags.
void system_set_exec_state_flag(uint8_t mask) {
  uint8_t sreg = SREG;
  cli();
  sys_rt_exec_state |= (mask);
  SREG = sreg;
}

void system_clear_exec_state_flag(uint8_t mask) {
  uint8_t sreg = SREG;
  cli();
  sys_rt_exec_state &= ~(mask);
  SREG = sreg;
}

void system_set_exec_alarm(uint8_t code) {
  uint8_t sreg = SREG;
  cli();
  sys_rt_exec_alarm = code;
  SREG = sreg;
}

void system_set_exec_report(uint8_t code) {
  uint8_t sreg = SREG;
  cli();
  sys_rt_exec_report = code;
  SREG = sreg;
}

void system_clear_exec_alarm() {
  uint8_t sreg = SREG;
  cli();
  sys_rt_exec_alarm = 0;
  SREG = sreg;
}

void system_set_exec_motion_override_flag(uint8_t mask) {
  uint8_t sreg = SREG;
  cli();
  sys_rt_exec_motion_override |= (mask);
  SREG = sreg;
}

void system_set_exec_accessory_override_flag(uint8_t mask) {
  uint8_t sreg = SREG;
  cli();
  sys_rt_exec_accessory_override |= (mask);
  SREG = sreg;
}

void system_clear_exec_motion_overrides() {
  uint8_t sreg = SREG;
  cli();
  sys_rt_exec_motion_override = 0;
  SREG = sreg;
}

void system_clear_exec_accessory_overrides() {
  uint8_t sreg = SREG;
  cli();
  sys_rt_exec_accessory_override = 0;
  SREG = sreg;
}

void system_clear_exec_report() {
  uint8_t sreg = SREG;
  cli();
  sys_rt_exec_report = 0;
  SREG = sreg;
}


