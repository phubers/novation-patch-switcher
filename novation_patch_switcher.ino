#include <MIDI.h>
#include <EEPROM.h>

// Create a MIDI instance
MIDI_CREATE_DEFAULT_INSTANCE();
// Define MIDI input channels
#define MIDI_INPUT_CHANNEL_1 3
#define MIDI_INPUT_CHANNEL_2 4
// Define MIDI output channels (synths 1-2 active in 2-synth mode, all four in 4-synth mode)
#define MIDI_OUTPUT_CHANNEL_1 3
#define MIDI_OUTPUT_CHANNEL_2 4
#define MIDI_OUTPUT_CHANNEL_3 5
#define MIDI_OUTPUT_CHANNEL_4 6
// Channel for merge mode via one of the output channels
#define MERGE_MODE_OUTPUT_CHANNEL MIDI_OUTPUT_CHANNEL_1
// Channel for switching between projects
#define PROJECT_SWITCH_CHANNEL 16
// EEPROM layout: 64 bytes per synth (0-63, 64-127, 128-191, 192-255)
#define EEPROM_PRESET_2_OFFSET 64
#define EEPROM_PRESET_3_OFFSET 128
#define EEPROM_PRESET_4_OFFSET 192
// CC number for master filter (used to enter patch select mode)
#define CC_MASTER_FILTER 0x4A
// Note sent during patch select mode for auditioning
#define AUDITION_NOTE 60

// Hardware pins for mode switch
#define MODE_BUTTON_PIN 2
#define MODE_LED_PIN 3

// Variables
uint8_t current_project;  // Current CIRCUIT project
uint8_t preset_1;
uint8_t preset_2;
uint8_t preset_3;
uint8_t preset_4;
// Flags
bool merge_mode = false;         // "Merge mode" sends data from channel 4 to MERGE_MODE_OUTPUT_CHANNEL
bool mode_patch_select = false;  // Patch selection mode
bool merged_output_has_preset_1 = false;
bool four_synth_mode = false;    // false = 2x128 patches (ch3/4), true = 4x64 patches (ch3-6)

void setup() {
  // Mode button and LED
  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(MODE_LED_PIN, OUTPUT);
  digitalWrite(MODE_LED_PIN, four_synth_mode ? HIGH : LOW);

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
  // Check mode button
  static bool last_button_state = HIGH;
  bool button_state = digitalRead(MODE_BUTTON_PIN);
  if (last_button_state == HIGH && button_state == LOW) {
    four_synth_mode = !four_synth_mode;
    digitalWrite(MODE_LED_PIN, four_synth_mode ? HIGH : LOW);
  }
  last_button_state = button_state;

  // MIDI reading
  MIDI.read();
}

// Program change
void handleProgramChange(uint8_t channel, uint8_t project) {
  if (channel == PROJECT_SWITCH_CHANNEL)  // If project is changed on Circuit Tracks - restore presets from EEPROM for MIDI 1 and MIDI 2 outputs
  {
    // CT sends 0-63 for immediate, 64-127 for delayed (at pattern end)
    // Map to 0-63 range for synths with only 64 presets
    bool delayed = project >= 64;
    uint8_t mapped_project = project % 64;

    current_project = mapped_project;  // 0 - 63

    // Read and send presets from EEPROM
    preset_1 = EEPROM.read(mapped_project);
    preset_2 = EEPROM.read(mapped_project + EEPROM_PRESET_2_OFFSET);
    MIDI.sendProgramChange(preset_1, MIDI_OUTPUT_CHANNEL_1);
    MIDI.sendProgramChange(preset_2, MIDI_OUTPUT_CHANNEL_2);

    if (four_synth_mode) {
      preset_3 = EEPROM.read(mapped_project + EEPROM_PRESET_3_OFFSET);
      preset_4 = EEPROM.read(mapped_project + EEPROM_PRESET_4_OFFSET);
      MIDI.sendProgramChange(preset_3, MIDI_OUTPUT_CHANNEL_3);
      MIDI.sendProgramChange(preset_4, MIDI_OUTPUT_CHANNEL_4);
    }
  }
}

