#include "LocationStore.h"

LocationStore::LocationStore() : mutex_(xSemaphoreCreateMutex()) {}

LocationStore::~LocationStore() {
  if (mutex_ != nullptr) {
    vSemaphoreDelete(mutex_);
  }
}

bool LocationStore::read(LocationFix &destination) const {
  if (mutex_ == nullptr || xSemaphoreTake(mutex_, kMutexWaitTicks) != pdTRUE) {
    return false;
  }

  destination = fix_;
  xSemaphoreGive(mutex_);
  return true;
}

bool LocationStore::write(const LocationFix &source) {
  if (mutex_ == nullptr || xSemaphoreTake(mutex_, kMutexWaitTicks) != pdTRUE) {
    return false;
  }

  fix_ = source;
  xSemaphoreGive(mutex_);
  return true;
}
