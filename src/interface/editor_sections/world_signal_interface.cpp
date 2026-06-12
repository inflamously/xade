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

#include "world_signal_interface.h"

#include "skin.h"

#include <algorithm>
#include <cmath>

namespace {
  const Colour kBackgroundColor(0xff000000);  // dark black
  const Colour kGridLineColor(0xff262626);    // subtle grey grid lines
  const Colour kOriginColor(0xffd0d0d0);      // bright dot marking world 0,0
  const Colour kBoundaryColor(0x33ffffff);    // faint ring around the world
  const Colour kDotColor(0xff7ec8ff);         // signal dot colour

  // Pulse tuning. The hovered dot pulses in place, its size and brightness
  // following a clean 0..1 swell:  value = 0.5 - 0.5 * cos(pulse_phase).
  const float kPulseSpeed = 0.006f;       // radians of phase per millisecond (~1s per pulse)
  const float kPulseGrowth = 0.6f;        // extra radius at the crest, as a fraction
  const float kHoverHitRadius = 9.0f;     // px; how close the cursor must be to hover a dot
  const int kScatterTestDotCount = 128;    // dots placed by the scatter test
}

WorldMapGrid::WorldMapGrid() : OpenGlImageComponent("world_map_grid"),
                               grid_size_(16.0f), pan_x_(0.0f), pan_y_(0.0f) {
  setInterceptsMouseClicks(false, false);
}

void WorldMapGrid::paintToImage(Graphics& g) {
  int width = getWidth();
  int height = getHeight();

  g.fillAll(kBackgroundColor);

  if (grid_size_ < 1.0f)
    return;

  // World (0,0) sits at the centre of the view, shifted by the pan offset.
  // Anchoring the grid to the origin keeps the dot on a line intersection.
  float origin_x = width * 0.5f + pan_x_;
  float origin_y = height * 0.5f + pan_y_;

  g.setColour(kGridLineColor);

  // Wrap the first line into [0, grid_size_) so the grid tiles seamlessly.
  float start_x = std::fmod(origin_x, grid_size_);
  if (start_x < 0.0f)
    start_x += grid_size_;
  float start_y = std::fmod(origin_y, grid_size_);
  if (start_y < 0.0f)
    start_y += grid_size_;

  for (float x = start_x; x < width; x += grid_size_)
    g.fillRect(roundToInt(x), 0, 1, height);

  for (float y = start_y; y < height; y += grid_size_)
    g.fillRect(0, roundToInt(y), width, 1);

  // Origin dot at world 0,0 (only when it is on screen).
  float radius = std::max(2.0f, grid_size_ * 0.2f);
  if (origin_x >= -radius && origin_x <= width + radius &&
      origin_y >= -radius && origin_y <= height + radius) {
    g.setColour(kOriginColor);
    g.fillEllipse(origin_x - radius, origin_y - radius, 2.0f * radius, 2.0f * radius);
  }
}

WorldMapDots::WorldMapDots() : OpenGlImageComponent("world_map_dots"),
                               boundary_radius_(0.0f), pan_x_(0.0f), pan_y_(0.0f),
                               hovered_index_(-1), pulse_phase_(0.0f), last_render_ms_(0.0) {
  setInterceptsMouseClicks(false, false);
}

WorldMapDots::~WorldMapDots() {
  stopTimer();
}

void WorldMapDots::scatterTestDots(int count) {
  // A fixed seed keeps the scatter stable across redraws (it is a test layout,
  // not a live signal feed yet).
  Random random(0x5310);
  setDots(world_signal::scatterDots(count, random));
}

Point<float> WorldMapDots::dotCenter(const WorldSignalDot& dot) const {
  // Matches WorldMapGrid: world origin at the centre of the view, plus pan.
  float origin_x = getWidth() * 0.5f + pan_x_;
  float origin_y = getHeight() * 0.5f + pan_y_;
  Point<float> offset = dot.offsetFromOrigin(boundary_radius_);
  return { origin_x + offset.x, origin_y + offset.y };
}

int WorldMapDots::dotAt(Point<float> position) const {
  int best = -1;
  float best_distance = 0.0f;
  for (int i = 0; i < static_cast<int>(dots_.size()); ++i) {
    // The dots are tiny, so allow a small slop around them for easy hovering.
    float hit_radius = jmax(dots_[i].base_radius, kHoverHitRadius);
    float distance = dotCenter(dots_[i]).getDistanceFrom(position);
    if (distance <= hit_radius && (best < 0 || distance < best_distance)) {
      best = i;
      best_distance = distance;
    }
  }
  return best;
}

