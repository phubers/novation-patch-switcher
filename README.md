# Novation Circuit Tracks MIDI Patch Switcher

An Arduino-based MIDI router that automatically switches presets on external synthesizers when changing projects on the Novation Circuit Tracks.

## Problem

The Circuit Tracks can sequence external synths via MIDI, but it doesn't send program changes to external gear when switching projects. This means you have to manually change patches on your synths every time you load a different project.

## Solution

This Arduino sketch sits between the Circuit Tracks and your external synths. It:

1. **Stores preset mappings** for each of the 64 CT projects in EEPROM
2. **Automatically sends program changes** to your synths when you switch projects
3. **Provides a patch selection mode** to program the mappings using the CT's pads

## Features

- **2-synth mode** (default): 128 patches per synth on MIDI channels 3 & 4
- **4-synth mode**: 64 patches each for 4 synths on MIDI channels 3-6
- **Persistent storage**: Mappings survive power cycles
- **No extra hardware required**: Works with just an Arduino and MIDI breakout
- **Optional button/LED**: Toggle between 2-synth and 4-synth modes

## Hardware

- Arduino (Uno, Nano, or similar)
- MIDI breakout board (directly to standard DIN cable or directly to TRS cable with type A or B jack)
- Optional: momentary button (pin 2) for mode switching (uses built-in LED)

## Wiring

| Arduino Pin | Connection |
|-------------|------------|
| RX | MIDI In from Circuit Tracks |
| TX | MIDI Out to synths |
| Pin 2 | Mode button (optional, active low) |
| Pin 13 | Built-in LED (4-synth mode indicator) |

## Usage

### Normal Operation

1. Connect CT MIDI Out to Arduino MIDI In
2. Connect Arduino MIDI Out to your synth(s)
3. Configure your synths to receive on channels 3-6
4. Switch projects on the CT - your synths follow automatically

### Programming Presets

Two methods available (set `USE_BUTTON_FOR_PATCH_SELECT` in code):

**Button mode** (`USE_BUTTON_FOR_PATCH_SELECT = true`):
1. Press the button to enter patch select mode (LED turns on)
2. Select a MIDI track on the CT
3. Play pads to select presets
4. Press the button again to exit (LED turns off)

**Filter knob mode** (default, `USE_BUTTON_FOR_PATCH_SELECT = false`):
1. Turn the **master filter knob to minimum** (fully left) on the CT
2. Select a MIDI track on the CT
3. Play pads to select presets
4. Turn the filter knob back up to exit

In both modes:
- **2-synth mode**: Notes 0-127 select patches for synth 1 or 2
- **4-synth mode**: Notes 0-63 for synths 1/2, notes 64-127 for synths 3/4

Use the **Note Expand** button to access all 128 notes across 4 pages (32 notes per page).

### Switching Between 2-Synth and 4-Synth Modes

**With button**: Hold the button while powering on/resetting the Arduino. The LED blinks to confirm:
- 2 blinks = 2-synth mode (128 patches per synth)
- 4 blinks = 4-synth mode (64 patches per synth)

**Without button**: Change the `four_synth_mode` default in code and re-upload.

## MIDI Channel Mapping

| Mode | CT Track | Note Range | Output Channel | Synth |
|------|----------|------------|----------------|-------|
| 2-synth | MIDI 1 | 0-127 | 3 | Synth 1 |
| 2-synth | MIDI 2 | 0-127 | 4 | Synth 2 |
| 4-synth | MIDI 1 | 0-63 | 3 | Synth 1 |
| 4-synth | MIDI 1 | 64-127 | 5 | Synth 3 |
| 4-synth | MIDI 2 | 0-63 | 4 | Synth 2 |
| 4-synth | MIDI 2 | 64-127 | 6 | Synth 4 |

## Dependencies

- [Arduino MIDI Library](https://github.com/FortySevenEffects/arduino_midi_library)

Install via Arduino Library Manager: Sketch > Include Library > Manage Libraries > search "MIDI Library"

## Credits

Original concept and initial code by [Provokator](https://www.youtube.com/@Provokator) (Reddit: u/Capable-Laugh-9312).

Bugfixes and enhancements by Patrick Hubers.

## License

MIT License - See [LICENSE](LICENSE) for details.
