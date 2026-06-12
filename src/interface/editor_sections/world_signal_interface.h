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
#include "world_signal_interface_dot.h"

#include <vector>

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

// The dot layer of the world map, drawn on top of the grid.
//
// Holds the signal dots and paints them around the origin, inside a circular
// boundary. When the mouse hovers directly over a dot, that single dot pulses
// (expands and brightens); the layer redraws itself every frame while a dot is
// hovered and settles back to rest once the mouse leaves it.
class WorldMapDots : public OpenGlImageComponent, public Timer {
  public:
    WorldMapDots();
    ~WorldMapDots() override;

    void paintToImage(Graphics& g) override;

    // While hovering, a timer advances the pulse and redraws each frame.
    void timerCallback() override;

    void setDots(std::vector<WorldSignalDot> dots) { dots_ = std::move(dots); redrawImage(true); }
    const std::vector<WorldSignalDot>& getDots() const { return dots_; }

    // Fills the world with `count` scattered dots (the "scatter test").
    void scatterTestDots(int count);

    // Radius of the circular world boundary, in pixels.
    void setBoundaryRadius(float radius) { boundary_radius_ = radius; }
    float getBoundaryRadius() const { return boundary_radius_; }

    // Matches the grid so the dots pan and centre together with it.
    void setPan(float x, float y) { pan_x_ = x; pan_y_ = y; }

    // Hover handling. Pass a mouse position (in this component's coordinates)
    // to pick out the dot under the cursor; that dot pulses. clearHover() drops
    // the highlight when the mouse leaves the view. Enabling a hover starts the
    // redraw timer so the pulse animates on the message thread.
    void updateHover(Point<float> position);
    void clearHover();
    int getHoveredIndex() const { return hovered_index_; }

  private:
    // Resting pixel centre of a dot (origin + offset, no pulse). Used both for
    // painting and for hit-testing the mouse against the dots.
    Point<float> dotCenter(const WorldSignalDot& dot) const;
    // Index of the dot under `position`, or -1 if none is hit.
    int dotAt(Point<float> position) const;

    std::vector<WorldSignalDot> dots_;
    float boundary_radius_;
    float pan_x_;
    float pan_y_;

    int hovered_index_;        // dot under the mouse, or -1 for none
    float pulse_phase_;        // accumulated pulse time (radians)
    double last_render_ms_;    // timestamp of the previous animated frame

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WorldMapDots)
};

// XO Lite style "world map" view.
//
// Built up in steps:
//   1. Dark container that spans the full page.            done
//   2. A grid of lines (~16x16px).                         done
//   3. An origin dot at grid 0,0 (centre of the view).     done
//   4. Drag/pan the grid; ctrl+click recenters.            done
//   5. Signal dots scattered in a boundary; pulse on hover. <-- current
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

    // The dot under the cursor pulses; track the mouse to pick it out.
    void mouseMove(const MouseEvent& e) override;
    void mouseExit(const MouseEvent& e) override;

  private:
    // Keeps the dot layer's pan in sync with the grid's.
    void setPan(float x, float y);

    std::unique_ptr<WorldMapGrid> grid_;
    std::unique_ptr<WorldMapDots> dots_;

    bool dragging_;
    Point<float> drag_start_;
    float pan_start_x_;
    float pan_start_y_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WorldSignalInterface)
};
