#include "src/Helpers/_CPlugin_Helper.h"

#include <SnappyProto.h>
#include <PrometheusArduino.h>

#ifdef USES_C019

#include "grafana_root_cert.h"

// #######################################################################################################
// ########################### Controller Plugin 019: Prometheus Remote Write ############################
// #######################################################################################################

# define CPLUGIN_019
# define CPLUGIN_ID_019 19
# define CPLUGIN_NAME_019 "Prometheus Remote Write"

# define C019_HTTP_URI_MAX_LEN  256

struct C019_ConfigStruct {
  void zero_last()
  {
    HttpUri[C019_HTTP_URI_MAX_LEN - 1] = 0;
  }

  char HttpUri[C019_HTTP_URI_MAX_LEN] = { 0 };
};

bool CPlugin_019(CPlugin::Function function, struct EventStruct *event, String& string)
{
  bool success = false;

  switch (function) {
    case CPlugin::Function::CPLUGIN_PROTOCOL_ADD: {
      Protocol[++protocolCount].Number     = CPLUGIN_ID_019;
      Protocol[protocolCount].usesAccount  = true;
      Protocol[protocolCount].usesPassword = true;
      Protocol[protocolCount].usesExtCreds = true;
      Protocol[protocolCount].usesMQTT     = false;
      Protocol[protocolCount].usesHost     = false;
      Protocol[protocolCount].usesPort     = false;
      Protocol[protocolCount].usesID       = false;
      Protocol[protocolCount].needsNetwork = true;
      Protocol[protocolCount].allowsExpire = false;
      break;
    }

    case CPlugin::Function::CPLUGIN_GET_DEVICENAME: {
      string = F(CPLUGIN_NAME_019);
      break;
    }

    case CPlugin::Function::CPLUGIN_INIT: {
      {
        MakeControllerSettings(ControllerSettings);

        if (AllocatedControllerSettings()) {
          LoadControllerSettings(event->ControllerIndex, ControllerSettings);
        }
      }
      success = init_c019_delay_queue(event->ControllerIndex);
      break;
    }

    case CPlugin::Function::CPLUGIN_EXIT: {
      exit_c019_delay_queue();
      break;
    }

# if 0
    case CPlugin::Function::CPLUGIN_PROTOCOL_TEMPLATE: {
      // XXX(mem): Is this a weird mechanism to get a default (?) value into some of the fields...
      event->String1 = "";
      event->String2 = "";
      break;
    }
# endif // if 0

    case CPlugin::Function::CPLUGIN_WEBFORM_LOAD: {
      {
        String HttpUri;

        if (!load_C019_ConfigStruct(event->ControllerIndex, HttpUri)) {
          return false;
        }
        addTableSeparator(F("Remote Write Config"), 2, 3);

        addFormTextBox(F("URI"), F("C019httpuri"), HttpUri, C019_HTTP_URI_MAX_LEN - 1);
      }
      {
        // Place in scope to delete ControllerSettings as soon as it is no longer needed
        MakeControllerSettings(ControllerSettings);

        if (!AllocatedControllerSettings()) {
          addHtmlError(F("Out of memory, cannot load page"));
        } else {
          LoadControllerSettings(event->ControllerIndex, ControllerSettings);
        }
      }
      break;
    }

    case CPlugin::Function::CPLUGIN_WEBFORM_SAVE: {
      std::shared_ptr<C019_ConfigStruct> customConfig(new C019_ConfigStruct);

      if (customConfig) {
        String httpuri = webArg(F("C019httpuri"));

        strlcpy(customConfig->HttpUri, httpuri.c_str(), sizeof(customConfig->HttpUri));
        customConfig->zero_last();
        SaveCustomControllerSettings(event->ControllerIndex, (byte *)customConfig.get(), sizeof(C019_ConfigStruct));
      }
      break;
    }

    case CPlugin::Function::CPLUGIN_PROTOCOL_SEND: {
      success = Create_schedule_HTTP_C019(event);
      break;
    }

    case CPlugin::Function::CPLUGIN_FLUSH: {
      process_c019_delay_queue();
      delay(0);
      break;
    }

    default:
      break;
  }
  return success;
}

// ********************************************************************************
// Prometheus Remote Write POST request
// ********************************************************************************

bool do_process_c019_delay_queue(int                       controller_number,
                                 const C019_queue_element& element,
                                 ControllerSettingsStruct& ControllerSettings);

