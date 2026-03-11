#include "face_renderer.h"

#include <Arduino.h>
#include <math.h>

namespace {
constexpr int kRenderPadding = 10;
constexpr float kGeomEpsilon = 0.006f;
constexpr float kLidEpsilon = 0.004f;
constexpr float kTiltEpsilon = 0.08f;
constexpr unsigned long kIdleRefreshMs = 240;

bool isDifferent(float a, float b, float eps) {
  return fabsf(a - b) > eps;
}

bool shapeDifferent(const ResolvedEyeShape& a, const ResolvedEyeShape& b) {
  if (a.mode != b.mode) return true;

  if (isDifferent(a.topWidthScale, b.topWidthScale, kGeomEpsilon)) return true;
  if (isDifferent(a.bottomWidthScale, b.bottomWidthScale, kGeomEpsilon)) return true;
  if (isDifferent(a.leftCantDeg, b.leftCantDeg, kTiltEpsilon)) return true;
  if (isDifferent(a.rightCantDeg, b.rightCantDeg, kTiltEpsilon)) return true;

  if (isDifferent(a.roundTopLeft, b.roundTopLeft, kGeomEpsilon)) return true;
  if (isDifferent(a.roundTopRight, b.roundTopRight, kGeomEpsilon)) return true;
  if (isDifferent(a.roundBottomLeft, b.roundBottomLeft, kGeomEpsilon)) return true;
  if (isDifferent(a.roundBottomRight, b.roundBottomRight, kGeomEpsilon)) return true;

  return false;
}
}

FaceRenderer::FaceRenderer(IDisplayPort& displayPort)
  : displayPort_(displayPort) {
}

bool FaceRenderer::render(const EyeModel& leftEye, const EyeModel& rightEye) {
  const unsigned long nowMs = millis();

  if (!hasLastFrame_) {
    displayPort_.clearFrame();
  } else {
    const bool changed = hasMeaningfulChange(leftEye, rightEye);
    const bool refreshAged = (nowMs - lastRenderMs_) >= kIdleRefreshMs;
    if (!changed && !refreshAged) {
      return false;
    }
  }

  Rect dirty = mergeRect(getEyeDirtyRect(hasLastFrame_ ? lastLeft_ : leftEye),
                         getEyeDirtyRect(hasLastFrame_ ? lastRight_ : rightEye));
  dirty = mergeRect(dirty, getEyeDirtyRect(leftEye));
  dirty = mergeRect(dirty, getEyeDirtyRect(rightEye));

  if (dirty.valid) {
    displayPort_.clearRect(dirty.x, dirty.y, dirty.w, dirty.h);
  }

  if (leftEye.useShapeV2) {
    displayPort_.drawEyeShape(leftEye.centerX,
                              leftEye.centerY,
                              leftEye.eyeRadius,
                              leftEye.openness,
                              leftEye.tiltDeg,
                              leftEye.squashY,
                              leftEye.stretchX,
                              leftEye.upperLid,
                              leftEye.lowerLid,
                              leftEye.shape);
  } else {
    displayPort_.drawEye(leftEye.centerX,
                         leftEye.centerY,
                         leftEye.eyeRadius,
                         leftEye.openness,
                         leftEye.tiltDeg,
                         leftEye.squashY,
                         leftEye.stretchX,
                         leftEye.roundness,
                         leftEye.upperLid,
                         leftEye.lowerLid);
  }

  if (rightEye.useShapeV2) {
    displayPort_.drawEyeShape(rightEye.centerX,
                              rightEye.centerY,
                              rightEye.eyeRadius,
                              rightEye.openness,
                              rightEye.tiltDeg,
                              rightEye.squashY,
                              rightEye.stretchX,
                              rightEye.upperLid,
                              rightEye.lowerLid,
                              rightEye.shape);
  } else {
    displayPort_.drawEye(rightEye.centerX,
                         rightEye.centerY,
                         rightEye.eyeRadius,
                         rightEye.openness,
                         rightEye.tiltDeg,
                         rightEye.squashY,
                         rightEye.stretchX,
                         rightEye.roundness,
                         rightEye.upperLid,
                         rightEye.lowerLid);
  }

  displayPort_.present();

  lastLeft_ = leftEye;
  lastRight_ = rightEye;
  hasLastFrame_ = true;
  lastRenderMs_ = nowMs;
  return true;
}

FaceRenderer::Rect FaceRenderer::getEyeDirtyRect(const EyeModel& eye) const {
  Rect rect;

  const int halfW = static_cast<int>((eye.eyeRadius + 18) * eye.stretchX);
  const int halfH = static_cast<int>((eye.eyeRadius + 18) * eye.squashY);

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
  if (leftEye.centerX != lastLeft_.centerX || leftEye.centerY != lastLeft_.centerY) return true;
  if (rightEye.centerX != lastRight_.centerX || rightEye.centerY != lastRight_.centerY) return true;
  if (leftEye.eyeRadius != lastLeft_.eyeRadius || rightEye.eyeRadius != lastRight_.eyeRadius) return true;
  if (leftEye.expression != lastLeft_.expression || rightEye.expression != lastRight_.expression) return true;

  if (isDifferent(leftEye.openness, lastLeft_.openness, kGeomEpsilon)) return true;
  if (isDifferent(rightEye.openness, lastRight_.openness, kGeomEpsilon)) return true;

  if (isDifferent(leftEye.upperLid, lastLeft_.upperLid, kLidEpsilon)) return true;
  if (isDifferent(leftEye.lowerLid, lastLeft_.lowerLid, kLidEpsilon)) return true;
  if (isDifferent(rightEye.upperLid, lastRight_.upperLid, kLidEpsilon)) return true;
  if (isDifferent(rightEye.lowerLid, lastRight_.lowerLid, kLidEpsilon)) return true;

  if (isDifferent(leftEye.tiltDeg, lastLeft_.tiltDeg, kTiltEpsilon)) return true;
  if (isDifferent(rightEye.tiltDeg, lastRight_.tiltDeg, kTiltEpsilon)) return true;

  if (isDifferent(leftEye.squashY, lastLeft_.squashY, kGeomEpsilon)) return true;
  if (isDifferent(rightEye.squashY, lastRight_.squashY, kGeomEpsilon)) return true;

  if (isDifferent(leftEye.stretchX, lastLeft_.stretchX, kGeomEpsilon)) return true;
  if (isDifferent(rightEye.stretchX, lastRight_.stretchX, kGeomEpsilon)) return true;

  if (isDifferent(leftEye.roundness, lastLeft_.roundness, kGeomEpsilon)) return true;
  if (isDifferent(rightEye.roundness, lastRight_.roundness, kGeomEpsilon)) return true;

  if (leftEye.useShapeV2 != lastLeft_.useShapeV2 || rightEye.useShapeV2 != lastRight_.useShapeV2) return true;
  if (shapeDifferent(leftEye.shape, lastLeft_.shape)) return true;
  if (shapeDifferent(rightEye.shape, lastRight_.shape)) return true;

  return false;
}
