// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// sg
#include "../../Data.h"
#include "Volume.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE StructuredVolume : public Volume
  {
    StructuredVolume();
    virtual ~StructuredVolume() override = default;
  };

  // StructuredVolume definitions /////////////////////////////////////////////

  StructuredVolume::StructuredVolume() : Volume("structuredRegular") {

      createChild("voxelType", "OSPDataType", OSP_SHORT);
      createChild("dimensions", "vec3i", vec3i(512, 512, 64));
      createChild("gridOrigin", "vec3f", vec3f(-125.f, -125.f, -64.f));
      // createChild("filter", "OSPVolumeFilter", OSP_VOLUME_FILTER_CUBIC);
      // createChild("gridSpacing", "vec3f", vec3f(2.f / 100));

  }

  OSP_REGISTER_SG_NODE_NAME(StructuredVolume, structuredRegular);

  }  // namespace sg
} // namespace ospray
