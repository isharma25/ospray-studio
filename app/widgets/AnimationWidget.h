// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include <string>
#include "../AnimationManager.h"
#include "sg/Frame.h"
#include "app/GUIContext.h"
using NodePtr = ospray::sg::NodePtr;

class AnimationWidget
{
 public:
  AnimationWidget(std::string name,
      const range1f &_timeRangeManual,
      NodePtr frame,
      std::vector<NodePtr> &timeseriesImporters,
      std::shared_ptr<GUIContext> gui);
  AnimationWidget(
      std::string name, std::shared_ptr<AnimationManager> animationManager);
  ~AnimationWidget();
  void addUI();
  void update();

  float getShutter()
  {
    return shutter;
  }

  // Initialize widget starting time based on animations it controls
  void init()
  {
    animationManager->init();
    if (!animationManager->getTime())
      time = animationManager->getTimeRange().lower;
    else {
      time = animationManager->getTime();
      shutter = animationManager->getShutter();
    }
  }

  void setShowUI()
  {
    showUI = true;
  }

  void togglePlay()
  {
    play = !play;
    lastUpdated = std::chrono::system_clock::now();
  }

  bool isPlaying()
  {
    return play;
  }

 private:
  bool showUI{false};
  bool play{false};
  bool loop{true};
  float speedup{1.0f};
  std::string name{"Animation Widget"};
  std::shared_ptr<AnimationManager> animationManager;
  std::chrono::time_point<std::chrono::system_clock> lastUpdated;
  float time{0.0f};
  float shutter{0.0f};
  range1f timeRangeManual;
  NodePtr frame;
  std::vector<NodePtr> timeseriesImporters;
  bool activeWorld{false};
  std::shared_ptr<GUIContext> gui;
  int worldCounter{-1};
};