bool do_process_c019_delay_queue(int controller_number, const C019_queue_element& element, ControllerSettingsStruct& ControllerSettings) {
  WiFiClientSecure client;

  if (!NetworkConnected())
  {
    addLog(LOG_LEVEL_ERROR, F("do_process_c019_delay_queue: network not connected"));
    return false;
  }

  if (element.uri.isEmpty())
  {
    addLog(LOG_LEVEL_ERROR, F("do_process_c019_delay_queue: uri is empty"));
    return false;
  }

  String user(getControllerUser(element.controller_idx, ControllerSettings));
  String pass(getControllerPass(element.controller_idx, ControllerSettings));

  if (user.isEmpty())
  {
    addLog(LOG_LEVEL_ERROR, F("do_process_c019_delay_queue: user is empty"));
    return false;
  }

  if (pass.isEmpty())
  {
    addLog(LOG_LEVEL_ERROR, F("do_process_c019_delay_queue: pass is empty"));
    return false;
  }

# if 0
  send_via_http(
    controller_number,
    ControllerSettings,
    element.controller_idx,
    client,
    element.uri,
    F("POST"),
    F(""), // header
    F(""), // post body?
    httpCode);
# endif // if 0

  // FIXME(mem): find a way to batch the requests, as it is, this is sending one request per _metric_.
  PromClient   pc(&client);
  WriteRequest req(VARS_PER_TASK);

  pc.setDebug(Serial);
  pc.setUrl(element.uri.c_str());
  pc.setUser(user.c_str());
  pc.setPass(pass.c_str());
  pc.setRootCA(grafanaCert);

  if (!pc.begin())
  {
    addLog(LOG_LEVEL_ERROR, F("client.begin() failed"));
    return false;
  }

  bool success = true;

  for (int i = 0; i < element.values; i++) {
    // FIXME(mem): this is creating the timeseries and destroying it; need to
    // rewrite constructors so that this is not necessary.
    TimeSeries ts(1, element.metricName[i].c_str(), element.labelset.c_str());

    ts.addSample(element._timestamp, element.value[i]);

    String line("timeseries ");
    line += i;
    line += " ";
    line += String((long)(element._timestamp));
    line += " ";
    line += element.metricName[i];
    line += " ";
    line += element.labelset;
    line += " ";
    line += element.value[i];

    addLog(LOG_LEVEL_INFO, line);

    req.addTimeSeries(ts);

    PromClient::SendResult res = pc.send(req);
    if (res != PromClient::SendResult::SUCCESS) {
      addLog(LOG_LEVEL_ERROR, pc.errmsg);
      success = false;
    }
  }

  // HTTP codes:
  // 1xx Informational response
  // 2xx Success
  return success;
}

bool load_C019_ConfigStruct(controllerIndex_t ControllerIndex, String& HttpUri)
{
  // Just copy the needed strings and destruct the C019_ConfigStruct as soon as possible
  std::shared_ptr<C019_ConfigStruct> customConfig(new C019_ConfigStruct);

  if (!customConfig) {
    return false;
  }

  LoadCustomControllerSettings(ControllerIndex, (byte *)customConfig.get(), sizeof(C019_ConfigStruct));
  customConfig->zero_last();
  HttpUri = customConfig->HttpUri;
  return true;
}

// ********************************************************************************
// Create request
// ********************************************************************************
boolean Create_schedule_HTTP_C019(struct EventStruct *event) {
  if (C019_DelayHandler == nullptr) {
    addLog(LOG_LEVEL_ERROR, F("No C019_DelayHandler"));
    return false;
  }

  LoadTaskSettings(event->TaskIndex);

  C019_queue_element e(event);

  String labelset = F("instance=\"%mac%\",job=\"%sysname%\"");

  parseControllerVariables(labelset, event, false);

  byte valueCount = getValueCountForTask(event->TaskIndex);

  e.values = 0;

  for (byte x = 0; x < valueCount; x++)
  {
    if (ExtraTaskSettings.TaskDeviceValueNames[x][0] == 0)
    {
      continue; // we skip values with empty labels
    }

    String tmplabelset = labelset;
    parseSingleControllerVariable(tmplabelset, event, x, false);

    // FIXME(mem): this should be out of the loop
    String ls = "{";
    ls += tmplabelset;
    ls += "}";
    e.labelset = ls;

    e.metricName[x] = ExtraTaskSettings.TaskDeviceValueNames[x];
    e.value[x] = getFloatUserVar(event, x);
    e.values++;
  }

  // Add a new element to the queue with the minimal payload
  bool success = C019_DelayHandler->addToQueue(std::move(e));

  if (success) {
    // Element was added.
    // Now we try to append to the existing element
    // and thus preventing the need to create a long string only to copy it to a queue element.
    C019_queue_element& element = C019_DelayHandler->sendQueue.back();

    if (!load_C019_ConfigStruct(event->ControllerIndex, element.uri))
    {
      if (loglevelActiveFor(LOG_LEVEL_ERROR))
      {
        String log = F("C019   : ");
        log += element.uri;
        addLog(LOG_LEVEL_ERROR, log);
      }
      C019_DelayHandler->sendQueue.pop_back();
      return false;
    }
  } else {
    addLog(LOG_LEVEL_ERROR, F("C019  : Could not add to delay handler"));
  }

  Scheduler.scheduleNextDelayQueue(ESPEasy_Scheduler::IntervalTimer_e::TIMER_C019_DELAY_QUEUE, C019_DelayHandler->getNextScheduleTime());

  return success;
}

static double getFloatUserVar(struct EventStruct *event, byte rel_index) {
  if (event == nullptr) {
    return NAN;
  }

  if (!validDeviceIndex(getDeviceIndex_from_TaskIndex(event->TaskIndex))) {
    return NAN;
  }

  if (rel_index >= getValueCountForTask(event->TaskIndex)) {
    return NAN;
  }

  switch (event->getSensorType()) {
  case Sensor_VType::SENSOR_TYPE_LONG:
    return (double)(UserVar.getSensorTypeLong(event->TaskIndex));

  case Sensor_VType::SENSOR_TYPE_STRING:
    return NAN;

  default:
    break;
  }

  return (double)(UserVar[event->BaseVarIndex + rel_index]);
}

#endif // ifdef USES_C019