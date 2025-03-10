/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "../../../inc/MarlinConfig.h"

#if ENABLED(MIXING_EXTRUDER)

#include "../../gcode.h"
#include "../../../feature/mixing.h"

/**
 * M163: Set a single mix factor for a mixing extruder
 *       This is called "weight" by some systems.
 *       Must be followed by M164 to normalize and commit them.
 *
 *   S[index]   The channel index to set
 *   P[float]   The mix value
 */
void GcodeSuite::M163() {
  const int mix_index = parser.intval('S');
  if (mix_index < MIXING_STEPPERS)
    mixer.set_collector(mix_index, parser.floatval('P'));
}

/**
 * M164: Normalize and commit the mix.
 *
 *   S[index]   The virtual tool to store
 *              If 'S' is omitted update the active virtual tool.
 */
void GcodeSuite::M164() {
  #if MIXING_VIRTUAL_TOOLS > 1
    const int tool_index = parser.intval('S', -1);
  #else
    constexpr int tool_index = 0;
  #endif
  if (tool_index >= 0) {
    if (tool_index < MIXING_VIRTUAL_TOOLS)
      mixer.normalize(tool_index);
  }
  else
    mixer.normalize();
}

#if ENABLED(DIRECT_MIXING_IN_G1)

  /**
   * M165: Set multiple mix factors for a mixing extruder.
   *       Omitted factors will be set to 0.
   *       The mix is normalized and stored in the current virtual tool.
   *
   *   A[factor] Mix factor for extruder stepper 1
   *   B[factor] Mix factor for extruder stepper 2
   *   C[factor] Mix factor for extruder stepper 3
   *   D[factor] Mix factor for extruder stepper 4
   *   H[factor] Mix factor for extruder stepper 5
   *   I[factor] Mix factor for extruder stepper 6
   *   J[factor] Mix factor for extruder stepper 7
   *   K[factor] Mix factor for extruder stepper 8
   */
  void GcodeSuite::M165() {
    // Get mixing parameters from the G-Code
    // The total "must" be 1.0 (but it will be normalized)
    // If no mix factors are given, the old mix is preserved
    const char mixing_codes[] = { LIST_N(MIXING_STEPPERS, 'A', 'B', 'C', 'D', 'H', 'I', 'J', 'K') };
    uint8_t mix_bits = 0;
    MIXER_STEPPER_LOOP(i) {
      if (parser.seenval(mixing_codes[i])) {
        SBI(mix_bits, i);
        mixer.set_collector(i, parser.value_float());
      }
    }
    // If any mixing factors were included, clear the rest
    // If none were included, preserve the last mix
    if (mix_bits) {
      MIXER_STEPPER_LOOP(i)
        if (!TEST(mix_bits, i)) mixer.set_collector(i, 0.0f);
      mixer.normalize();
    }
    // Report the latest V-tool mixes
    if (parser.seen_test('R')) M165_report();
  }

  void GcodeSuite::M165_report(const bool forReplay/*=true*/) {
    gcode.report_heading_etc(forReplay, F(STR_CURRENT_VTOOLS));
    VTOOLS_LOOP(i) {
      // Get mixes from all tools as a percentage
      mixer.refresh_collector(100.0, i);
      SERIAL_ECHO(F("  V"), i, ":");  // Avoid using 'Tn:' substring so output is fully visible on
                                      // BTT Touchscreen terminal.
      SERIAL_ECHOLNPGM(LIST_N(DOUBLE(MIXING_STEPPERS),
         "  A", p_float_t(mixer.collector[0], 1),
          " B", p_float_t(mixer.collector[1], 1),
          " C", p_float_t(mixer.collector[2], 1),
          " D", p_float_t(mixer.collector[3], 1),
          " H", p_float_t(mixer.collector[4], 1),
          " I", p_float_t(mixer.collector[5], 1),
          " J", p_float_t(mixer.collector[6], 1),
          " K", p_float_t(mixer.collector[7], 1)));
    }
  }

#endif // DIRECT_MIXING_IN_G1

#endif // MIXING_EXTRUDER
