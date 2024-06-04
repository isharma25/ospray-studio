// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "AnimationWidget.h"
#include <imgui.h>
#include "sg/visitors/PrintNodes.h"
#include "sg/NodeType.h"
#include "sg/importer/Importer.h"

AnimationWidget::AnimationWidget(std::string name,
    const range1f &_timeRangeManual,
    NodePtr _frame,
    std::vector<NodePtr> &_timeseriesImporters,
    std::shared_ptr<GUIContext> _gui)
    : name(name),
      timeRangeManual(_timeRangeManual),
      frame(_frame),
      timeseriesImporters(_timeseriesImporters),
      gui(_gui)
{
  lastUpdated = std::chrono::system_clock::now();
  time = timeRangeManual.lower;
}

AnimationWidget::AnimationWidget(
    std::string name, std::shared_ptr<AnimationManager> animationManager)
    : name(name), animationManager(animationManager)
{
  lastUpdated = std::chrono::system_clock::now();
  if (!animationManager->getTime()) {
    time = animationManager->getTimeRange().lower;
  } else {
    time = animationManager->getTime();
    shutter = animationManager->getShutter();
  }

  animationManager->update(time, shutter);
}

AnimationWidget::~AnimationWidget()
{
  if (animationManager)
    animationManager.reset();
}

void AnimationWidget::update()
{
  auto now = std::chrono::system_clock::now();
  auto &timeRange = animationManager? animationManager->getTimeRange() : timeRangeManual;
  if (play) {
    time += std::chrono::duration<float>(now - lastUpdated).count() * speedup;
    if (time > timeRange.upper) {
      if (loop) {
        const float d = timeRange.size();
        time =
            d == 0.f ? timeRange.lower : timeRange.lower + std::fmod(time, d);
      } else {
        time = timeRange.lower;
        play = false;
      }
    }
  }
  if (animationManager) 
    animationManager->update(time, shutter);
  else {
    auto &world = frame->child("world");
      // add new importer
      if (worldCounter != (int)time) {
      // remove an existing importer
      for (auto &c : world.children())
        if (c.second->type() == sg::NodeType::IMPORTER)
          world.remove(c.first);
        auto currentImporter = timeseriesImporters[(int)time];
        world.add(currentImporter);
        gui->refreshScene(false, false); // disable importing for second time and do not reset camera
        worldCounter = (int)time;
    }
  }
  lastUpdated = now;
}

// update UI and process any UI events
void AnimationWidget::addUI()
{
  if (!showUI)
    return;
    ImGui::Begin(name.c_str(), &showUI);
    auto &timeRange = animationManager? animationManager->getTimeRange() : timeRangeManual;
    if (animationManager) {
    auto &animations = animationManager->getAnimations();

    if (animationManager->getAnimations().empty()) {
      ImGui::Text("== No animated objects in the scene ==");
      ImGui::End();
      return;
    }
    for (auto &a : animations)
      ImGui::Checkbox(a.name.c_str(), &a.active);
    }

    if (ImGui::Button(play ? "Pause" : "Play ")) {
      play = !play;
      lastUpdated = std::chrono::system_clock::now();
    }

    bool modified = play;

    ImGui::SameLine();
    if (ImGui::SliderFloat("time", &time, timeRange.lower, timeRange.upper))
      modified = true;

    ImGui::SameLine();
    ImGui::Checkbox("Loop", &loop);

    { // simulate log slider
      static float exp = 0.f;
      ImGui::SliderFloat("speedup: ", &exp, -3.f, 3.f);
      speedup = std::pow(10.0f, exp);
      ImGui::SameLine();
      ImGui::Text("%.*f", std::max(0, int(1.99f - exp)), speedup);
    }

    if (ImGui::SliderFloat("shutter", &shutter, 0.0f, timeRange.size()))
      modified = true;

    ImGui::Spacing();
    ImGui::End();

    if (modified)
      update();
}
