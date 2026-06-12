/* Copyright 2013-2019 Matt Tytel
 *
 * vital is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * vital is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vital.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "JuceHeader.h"
#include "synth_section.h"

// XO Lite style "world map" view.
//
// Built up in steps:
//   1. Dark container that spans the full page.            <-- current
//   2. A grid of lines (~16x16px).                         (todo)
//   3. An origin dot at grid 0,0.                          (todo)
//   4. Drag/pan the grid around.                           (todo)
class SignalInterface : public SynthSection {
  public:
    SignalInterface();
    ~SignalInterface();

    void paintBackground(Graphics& g) override;
    void resized() override;

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SignalInterface)
};
