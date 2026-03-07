#include "face_renderer.h"

FaceRenderer::FaceRenderer(DisplayHAL& displayHal)
  : displayHal_(displayHal) {
}

void FaceRenderer::render(const EyeModel& leftEye, const EyeModel& rightEye) {
  displayHal_.clearFrame();
  displayHal_.drawText(20, 20, "FACE ENGINE OK");

  displayHal_.drawEye(
    leftEye.centerX,
    leftEye.centerY,
    leftEye.eyeRadius,
    leftEye.openness
  );

  displayHal_.drawEye(
    rightEye.centerX,
    rightEye.centerY,
    rightEye.eyeRadius,
    rightEye.openness
  );

  if (leftEye.openness > 0.3f) {
    displayHal_.drawPupil(
      leftEye.centerX + leftEye.pupilOffsetX,
      leftEye.centerY + leftEye.pupilOffsetY,
      leftEye.pupilRadius
    );
  }

  if (rightEye.openness > 0.3f) {
    displayHal_.drawPupil(
      rightEye.centerX + rightEye.pupilOffsetX,
      rightEye.centerY + rightEye.pupilOffsetY,
      rightEye.pupilRadius
    );
  }

  displayHal_.present();
}