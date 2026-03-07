#include "face_renderer.h"
#include <math.h>

namespace {
constexpr int kRenderPadding = 8;

bool isDifferentFloat(float a, float b) {
  return fabsf(a - b) > 0.01f;
}
}

FaceRenderer::FaceRenderer(IDisplayPort& displayPort)
  : displayPort_(displayPort) {
}

bool FaceRenderer::render(const EyeModel& leftEye, const EyeModel& rightEye) {
  if (!hasLastFrame_) {
    displayPort_.clearFrame();
  } else if (!hasMeaningfulChange(leftEye, rightEye)) {
    return false;
  }

  Rect dirty = mergeRect(getEyeDirtyRect(hasLastFrame_ ? lastLeft_ : leftEye),
                         getEyeDirtyRect(hasLastFrame_ ? lastRight_ : rightEye));
  dirty = mergeRect(dirty, getEyeDirtyRect(leftEye));
  dirty = mergeRect(dirty, getEyeDirtyRect(rightEye));

  if (dirty.valid) {
    displayPort_.clearRect(dirty.x, dirty.y, dirty.w, dirty.h);
  }

  displayPort_.drawEye(leftEye.centerX,
                      leftEye.centerY,
                      leftEye.eyeRadius,
                      leftEye.openness,
                      leftEye.tiltDeg,
                      leftEye.squashY,
                      leftEye.stretchX,
                      leftEye.upperLid,
                      leftEye.lowerLid);

  displayPort_.drawEye(rightEye.centerX,
                      rightEye.centerY,
                      rightEye.eyeRadius,
                      rightEye.openness,
                      rightEye.tiltDeg,
                      rightEye.squashY,
                      rightEye.stretchX,
                      rightEye.upperLid,
                      rightEye.lowerLid);

  displayPort_.present();

  lastLeft_ = leftEye;
  lastRight_ = rightEye;
  hasLastFrame_ = true;
  return true;
}

FaceRenderer::Rect FaceRenderer::getEyeDirtyRect(const EyeModel& eye) const {
  Rect rect;

  const int halfW = static_cast<int>((eye.eyeRadius + 16) * eye.stretchX);
  const int halfH = static_cast<int>((eye.eyeRadius + 16) * eye.squashY);

  rect.x = eye.centerX - halfW - kRenderPadding;
  rect.y = eye.centerY - halfH - kRenderPadding;
  rect.w = (halfW * 2) + (kRenderPadding * 2);
  rect.h = (halfH * 2) + (kRenderPadding * 2);
  rect.valid = rect.w > 0 && rect.h > 0;
  return rect;
}

FaceRenderer::Rect FaceRenderer::mergeRect(const Rect& a, const Rect& b) const {
  if (!a.valid) return b;
  if (!b.valid) return a;

  Rect out;
  const int minX = (a.x < b.x) ? a.x : b.x;
  const int minY = (a.y < b.y) ? a.y : b.y;
  const int maxX = ((a.x + a.w) > (b.x + b.w)) ? (a.x + a.w) : (b.x + b.w);
  const int maxY = ((a.y + a.h) > (b.y + b.h)) ? (a.y + a.h) : (b.y + b.h);

  out.x = minX;
  out.y = minY;
  out.w = maxX - minX;
  out.h = maxY - minY;
  out.valid = out.w > 0 && out.h > 0;
  return out;
}

bool FaceRenderer::hasMeaningfulChange(const EyeModel& leftEye, const EyeModel& rightEye) const {
  if (isDifferentFloat(leftEye.openness, lastLeft_.openness)) return true;
  if (isDifferentFloat(rightEye.openness, lastRight_.openness)) return true;

  if (isDifferentFloat(leftEye.upperLid, lastLeft_.upperLid)) return true;
  if (isDifferentFloat(leftEye.lowerLid, lastLeft_.lowerLid)) return true;
  if (isDifferentFloat(rightEye.upperLid, lastRight_.upperLid)) return true;
  if (isDifferentFloat(rightEye.lowerLid, lastRight_.lowerLid)) return true;

  if (isDifferentFloat(leftEye.tiltDeg, lastLeft_.tiltDeg)) return true;
  if (isDifferentFloat(rightEye.tiltDeg, lastRight_.tiltDeg)) return true;
  if (isDifferentFloat(leftEye.squashY, lastLeft_.squashY)) return true;
  if (isDifferentFloat(rightEye.squashY, lastRight_.squashY)) return true;
  if (isDifferentFloat(leftEye.stretchX, lastLeft_.stretchX)) return true;
  if (isDifferentFloat(rightEye.stretchX, lastRight_.stretchX)) return true;

  return false;
}
