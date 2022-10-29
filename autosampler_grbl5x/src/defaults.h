/*
  defaults.h - defaults settings configuration file
  Part of Grbl

  Copyright (c) 2017-2018 Gauthier Briere
  Copyright (c) 2012-2016 Sungeun K. Jeon for Gnea Research LLC

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

/* The defaults.h file serves as a central default settings selector for different machine
   types, from DIY CNC mills to CNC conversions of off-the-shelf machines. The settings
   files listed here are supplied by users, so your results may vary. However, this should
   give you a good starting point as you get to know your machine and tweak the settings for
   your nefarious needs.
   NOTE: Ensure one and only one of these DEFAULTS_XXX values is defined in config.h */

#ifndef defaults_h

  #ifdef DEFAULTS_GENERIC
    // Generic conservative settings for a RAMP CNC machine. You must update these yourself. 
    // Keep in mind that Grbl is highly efficient and settings can be significantly different.
    // Especially when optimizing for a different CNC task like going from 3d printing to 
    // CNC milling or laser cutting. Unlike Marlin, these defaults are only applied when the 
    // EEPROM is explicitly wiped, either by a `$RST=*` command or Grbl detecting a settings
    // version type change (not frequent).
    // https://blog.prusaprinters.org/calculator_3416/
    #define STEPS_ONLY_OPMODE
    #define STEPS_CENTRIC_OPMODE

    #define STEP_MAX_FREQUENCY 10000
    #define SECONDS_PER_MINUTE 60

    #ifdef STEPS_ONLY_OPMODE
    #define DEFAULT_AXIS1_STEPS_PER_UNIT 16.0
    #define DEFAULT_AXIS2_STEPS_PER_UNIT 16.0
    #define DEFAULT_AXIS3_STEPS_PER_UNIT 16.0
    #else
    #define MICROSTEPS_AXIS1 16     // Microstepping = 1/4 pas
    #define MICROSTEPS_AXIS2 16     // Microstepping = 1/4 pas
    #define MICROSTEPS_AXIS3 16     // Microstepping = 1/4 pas
    #define STEP_REVS_AXIS1 200    // Moteurs à 200 pas par tour
    #define STEP_REVS_AXIS2 200    // Moteurs à 200 pas par tour
    #define STEP_REVS_AXIS3 200    // Moteurs à 200 pas par tour
    #define UNIT_PER_REV_AXIS1 2.0 // 2mm
    #define UNIT_PER_REV_AXIS2 2.0 // 2mm
    #define UNIT_PER_REV_AXIS3 2.0 // 2mm
    #define DEFAULT_AXIS1_STEPS_PER_UNIT (MICROSTEPS_AXIS1*STEP_REVS_AXIS1/UNIT_PER_REV_AXIS1) // 400
    #define DEFAULT_AXIS2_STEPS_PER_UNIT (MICROSTEPS_AXIS2*STEP_REVS_AXIS2/UNIT_PER_REV_AXIS2)
    #define DEFAULT_AXIS3_STEPS_PER_UNIT (MICROSTEPS_AXIS3*STEP_REVS_AXIS3/UNIT_PER_REV_AXIS3)
    #endif

    #define DEFAULT_AXIS1_MAX_RATE (STEP_MAX_FREQUENCY/DEFAULT_AXIS1_STEPS_PER_UNIT*SECONDS_PER_MINUTE) // 3000
    #define DEFAULT_AXIS2_MAX_RATE (STEP_MAX_FREQUENCY/DEFAULT_AXIS2_STEPS_PER_UNIT*SECONDS_PER_MINUTE)
    #define DEFAULT_AXIS3_MAX_RATE (STEP_MAX_FREQUENCY/DEFAULT_AXIS3_STEPS_PER_UNIT*SECONDS_PER_MINUTE)

    #ifdef STEPS_CENTRIC_OPMODE
    #define DEFAULT_AXIS1_MAX_TRAVEL (30000/DEFAULT_AXIS1_STEPS_PER_UNIT) // mm
    #define DEFAULT_AXIS2_MAX_TRAVEL (15000/DEFAULT_AXIS2_STEPS_PER_UNIT) // mm
    #define DEFAULT_AXIS3_MAX_TRAVEL (15000/DEFAULT_AXIS3_STEPS_PER_UNIT) // mm
    #define DEFAULT_AXIS1_ACCELERATION (120000/DEFAULT_AXIS1_STEPS_PER_UNIT*SECONDS_PER_MINUTE*SECONDS_PER_MINUTE) // 300*60*60 mm/min^2 = 300 mm/sec^2
    #define DEFAULT_AXIS2_ACCELERATION (120000/DEFAULT_AXIS1_STEPS_PER_UNIT*SECONDS_PER_MINUTE*SECONDS_PER_MINUTE) // 300*60*60 mm/min^2 = 300 mm/sec^2
    #define DEFAULT_AXIS3_ACCELERATION (120000/DEFAULT_AXIS1_STEPS_PER_UNIT*SECONDS_PER_MINUTE*SECONDS_PER_MINUTE) // 100*60*60 mm/min^2 = 100 mm/sec^2
    #else
    #define DEFAULT_AXIS1_MAX_TRAVEL 400.0 // mm
    #define DEFAULT_AXIS2_MAX_TRAVEL 200.0 // mm
    #define DEFAULT_AXIS3_MAX_TRAVEL 200.0 // mm
    #define DEFAULT_AXIS1_ACCELERATION (300.0*SECONDS_PER_MINUTE*SECONDS_PER_MINUTE) // 300*60*60 mm/min^2 = 300 mm/sec^2
    #define DEFAULT_AXIS2_ACCELERATION (300.0*SECONDS_PER_MINUTE*SECONDS_PER_MINUTE) // 300*60*60 mm/min^2 = 300 mm/sec^2
    #define DEFAULT_AXIS3_ACCELERATION (300.0*SECONDS_PER_MINUTE*SECONDS_PER_MINUTE) // 100*60*60 mm/min^2 = 100 mm/sec^2
    #endif
    #define DEFAULT_AXIS1_RATIO 0.5 // pos/total
    #define DEFAULT_AXIS2_RATIO 1 // pos/total
    #define DEFAULT_AXIS3_RATIO 1 // pos/total
    #if N_AXIS > 3
      #if AXIS_4_NAME != AXIS_1_NAME && AXIS_4_NAME != AXIS_2_NAME && AXIS_4_NAME != AXIS_3_NAME
        #ifdef STEPS_ONLY_OPMODE
        #define DEFAULT_AXIS4_STEPS_PER_UNIT 16.0
        #else
        #define MICROSTEPS_AXIS4 16     // Microstepping = 1/4 pas
        #define STEP_REVS_AXIS4 200    // Moteurs à 200 pas par tour
        #define UNIT_PER_REV_AXIS4 2.0 // mm
        #define DEFAULT_AXIS4_STEPS_PER_UNIT (MICROSTEPS_AXIS4*STEP_REVS_AXIS4/UNIT_PER_REV_AXIS4)
        #endif
        #define DEFAULT_AXIS4_MAX_RATE (STEP_MAX_FREQUENCY/DEFAULT_AXIS4_STEPS_PER_UNIT*SECONDS_PER_MINUTE)
        #define DEFAULT_AXIS4_ACCELERATION (120000/DEFAULT_AXIS1_STEPS_PER_UNIT*SECONDS_PER_MINUTE*SECONDS_PER_MINUTE) // 100*60*60 mm/min^2 = 100 mm/sec^2
        #ifdef STEPS_CENTRIC_OPMODE
        #define DEFAULT_AXIS4_MAX_TRAVEL (8000/DEFAULT_AXIS4_STEPS_PER_UNIT) // mm
        #else
        #define DEFAULT_AXIS4_MAX_TRAVEL 100 // mm
        #endif
        #define DEFAULT_AXIS4_RATIO 1 // pos/total
      #elif AXIS_4_NAME == AXIS_1_NAME
        #define DEFAULT_AXIS4_STEPS_PER_UNIT DEFAULT_AXIS1_STEPS_PER_UNIT
        #define DEFAULT_AXIS4_MAX_RATE DEFAULT_AXIS1_MAX_RATE
        #define DEFAULT_AXIS4_ACCELERATION DEFAULT_AXIS1_ACCELERATION
        #define DEFAULT_AXIS4_MAX_TRAVEL DEFAULT_AXIS1_MAX_TRAVEL
      #elif AXIS_4_NAME == AXIS_2_NAME
        #define DEFAULT_AXIS4_STEPS_PER_UNIT DEFAULT_AXIS2_STEPS_PER_UNIT
        #define DEFAULT_AXIS4_MAX_RATE DEFAULT_AXIS2_MAX_RATE
        #define DEFAULT_AXIS4_ACCELERATION DEFAULT_AXIS2_ACCELERATION
        #define DEFAULT_AXIS4_MAX_TRAVEL DEFAULT_AXIS2_MAX_TRAVEL
      #else
        #define DEFAULT_AXIS4_STEPS_PER_UNIT DEFAULT_AXIS3_STEPS_PER_UNIT
        #define DEFAULT_AXIS4_MAX_RATE DEFAULT_AXIS3_MAX_RATE
        #define DEFAULT_AXIS4_ACCELERATION DEFAULT_AXIS3_ACCELERATION
        #define DEFAULT_AXIS4_MAX_TRAVEL DEFAULT_AXIS3_MAX_TRAVEL
      #endif
    #endif
    #if N_AXIS > 4
      #if AXIS_5_NAME != AXIS_1_NAME && AXIS_5_NAME != AXIS_2_NAME && AXIS_5_NAME != AXIS_3_NAME && \
          AXIS_5_NAME != AXIS_4_NAME
        #define DEFAULT_AXIS5_STEPS_PER_UNIT 8.888889 // Direct drive : (200 pas par tours * 1/16 microsteps)/360°
        #define DEFAULT_AXIS5_MAX_RATE 1440 // °/mn
        #define DEFAULT_AXIS5_ACCELERATION (50.0*60*60) // 100*60*60 mm/min^2 = 100 mm/sec^2
        #define DEFAULT_AXIS5_MAX_TRAVEL 180.0 // °
      #elif AXIS_5_NAME == AXIS_1_NAME
        #define DEFAULT_AXIS5_STEPS_PER_UNIT DEFAULT_AXIS1_STEPS_PER_UNIT
        #define DEFAULT_AXIS5_MAX_RATE DEFAULT_AXIS1_MAX_RATE
        #define DEFAULT_AXIS5_ACCELERATION DEFAULT_AXIS1_ACCELERATION
        #define DEFAULT_AXIS5_MAX_TRAVEL DEFAULT_AXIS1_MAX_TRAVEL
      #elif AXIS_5_NAME == AXIS_2_NAME
        #define DEFAULT_AXIS5_STEPS_PER_UNIT DEFAULT_AXIS2_STEPS_PER_UNIT
        #define DEFAULT_AXIS5_MAX_RATE DEFAULT_AXIS2_MAX_RATE
        #define DEFAULT_AXIS5_ACCELERATION DEFAULT_AXIS2_ACCELERATION
        #define DEFAULT_AXIS5_MAX_TRAVEL DEFAULT_AXIS2_MAX_TRAVEL
      #elif AXIS_5_NAME == AXIS_3_NAME
        #define DEFAULT_AXIS5_STEPS_PER_UNIT DEFAULT_AXIS3_STEPS_PER_UNIT
        #define DEFAULT_AXIS5_MAX_RATE DEFAULT_AXIS3_MAX_RATE
        #define DEFAULT_AXIS5_ACCELERATION DEFAULT_AXIS3_ACCELERATION
        #define DEFAULT_AXIS5_MAX_TRAVEL DEFAULT_AXIS3_MAX_TRAVEL
      #else
        #define DEFAULT_AXIS5_STEPS_PER_UNIT DEFAULT_AXIS4_STEPS_PER_UNIT
        #define DEFAULT_AXIS5_MAX_RATE DEFAULT_AXIS4_MAX_RATE
        #define DEFAULT_AXIS5_ACCELERATION DEFAULT_AXIS4_ACCELERATION
        #define DEFAULT_AXIS5_MAX_TRAVEL DEFAULT_AXIS4_MAX_TRAVEL
      #endif
    #endif
    #if N_AXIS > 5
      #if AXIS_6_NAME != AXIS_1_NAME && AXIS_6_NAME != AXIS_2_NAME && \
          AXIS_6_NAME != AXIS_3_NAME && AXIS_6_NAME != AXIS_4_NAME && AXIS_6_NAME != AXIS_5_NAME
        #define DEFAULT_AXIS6_STEPS_PER_UNIT 8.888889 // Direct drive : (200 pas par tours * 1/16 microsteps)/360°
        #define DEFAULT_AXIS6_MAX_RATE 1440 // °/mn
        #define DEFAULT_AXIS6_ACCELERATION (50.0*60*60) // 100*60*60 mm/min^2 = 100 mm/sec^2
        #define DEFAULT_AXIS6_MAX_TRAVEL 180.0 // °
      #elif AXIS_6_NAME == AXIS_1_NAME
        #define DEFAULT_AXIS6_STEPS_PER_UNIT DEFAULT_AXIS1_STEPS_PER_UNIT
        #define DEFAULT_AXIS6_MAX_RATE DEFAULT_AXIS1_MAX_RATE
        #define DEFAULT_AXIS6_ACCELERATION DEFAULT_AXIS1_ACCELERATION
        #define DEFAULT_AXIS6_MAX_TRAVEL DEFAULT_AXIS1_MAX_TRAVEL
      #elif AXIS_6_NAME == AXIS_2_NAME
        #define DEFAULT_AXIS6_STEPS_PER_UNIT DEFAULT_AXIS2_STEPS_PER_UNIT
        #define DEFAULT_AXIS6_MAX_RATE DEFAULT_AXIS2_MAX_RATE
        #define DEFAULT_AXIS6_ACCELERATION DEFAULT_AXIS2_ACCELERATION
        #define DEFAULT_AXIS6_MAX_TRAVEL DEFAULT_AXIS2_MAX_TRAVEL
      #elif AXIS_6_NAME == AXIS_3_NAME
        #define DEFAULT_AXIS6_STEPS_PER_UNIT DEFAULT_AXIS3_STEPS_PER_UNIT
        #define DEFAULT_AXIS6_MAX_RATE DEFAULT_AXIS3_MAX_RATE
        #define DEFAULT_AXIS6_ACCELERATION DEFAULT_AXIS3_ACCELERATION
        #define DEFAULT_AXIS6_MAX_TRAVEL DEFAULT_AXIS3_MAX_TRAVEL
      #elif AXIS_6_NAME == AXIS_4_NAME
        #define DEFAULT_AXIS6_STEPS_PER_UNIT DEFAULT_AXIS4_STEPS_PER_UNIT
        #define DEFAULT_AXIS6_MAX_RATE DEFAULT_AXIS4_MAX_RATE
        #define DEFAULT_AXIS6_ACCELERATION DEFAULT_AXIS4_ACCELERATION
        #define DEFAULT_AXIS6_MAX_TRAVEL DEFAULT_AXIS4_MAX_TRAVEL
      #else
        #define DEFAULT_AXIS6_STEPS_PER_UNIT DEFAULT_AXIS5_STEPS_PER_UNIT
        #define DEFAULT_AXIS6_MAX_RATE DEFAULT_AXIS5_MAX_RATE
        #define DEFAULT_AXIS6_ACCELERATION DEFAULT_AXIS5_ACCELERATION
        #define DEFAULT_AXIS6_MAX_TRAVEL DEFAULT_AXIS5_MAX_TRAVEL
      #endif
    #endif
    #define DEFAULT_SPINDLE_RPM_MAX 12000 // rpm
    #define DEFAULT_SPINDLE_RPM_MIN 550.0 // rpm
    #define DEFAULT_STEP_PULSE_MICROSECONDS 10
    #define DEFAULT_DISABLE_RESET 1
    #define DEFAULT_SIG_OUT_INVERT_MASK 0
    #define DEFAULT_SIG_IN_INVERT_MASK 0
    #define DEFAULT_STEPPING_INVERT_MASK 0
    #define DEFAULT_DIRECTION_INVERT_MASK 0
    #define DEFAULT_STEPPER_IDLE_LOCK_TIME 255 // msec (0-254, 255 keeps steppers enabled)
    #define DEFAULT_STATUS_REPORT_MASK 1 // MPos enabled
    #define DEFAULT_JUNCTION_DEVIATION 0.02 // mm
    #define DEFAULT_ARC_TOLERANCE 0.002 // mm
    #define DEFAULT_REPORT_INCHES 0 // false
    #define DEFAULT_INVERT_ST_ENABLE 0 // false
    #define DEFAULT_INVERT_LIMIT_PINS 0 // false
    #define DEFAULT_SOFT_LIMIT_ENABLE 1 // true
    #define DEFAULT_HARD_LIMIT_ENABLE 0 // false
    #define DEFAULT_INVERT_PROBE_PIN 0 // false
    #define DEFAULT_LASER_MODE 0 // false
    #define DEFAULT_HOMING_ENABLE 0 // true
    #define DEFAULT_HOMING_DIR_MASK 1 // move positive dir
    #define DEFAULT_LIMIT_INVERT_MASK 0
    #define DEFAULT_HOMING_FEED_RATE 36000/DEFAULT_AXIS1_STEPS_PER_UNIT // mm/min
    #define DEFAULT_HOMING_SEEK_RATE 300000/DEFAULT_AXIS1_STEPS_PER_UNIT // mm/min
    #define DEFAULT_HOMING_DEBOUNCE_DELAY 500 // msec (0-65k)
    #define DEFAULT_HOMING_PULLOFF 96/DEFAULT_AXIS1_STEPS_PER_UNIT // mm
    #define MAX_FEED_RATE min(min(DEFAULT_AXIS1_MAX_RATE, DEFAULT_AXIS3_MAX_RATE), min(DEFAULT_AXIS2_MAX_RATE, DEFAULT_AXIS3_MAX_RATE))
  #endif 

#endif