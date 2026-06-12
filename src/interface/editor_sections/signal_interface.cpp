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

#include "signal_interface.h"

#include "skin.h"

#include <algorithm>
#include <cmath>

namespace {
  const Colour kBackgroundColor(0xff000000);  // dark black
  const Colour kGridLineColor(0xff262626);    // subtle grey grid lines
  const Colour kOriginColor(0xffd0d0d0);      // bright dot marking world 0,0
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

SignalInterface::SignalInterface() : SynthSection("signal"),
                                     dragging_(false), pan_start_x_(0.0f), pan_start_y_(0.0f) {
  setOpaque(false);

  grid_ = std::make_unique<WorldMapGrid>();
  addOpenGlComponent(grid_.get());
}

SignalInterface::~SignalInterface() { }

void SignalInterface::paintBackground(Graphics& g) {
  g.fillAll(kBackgroundColor);
  paintChildrenBackgrounds(g);
}

void SignalInterface::resized() {
  grid_->setGridSize(16.0f * size_ratio_);
  grid_->setBounds(getLocalBounds());
  SynthSection::resized();
}

void SignalInterface::resetWorldView() {
    dragging_ = false;
    grid_->setPan(0.0f, 0.0f);
    grid_->redrawImage(true);
}

void SignalInterface::mouseDown(const MouseEvent& e) {
  dragging_ = true;
  drag_start_ = e.position;
  pan_start_x_ = grid_->getPanX();
  pan_start_y_ = grid_->getPanY();
}

void SignalInterface::mouseDrag(const MouseEvent& e) {
  if (!dragging_)
    return;

  grid_->setPan(pan_start_x_ + (e.position.x - drag_start_.x),
                pan_start_y_ + (e.position.y - drag_start_.y));
  grid_->redrawImage(true);
}

void SignalInterface::mouseDoubleClick(const MouseEvent& event)
{
    if (event.mods.isCtrlDown()) {
        // Ctrl+click recenters the view on the origin.
        resetWorldView();
    }
}
