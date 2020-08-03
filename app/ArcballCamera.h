// ======================================================================== //
// Copyright 2017-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include <vector>

#include "rkcommon/math/AffineSpace.h"

using namespace rkcommon::math;

class ArcballCamera;

#if 0
//
//helpers to cerealize things in rkcommon without modifying rkcommon directly
//todo: this runs and serializes but camera gets garbage, probably template related
namespace rkcommon { namespace math {
template<class Archive> void serialize(Archive & archive, vec3f &m) { archive( m.x,m.y,m.z ); }
template<class Archive> void serialize(Archive & archive, quaternionf &m) { archive( m.r,m.i,m.j,m.k ); }
template<class Archive> void serialize(Archive & archive, LinearSpace3f &m) { archive( m.vx, m.vy ); }
template<class Archive> void serialize(Archive & archive, AffineSpace3f &m) { archive( m.l, m.p ); }
} }
//
#endif
class CameraState
{
 public:
  CameraState() = default;
  CameraState(const AffineSpace3f &centerTrans,
              const AffineSpace3f &trans,
              const quaternionf &rot)
      : centerTranslation(centerTrans), translation(trans), rotation(rot)
  {
  }

  CameraState slerp(const CameraState &to, float frac) const
  {
    CameraState cs;

    cs.centerTranslation = lerp(frac, centerTranslation, to.centerTranslation);
    cs.translation       = lerp(frac, translation, to.translation);
    if (rotation != to.rotation)
      cs.rotation = slerp(rotation, to.rotation, frac);
    else
      cs.rotation = rotation;

    return cs;
  }

  friend std::string to_string(const CameraState &state)
  {
    // returns the world position
    const AffineSpace3f rot = LinearSpace3f(state.rotation);
    const AffineSpace3f camera = state.translation * rot * state.centerTranslation;
    vec3f pos = xfmPoint(rcp(camera), vec3f(0, 0, 1));
    std::stringstream ss;
    ss << pos;
    return ss.str();
  }

  // participate in Cerealization
  template<class Archive>
  void serialize(Archive & archive)
  {
    archive( centerTranslation, translation, rotation );
  }

 protected:
  friend ArcballCamera;

  float dot(const quaternionf &q0, const quaternionf &q1) const
  {
    return q0.r * q1.r + q0.i * q1.i + q0.j * q1.j + q0.k * q1.k;
  }

  quaternionf slerp(const quaternionf &q0, const quaternionf &q1, float t) const
  {
    quaternionf qt0 = q0, qt1 = q1;
    float d = dot(qt0, qt1);
    if (d < 0.f) {
      // prevent "long way around"
      qt0 = -qt0;
      d   = -d;
    } else if (d > 0.9995) {
      // angles too small
      quaternionf l = lerp(t, q0, q1);
      return normalize(l);
    }

    float theta  = std::acos(d);
    float thetat = theta * t;

    float s0 = std::cos(thetat) - d * std::sin(thetat) / std::sin(theta);
    float s1 = std::sin(thetat) / std::sin(theta);

    return s0 * qt0 + s1 * qt1;
  }

  AffineSpace3f centerTranslation, translation;
  quaternionf rotation;
};

class ArcballCamera
{
 public:
  ArcballCamera(const box3f &worldBounds, const vec2i &windowSize);

  // All mouse positions passed should be in [-1, 1] normalized screen coords
  void rotate(const vec2f &from, const vec2f &to);
  void constrainedRotate(const vec2f &from, const vec2f &to, int axis /* 0 = x, 1 = y, 2 = z, otherwise none*/);
  void zoom(float amount);
  void pan(const vec2f &delta);

  vec3f eyePos() const;
  vec3f center() const;
  void setCenter(const vec3f &newCenter);

  vec3f lookDir() const;
  vec3f upDir() const;

  void setRotation(quaternionf);
  void setState(const CameraState &state);
  CameraState getState() const;

  void updateWindowSize(const vec2i &windowSize);
  void setNewWorldBounds(const box3f &worldBounds);

 protected:
  void updateCamera();

  // Project the point in [-1, 1] screen space onto the arcball sphere
  quaternionf screenToArcball(const vec2f &p);

  float zoomSpeed;
  vec2f invWindowSize;
  AffineSpace3f centerTranslation, translation, invCamera;
  quaternionf rotation;
  
};

// Catmull-Rom quaternion interpolation
// requires "endpoint" states `prefix` and `suffix`
// returns an interpolated state at `frac` [0, 1] between `from` and `to`
CameraState catmullRom(const CameraState &prefix,
                       const CameraState &from,
                       const CameraState &to,
                       const CameraState &suffix,
                       float frac);

// build an interpolated path from a vector of CameraStates
// using Catmull-Rom quaternion interpolation
// for n >= 2 anchors, creates (n - 1) * (1 / stepSize) CameraStates
std::vector<CameraState> buildPath(const std::vector<CameraState> &anchors,
                                   const float stepSize = 0.1);
