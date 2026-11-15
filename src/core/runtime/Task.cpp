#include "core/runtime/Task.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace base {

Task::Task(TaskFunction fn, const char* name, uint32_t stackBytes, uint32_t priority, void* arg)
    : fn_(fn), name_(name), stack_(stackBytes), priority_(priority), arg_(arg) {}

Task::~Task() {
  stop();
}

bool Task::start() {
  if (handle_ != nullptr) {
    return false;
  }
  // xTaskCreate usa afinidad AUTO (tskNO_AFFINITY) por defecto (D-0053).
  // En core 3.x TaskHandle_t es struct tskTaskControlBlock* (ya no void*);
  // usamos un handle temporal y lo guardamos como void*.
  TaskHandle_t h = nullptr;
  const BaseType_t result = xTaskCreate(fn_, name_, stack_, arg_, priority_, &h);
  handle_ = h;
  return result == pdPASS;
}

void Task::stop() {
  if (handle_ != nullptr) {
    vTaskDelete(static_cast<TaskHandle_t>(handle_));
    handle_ = nullptr;
  }
}

}  // namespace base
