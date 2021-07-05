#ifndef CONTROLLERQUEUE_C019_QUEUE_ELEMENT_H
#define CONTROLLERQUEUE_C019_QUEUE_ELEMENT_H

# include "../../ESPEasy_common.h"
# include "../CustomBuild/ESPEasyLimits.h"
# include "../DataStructs/DeviceStruct.h"
# include "../DataStructs/UnitMessageCount.h"
# include "../Globals/CPlugins.h"
# include "../Globals/Plugins.h"
# include "../Globals/ESPEasy_time.h"

struct EventStruct;

#ifdef USES_C019

/*********************************************************************************************\
* C019_queue_element for queueing requests for C019: Generic HTTP Advanced.
\*********************************************************************************************/
class C019_queue_element {
public:

  C019_queue_element() = default;

  C019_queue_element(C019_queue_element&& other) = default;

  C019_queue_element(const C019_queue_element& other) = delete;

  C019_queue_element(const struct EventStruct *event);

  bool                      isDuplicate(const C019_queue_element& other) const;

  const UnitMessageCount_t* getUnitMessageCount() const {
    return nullptr;
  }

  size_t getSize() const;

  String uri;

  String labelset;

  int values = 0;
  String metricName[VARS_PER_TASK];
  float value[VARS_PER_TASK];

  int idx                          = 0;
  int64_t _timestamp               = (int64_t)(node_time.sysTime*1000);
  taskIndex_t TaskIndex            = INVALID_TASK_INDEX;
  controllerIndex_t controller_idx = INVALID_CONTROLLER_INDEX;
  Sensor_VType sensorType          = Sensor_VType::SENSOR_TYPE_NONE;
};

#endif // USES_C019

#endif // CONTROLLERQUEUE_C019_QUEUE_ELEMENT_H
