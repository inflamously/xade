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

#include "world_signal_interface_test.h"
#include "world_signal_interface_dot.h"

#include <cmath>

void WorldSignalInterfaceTest::runTest() {
  const float kBoundaryRadius = 200.0f;
  const float kEpsilon = 1.0e-3f;

  beginTest("Scatter stays inside the boundary");
  {
    Random random(0x1234);
    std::vector<WorldSignalDot> dots = world_signal::scatterDots(500, random);
    expectEquals((int)dots.size(), 500);

    for (const WorldSignalDot& dot : dots) {
      // Strength is a 0..1 difference value.
      expect(dot.signal_strength >= 0.0f && dot.signal_strength <= 1.0f);

      // Every dot must land within the world boundary (plus float slack).
      Point<float> offset = dot.offsetFromOrigin(kBoundaryRadius);
      float length = std::sqrt(offset.x * offset.x + offset.y * offset.y);
      expect(length <= kBoundaryRadius + kEpsilon);

      // The offset length must match the reported distance.
      expectWithinAbsoluteError(length, dot.distanceFromOrigin(kBoundaryRadius), kEpsilon);
    }
  }

  beginTest("Distance scales with signal strength and clamps");
  {
    WorldSignalDot centre;
    centre.signal_strength = 0.0f;
    expectWithinAbsoluteError(centre.distanceFromOrigin(kBoundaryRadius), 0.0f, kEpsilon);

    WorldSignalDot edge;
    edge.signal_strength = 1.0f;
    expectWithinAbsoluteError(edge.distanceFromOrigin(kBoundaryRadius), kBoundaryRadius, kEpsilon);

    WorldSignalDot mid;
    mid.signal_strength = 0.5f;
    expectWithinAbsoluteError(mid.distanceFromOrigin(kBoundaryRadius), kBoundaryRadius * 0.5f, kEpsilon);

    // Out-of-range strength is clamped to the boundary, never beyond it.
    WorldSignalDot over;
    over.signal_strength = 5.0f;
    expectWithinAbsoluteError(over.distanceFromOrigin(kBoundaryRadius), kBoundaryRadius, kEpsilon);
  }

  beginTest("Scatter is reproducible for a fixed seed");
  {
    Random a(0x99);
    Random b(0x99);
    std::vector<WorldSignalDot> first = world_signal::scatterDots(32, a);
    std::vector<WorldSignalDot> second = world_signal::scatterDots(32, b);

    expectEquals((int)first.size(), (int)second.size());
    for (size_t i = 0; i < first.size(); ++i) {
      expectWithinAbsoluteError(first[i].angle, second[i].angle, kEpsilon);
      expectWithinAbsoluteError(first[i].signal_strength, second[i].signal_strength, kEpsilon);
    }
  }
}

static WorldSignalInterfaceTest world_signal_interface_test;
