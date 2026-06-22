#include <MIDI.h>
#include <EEPROM.h>

// Create a MIDI instance
MIDI_CREATE_DEFAULT_INSTANCE();
// Define MIDI input channels
#define MIDI_INPUT_CHANNEL_1 3
#define MIDI_INPUT_CHANNEL_2 4
// Define MIDI output channels
#define MIDI_OUTPUT_CHANNEL_1 3
#define MIDI_OUTPUT_CHANNEL_2 4
// Channel for merge mode via one of the output channels
#define MERGE_MODE_OUTPUT_CHANNEL MIDI_OUTPUT_CHANNEL_1
// Channel for switching between projects
#define PROJECT_SWITCH_CHANNEL 16
// EEPROM layout: preset_1 at [0-63], preset_2 at [64-127]
#define EEPROM_PRESET_2_OFFSET 64
// CC number for master filter (used to enter patch select mode)
#define CC_MASTER_FILTER 0x4A
// Note sent during patch select mode for auditioning
#define AUDITION_NOTE 60

// Variables
uint8_t current_project;  // Current CIRCUIT project
uint8_t preset_1;
uint8_t preset_2;
// Flags
bool merge_mode = false;         // "Merge mode" sends data from channel 4 to MERGE_MODE_OUTPUT_CHANNEL
bool mode_patch_select = false;  // Patch selection mode
bool merged_output_has_preset_1 = false;

void setup() {
  // MIDI initialization
  MIDI.begin(MIDI_CHANNEL_OMNI);
  // MIDI thru disabled
  MIDI.turnThruOff();
  // Attach handler for Program Change
  MIDI.setHandleProgramChange(handleProgramChange);
  // Set handlers for Note On and Note Off
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  // Set handler for Control Change
  MIDI.setHandleControlChange(handleControlChange);
  // Set handler for Real-Time messages (Clock, Start, Stop, Continue)
  MIDI.setHandleClock(handleRealTimeClock);
  MIDI.setHandleStart(handleRealTimeStart);
  MIDI.setHandleStop(handleRealTimeStop);
}

void loop() {
  // MIDI reading
  MIDI.read();
}

// Program change
void handleProgramChange(uint8_t channel, uint8_t project) {
  if (channel == PROJECT_SWITCH_CHANNEL)  // If project is changed on Circuit Tracks - restore presets from EEPROM for MIDI 1 and MIDI 2 outputs
  {
    preset_1 = EEPROM.read(project);  // Read presets from EEPROM
    preset_2 = EEPROM.read(project + EEPROM_PRESET_2_OFFSET);
    current_project = project;  // 0 - 63
    // Send program change MIDI 1
    MIDI.sendProgramChange(preset_1, MIDI_OUTPUT_CHANNEL_1);
    // Send program change MIDI 2
    MIDI.sendProgramChange(preset_2, MIDI_OUTPUT_CHANNEL_2);
  }
}

// Note On handler
void handleNoteOn(byte channel, byte note, byte velocity) {
  static uint8_t prev_preset_1;
  static uint8_t prev_preset_2;
  if (channel == MIDI_INPUT_CHANNEL_1) {
    if (merge_mode) {
      if (merged_output_has_preset_1 && preset_1 != preset_2) {
        MIDI.sendProgramChange(preset_1, MIDI_OUTPUT_CHANNEL_1);
        merged_output_has_preset_1 = false;
      }
    }
    if (mode_patch_select) {
      preset_1 = note;
      // Send program change only once
      if (prev_preset_1 != note) {
        MIDI.sendProgramChange(preset_1, MIDI_OUTPUT_CHANNEL_1);
        EEPROM.write(current_project, preset_1);
        prev_preset_1 = preset_1;
      }
      MIDI.sendNoteOn(AUDITION_NOTE, velocity, channel);
    } else {
      MIDI.sendNoteOn(note, velocity, channel);
    }
  } else if (channel == MIDI_INPUT_CHANNEL_2) {
    if (merge_mode)  // If merge mode activated output to channel 3
    {
      channel = MERGE_MODE_OUTPUT_CHANNEL;
      if (!merged_output_has_preset_1 && preset_1 != preset_2) {
        MIDI.sendProgramChange(preset_2, channel);
        merged_output_has_preset_1 = true;
      }
    }
    if (mode_patch_select) {
      preset_2 = note;
      // Send program change only once
      if (prev_preset_2 != note) {
        MIDI.sendProgramChange(preset_2, channel);
        EEPROM.write(current_project + EEPROM_PRESET_2_OFFSET, preset_2);
        prev_preset_2 = preset_2;
      }
      MIDI.sendNoteOn(AUDITION_NOTE, velocity, channel);
    } else {
      MIDI.sendNoteOn(note, velocity, channel);
    }
  }
}

// Note Off handler
void handleNoteOff(byte channel, byte note, byte velocity) {
  if (channel == MIDI_INPUT_CHANNEL_1) {
    if (mode_patch_select) {
      MIDI.sendNoteOff(AUDITION_NOTE, velocity, channel);
    } else {
      MIDI.sendNoteOff(note, velocity, channel);
    }
  } else if (channel == MIDI_INPUT_CHANNEL_2) {
    if (merge_mode) {
      channel = MERGE_MODE_OUTPUT_CHANNEL;
    }
    if (mode_patch_select) {
      MIDI.sendNoteOff(AUDITION_NOTE, velocity, channel);
    } else {
      MIDI.sendNoteOff(note, velocity, channel);
    }
  }
}

// Control Change (CC) handler
void handleControlChange(byte channel, byte control, byte value) {
  if (channel == MIDI_INPUT_CHANNEL_1) {
    MIDI.sendControlChange(control, value, channel);
  } else if (channel == MIDI_INPUT_CHANNEL_2) {
    if (merge_mode) {
      channel = MERGE_MODE_OUTPUT_CHANNEL;
    }
    MIDI.sendControlChange(control, value, channel);
  }
  if (channel == PROJECT_SWITCH_CHANNEL && control == CC_MASTER_FILTER && value == 0x00)  // If master filter pot is set to minimum
  {
    mode_patch_select = true;
  } else {
    mode_patch_select = false;
  }
}

void handleRealTimeClock() {
  MIDI.sendClock();  // Relay the message to the MIDI output
}

void handleRealTimeStart() {
  MIDI.sendStart();  // Relay the message to the MIDI output
}

void handleRealTimeStop() {
  MIDI.sendStop();  // Relay the message to the MIDI output
}
