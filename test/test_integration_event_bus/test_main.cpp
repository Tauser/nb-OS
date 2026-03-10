#include <unity.h>

#include "../../src/core/event_bus.h"
#include "../../src/core/event_bus.cpp"

class CountingListener : public IEventListener {
public:
  int count = 0;
  EventType lastType = EventType::None;

  void onEvent(const Event& event) override {
    count++;
    lastType = event.type;
  }
};

void test_event_bus_delivers_to_subscriber_of_type() {
  EventBus bus;
  CountingListener listener;

  TEST_ASSERT_TRUE(bus.subscribe(EventType::EVT_TOUCH, &listener));

  Event ev;
  ev.type = EventType::EVT_TOUCH;
  ev.source = EventSource::SensorService;
  ev.value = 42;
  ev.timestamp = 100;

  bus.publish(ev);

  TEST_ASSERT_EQUAL_INT(1, listener.count);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(EventType::EVT_TOUCH), static_cast<int>(listener.lastType));
}

void test_event_bus_any_receives_multiple_events() {
  EventBus bus;
  CountingListener listener;

  TEST_ASSERT_TRUE(bus.subscribe(EventType::Any, &listener));

  Event a;
  a.type = EventType::EVT_TOUCH;
  a.source = EventSource::SensorService;
  a.timestamp = 1;
  bus.publish(a);

  Event b;
  b.type = EventType::EVT_VOICE_ACTIVITY;
  b.source = EventSource::VoiceService;
  b.timestamp = 2;
  bus.publish(b);

  TEST_ASSERT_EQUAL_INT(2, listener.count);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(EventType::EVT_VOICE_ACTIVITY), static_cast<int>(listener.lastType));
}

void test_event_bus_unsubscribe_stops_delivery() {
  EventBus bus;
  CountingListener listener;

  TEST_ASSERT_TRUE(bus.subscribe(EventType::EVT_TOUCH, &listener));
  TEST_ASSERT_TRUE(bus.unsubscribe(EventType::EVT_TOUCH, &listener));

  Event ev;
  ev.type = EventType::EVT_TOUCH;
  ev.source = EventSource::SensorService;
  ev.timestamp = 3;
  bus.publish(ev);

  TEST_ASSERT_EQUAL_INT(0, listener.count);
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_event_bus_delivers_to_subscriber_of_type);
  RUN_TEST(test_event_bus_any_receives_multiple_events);
  RUN_TEST(test_event_bus_unsubscribe_stops_delivery);
  UNITY_END();
}

void loop() {
}
