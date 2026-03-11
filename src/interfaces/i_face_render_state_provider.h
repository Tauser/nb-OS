#pragma once

#include "../models/face_render_state.h"

class IFaceRenderStateProvider {
public:
  virtual ~IFaceRenderStateProvider() = default;
  virtual const FaceRenderState& getFaceRenderState() const = 0;
};
