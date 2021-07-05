#include "../ControllerQueue/C019_queue_element.h"

#include "../DataStructs/ESPEasy_EventStruct.h"

#ifdef USES_C019

C019_queue_element::C019_queue_element(const struct EventStruct *event) : idx(event->idx),
  TaskIndex(event->TaskIndex),
  controller_idx(event->ControllerIndex),
  sensorType(event->sensorType) {}

size_t C019_queue_element::getSize() const {
  size_t total = sizeof(*this);

  total += uri.length();
  return total;
}

bool C019_queue_element::isDuplicate(const C019_queue_element& other) const {
  if (other.values != values) {
    return false;
  }

  if ((other.controller_idx != controller_idx) ||
      (other.TaskIndex != TaskIndex) ||
      (other.sensorType != sensorType) ||
      (other.idx != idx))
  {
    return false;
  }

  if (!other.labelset.equals(labelset)) {
    return false;
  }

  for (int i = 0; i < values; i++) {
    if (!other.metricName[i].equals(metricName[i])) {
      return false;
    }

    if (other.value[i] != value[i]) {
      return false;
    }
  }

  return other.uri.equals(uri);
}

#endif // ifdef USES_C019