void WorldMapDots::updateHover(Point<float> position) {
  int index = dotAt(position);
  if (index == hovered_index_)
    return;

  hovered_index_ = index;
  if (hovered_index_ >= 0) {
    // Restart the pulse from rest each time a new dot is picked up.
    pulse_phase_ = 0.0f;
    last_render_ms_ = Time::currentTimeMillis();
    if (!isTimerRunning())
      startTimerHz(60);
  }
  else {
    stopTimer();
    redrawImage(true);  // settle the dot back to its resting size
  }
}

void WorldMapDots::clearHover() {
  if (hovered_index_ < 0)
    return;

  hovered_index_ = -1;
  stopTimer();
  redrawImage(true);  // settle the dot back to its resting size
}

void WorldMapDots::timerCallback() {
  double now = static_cast<double>(Time::currentTimeMillis());
  double delta_ms = now - last_render_ms_;
  last_render_ms_ = now;

  // Guard against a large first step or a clock hiccup.
  delta_ms = jlimit(0.0, 100.0, delta_ms);
  pulse_phase_ += static_cast<float>(delta_ms) * kPulseSpeed;
  redrawImage(true);
}

void WorldMapDots::paintToImage(Graphics& g) {
  // World origin: centre of the view, shifted by the shared pan offset. Matches
  // WorldMapGrid so the dots line up with the grid's origin dot.
  float origin_x = getWidth() * 0.5f + pan_x_;
  float origin_y = getHeight() * 0.5f + pan_y_;

  // Faint ring marking the world boundary the dots are scattered within.
  if (boundary_radius_ > 0.0f) {
    g.setColour(kBoundaryColor);
    g.drawEllipse(origin_x - boundary_radius_, origin_y - boundary_radius_,
                  2.0f * boundary_radius_, 2.0f * boundary_radius_, 1.0f);
  }

  for (int i = 0; i < static_cast<int>(dots_.size()); ++i) {
    const WorldSignalDot& dot = dots_[i];
    Point<float> center = dotCenter(dot);

    // Only the hovered dot pulses; the rest sit at rest (pulse == 0). The swell
    // runs 0..1 and back so the dot grows and brightens then settles.
    float pulse = (i == hovered_index_) ? (0.5f - 0.5f * std::cos(pulse_phase_)) : 0.0f;

    float radius = dot.base_radius * (1.0f + kPulseGrowth * pulse);
    float brightness = jlimit(0.0f, 1.0f, 0.55f + 0.45f * pulse);

    g.setColour(kDotColor.withMultipliedAlpha(brightness));
    g.fillEllipse(center.x - radius, center.y - radius, 2.0f * radius, 2.0f * radius);
  }
}

WorldSignalInterface::WorldSignalInterface() : SynthSection("world_signal"),
                                     dragging_(false), pan_start_x_(0.0f), pan_start_y_(0.0f) {
  setOpaque(false);

  grid_ = std::make_unique<WorldMapGrid>();
  addOpenGlComponent(grid_.get());

  // Dots sit on top of the grid. Scatter a test set so the world is populated.
  dots_ = std::make_unique<WorldMapDots>();
  dots_->scatterTestDots(kScatterTestDotCount);
  addOpenGlComponent(dots_.get());
}

WorldSignalInterface::~WorldSignalInterface() { }

void WorldSignalInterface::paintBackground(Graphics& g) {
  g.fillAll(kBackgroundColor);
  paintChildrenBackgrounds(g);
}

void WorldSignalInterface::resized() {
  grid_->setGridSize(16.0f * size_ratio_);
  grid_->setBounds(getLocalBounds());

  dots_->setBounds(getLocalBounds());
  // Boundary fills most of the view: 40% of its shorter side.
  dots_->setBoundaryRadius(0.4f * jmin(getWidth(), getHeight()));

  SynthSection::resized();
}

void WorldSignalInterface::setPan(float x, float y) {
  grid_->setPan(x, y);
  grid_->redrawImage(true);
  dots_->setPan(x, y);
  dots_->redrawImage(true);
}

void WorldSignalInterface::resetWorldView() {
    dragging_ = false;
    setPan(0.0f, 0.0f);
}

void WorldSignalInterface::mouseDown(const MouseEvent& e) {
  dragging_ = true;
  drag_start_ = e.position;
  pan_start_x_ = grid_->getPanX();
  pan_start_y_ = grid_->getPanY();
}

void WorldSignalInterface::mouseDrag(const MouseEvent& e) {
  if (!dragging_)
    return;

  setPan(pan_start_x_ + (e.position.x - drag_start_.x),
         pan_start_y_ + (e.position.y - drag_start_.y));
}

void WorldSignalInterface::mouseDoubleClick(const MouseEvent& event)
{
    if (event.mods.isCtrlDown()) {
        // Ctrl+click recenters the view on the origin.
        resetWorldView();
    }
}

void WorldSignalInterface::mouseMove(const MouseEvent& e) {
  dots_->updateHover(e.position);
}

void WorldSignalInterface::mouseExit(const MouseEvent& e) {
  dots_->clearHover();
}
