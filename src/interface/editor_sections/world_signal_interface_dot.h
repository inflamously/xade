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

#include <cmath>
#include <vector>

// A single point on the signal world map.
//
// A dot is stored in polar form relative to the world origin (0,0):
//   - angle:           which direction it sits from the centre, in radians.
//   - signal_strength: how "different" this signal is from the others, 0..1.
//                      0 = identical (sits on the origin), 1 = most different
//                      (pushed out to the boundary).
//
// Keeping the dot purely as data (no OpenGL, no JUCE component) means the
// placement maths can be unit tested on its own. The interface layer turns the
// dot into pixels with offsetFromOrigin() below.
struct WorldSignalDot {
  float angle = 0.0f;            // radians, direction from the world origin
  float signal_strength = 0.0f;  // 0..1, the difference from other audio elements
  float base_radius = 5.0f;      // dot size in px before any DPI scaling

  // How far from the origin (in px) this dot sits, clamped to the boundary so
  // a dot can never escape the world. Stronger signal == farther out.
  float distanceFromOrigin(float boundary_radius) const {
    float distance = signal_strength * boundary_radius;
    return juce::jlimit(0.0f, boundary_radius, distance);
  }

  // Pixel offset from the world origin (before panning or pulse animation).
  juce::Point<float> offsetFromOrigin(float boundary_radius) const {
    float distance = distanceFromOrigin(boundary_radius);
    return { std::cos(angle) * distance, std::sin(angle) * distance };
  }

  // Position in the normalized unit disk (boundary radius == 1). Used by the
  // scatter to keep dots from overlapping without needing the pixel boundary,
  // so collisions stay stable as the view (and boundary) is resized.
  juce::Point<float> unitPosition() const {
    return { std::cos(angle) * signal_strength, std::sin(angle) * signal_strength };
  }
};

namespace world_signal {
  // Scatters `count` dots randomly around the origin. Angles cover the full
  // circle and strengths span 0..1, so the dots spread across the whole world
  // up to the boundary. Pass a seeded juce::Random for reproducible results
  // (used by the test).
  //
  // Collision: a candidate is rejected if it lands closer than
  // `min_separation` (in normalized units, i.e. a fraction of the boundary
  // radius) to an already-placed dot, so dots never sit on top of each other.
  // Placement uses rejection sampling with a capped number of attempts per dot
  // so a too-dense request can never loop forever -- a dot that can't find a
  // free spot within the cap is simply dropped (the returned count may be less
  // than `count`).
  inline std::vector<WorldSignalDot> scatterDots(int count, juce::Random& random,
                                                 float min_separation = 0.16f) {
    std::vector<WorldSignalDot> dots;
    dots.reserve(juce::jmax(0, count));

    const float min_separation_sq = min_separation * min_separation;
    const int max_attempts = 16;  // per dot, before giving up on a free spot

    for (int i = 0; i < count; ++i) {
      for (int attempt = 0; attempt < max_attempts; ++attempt) {
        WorldSignalDot dot;
        dot.angle = random.nextFloat() * juce::MathConstants<float>::twoPi;
        dot.signal_strength = random.nextFloat();
        juce::Point<float> position = dot.unitPosition();

        bool overlaps = false;
        for (const auto& placedDot : dots) {
          if (position.getDistanceSquaredFrom(placedDot.unitPosition()) < min_separation_sq || position.getDistanceSquaredFromOrigin() < min_separation_sq) {
            overlaps = true;
            break;
          }
        }

        if (!overlaps) {
          dots.push_back(dot);
          break;
        }
      }
    }

    return dots;
  }
}
