#include "core/publishers/PublisherManager.hpp"

namespace base {

void PublisherManager::registerPublisher(Publisher* publisher) {
  publishers_.push_back(publisher);
}

void PublisherManager::publishAll(const Measurement& m) {
  for (Publisher* publisher : publishers_) {
    if (publisher->enabled()) {
      publisher->publish(m);
    }
  }
}

}  // namespace base
