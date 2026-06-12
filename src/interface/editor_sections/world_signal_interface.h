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
#include "open_gl_image_component.h"

// The pannable grid layer of the world map. Drawn into an OpenGL image so it
// can be cheaply re-rendered when panned (no full-window background repaint).
class WorldMapGrid : public OpenGlImageComponent {
  public:
    WorldMapGrid();

    void paintToImage(Graphics& g) override;

    // Grid spacing in pixels (defaults to 16).
    void setGridSize(float size) { grid_size_ = size; }
    float getGridSize() const { return grid_size_; }

    // Pan offset, in pixels. Used in a later step to drag the grid around.
    void setPan(float x, float y) { pan_x_ = x; pan_y_ = y; }
    float getPanX() const { return pan_x_; }
    float getPanY() const { return pan_y_; }

  private:
    float grid_size_;
    float pan_x_;
    float pan_y_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WorldMapGrid)
};

// XO Lite style "world map" view.
//
// Built up in steps:
//   1. Dark container that spans the full page.            done
//   2. A grid of lines (~16x16px).                         done
//   3. An origin dot at grid 0,0 (centre of the view).     done
//   4. Drag/pan the grid; ctrl+click recenters.            <-- current
class WorldSignalInterface : public SynthSection {
  public:
    WorldSignalInterface();
    ~WorldSignalInterface();

    void paintBackground(Graphics& g) override;
    void resized() override;

    void resetWorldView();

    // Drag to pan the grid; ctrl+click recenters on the origin.
    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseDoubleClick(const MouseEvent& event) override;

  private:
    std::unique_ptr<WorldMapGrid> grid_;

    bool dragging_;
    Point<float> drag_start_;
    float pan_start_x_;
    float pan_start_y_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WorldSignalInterface)
};
