#include <Arduino.h>
#include "app/app_factory.h"

static AppFactory appFactory;

void setup() {
  appFactory.init();
}

void loop() {
  appFactory.update();
}