// Note On handler
void handleNoteOn(byte channel, byte note, byte velocity) {
  static uint8_t prev_preset_1;
  static uint8_t prev_preset_2;
  static uint8_t prev_preset_3;
  static uint8_t prev_preset_4;

  if (channel == MIDI_INPUT_CHANNEL_1) {
    if (merge_mode) {
      if (merged_output_has_preset_1 && preset_1 != preset_2) {
        MIDI.sendProgramChange(preset_1, MIDI_OUTPUT_CHANNEL_1);
        merged_output_has_preset_1 = false;
      }
    }
    if (mode_patch_select) {
      if (four_synth_mode && note >= 64) {
        // 4-synth mode: Notes 64-127 -> synth 3
        preset_3 = note - 64;
        if (prev_preset_3 != preset_3) {
          MIDI.sendProgramChange(preset_3, MIDI_OUTPUT_CHANNEL_3);
          EEPROM.write(current_project + EEPROM_PRESET_3_OFFSET, preset_3);
          prev_preset_3 = preset_3;
        }
      } else {
        // 2-synth mode: all notes, or 4-synth mode: notes 0-63 -> synth 1
        preset_1 = note;  // both cases: direct mapping
        if (prev_preset_1 != note) {
          MIDI.sendProgramChange(preset_1, MIDI_OUTPUT_CHANNEL_1);
          EEPROM.write(current_project, preset_1);
          prev_preset_1 = preset_1;
        }
        MIDI.sendNoteOn(AUDITION_NOTE, velocity, MIDI_OUTPUT_CHANNEL_1);
      }
    } else {
      MIDI.sendNoteOn(note, velocity, channel);
    }
  } else if (channel == MIDI_INPUT_CHANNEL_2) {
    if (merge_mode) {
      channel = MERGE_MODE_OUTPUT_CHANNEL;
      if (!merged_output_has_preset_1 && preset_1 != preset_2) {
        MIDI.sendProgramChange(preset_2, channel);
        merged_output_has_preset_1 = true;
      }
    }
    if (mode_patch_select) {
      if (four_synth_mode && note >= 64) {
        // 4-synth mode: Notes 64-127 -> synth 4
        preset_4 = note - 64;
        if (prev_preset_4 != preset_4) {
          MIDI.sendProgramChange(preset_4, MIDI_OUTPUT_CHANNEL_4);
          EEPROM.write(current_project + EEPROM_PRESET_4_OFFSET, preset_4);
          prev_preset_4 = preset_4;
        }
      } else {
        // 2-synth mode: all notes, or 4-synth mode: notes 0-63 -> synth 2
        preset_2 = note;
        if (prev_preset_2 != note) {
          MIDI.sendProgramChange(preset_2, MIDI_OUTPUT_CHANNEL_2);
          EEPROM.write(current_project + EEPROM_PRESET_2_OFFSET, preset_2);
          prev_preset_2 = preset_2;
        }
        MIDI.sendNoteOn(AUDITION_NOTE, velocity, MIDI_OUTPUT_CHANNEL_2);
      }
    } else {
      MIDI.sendNoteOn(note, velocity, channel);
    }
  }
}

// Note Off handler
void handleNoteOff(byte channel, byte note, byte velocity) {
  if (channel == MIDI_INPUT_CHANNEL_1) {
    if (mode_patch_select) {
      // Audition note only for MM2 (2-synth mode: all notes, 4-synth mode: notes 0-63)
      if (!four_synth_mode || note < 64) {
        MIDI.sendNoteOff(AUDITION_NOTE, velocity, MIDI_OUTPUT_CHANNEL_1);
      }
    } else {
      MIDI.sendNoteOff(note, velocity, channel);
    }
  } else if (channel == MIDI_INPUT_CHANNEL_2) {
    if (merge_mode) {
      channel = MERGE_MODE_OUTPUT_CHANNEL;
    }
    if (mode_patch_select) {
      // Audition note only for MM2 (2-synth mode: all notes, 4-synth mode: notes 0-63)
      if (!four_synth_mode || note < 64) {
        MIDI.sendNoteOff(AUDITION_NOTE, velocity, MIDI_OUTPUT_CHANNEL_2);
      }
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
