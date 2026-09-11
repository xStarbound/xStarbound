#include "StarListener.hpp"

namespace Star {

Listener::~Listener() {}

CallbackListener::CallbackListener(function<void()> callback)
    : callback(std::move(callback)) {}

void CallbackListener::trigger() {
  if (callback)
    callback();
}

TrackerListener::TrackerListener() : triggered(false) {}

void ListenerGroup::addListener(ListenerWeakPtr listener) {
  RecursiveMutexLocker locker(m_mutex);
  m_listeners.insert(std::move(listener));
}

void ListenerGroup::removeListener(ListenerWeakPtr listener) {
  RecursiveMutexLocker locker(m_mutex);
  m_listeners.erase(std::move(listener));
}

void ListenerGroup::clearExpiredListeners() {
  RecursiveMutexLocker locker(m_mutex);
  eraseWhere(m_listeners, mem_fn(&ListenerWeakPtr::expired));
};

void ListenerGroup::clearAllListeners() {
  RecursiveMutexLocker locker(m_mutex);
  m_listeners.clear();
}

void ListenerGroup::trigger() {
  RecursiveMutexLocker locker(m_mutex);
  // FezzedOne: Needed to avoid iterator invalidation by any triggered listeners that end up adding their own listener down the line.
  List<ListenerWeakPtr> listenersToCheck = {};
  for (auto listener : m_listeners) {
    listenersToCheck.append(listener);
  }
  for (auto const& listener : listenersToCheck) {
    if (auto lock = listener.lock())
      lock->trigger();
  }
  eraseWhere(m_listeners, mem_fn(&ListenerWeakPtr::expired));
  // filter(m_listeners, [](ListenerWeakPtr const& wl) {
  //   if (auto lock = wl.lock()) {
  //     lock->trigger();
  //     return true;
  //   } else {
  //     return false;
  //   }
  // });
}

} // namespace Star
