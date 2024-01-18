#ifndef STAR_REF_PTR_HPP
#define STAR_REF_PTR_HPP

#include "StarException.hpp"
#include "StarHash.hpp"

namespace Star {

class RefZeroTracker {
public:
  friend void refPtrIncRef(RefZeroTracker* p);
  friend void refPtrDecRef(RefZeroTracker* p);
  friend bool trackerZeroed(RefZeroTracker* p);
  friend void setTrackerZeroed(RefZeroTracker* p);

  RefZeroTracker();

private:
  size_t m_refCounter;
  bool m_objectZeroed;
};

// Base class for RefPtr that is NOT thread safe.  This can have a performance
// benefit over shared_ptr in single threaded contexts.
class RefCounter {
public:
  friend void refPtrIncRef(RefCounter* p);
  friend void refPtrDecRef(RefCounter* p);
  friend void refPtrZeroRef(RefCounter* p);

protected:
  RefCounter();
  virtual ~RefCounter() = default;

private:
  size_t m_refCounter;
};

// Reference counted ptr for intrusive reference counted types.  Calls
// unqualified refPtrIncRef and refPtrDecRef functions to manage the reference
// count.
template <typename T>
class RefPtr {
public:
  typedef T element_type;

  RefPtr();
  // explicit RefPtr(T* p, bool addRef = true);
  explicit RefPtr(T* p, RefZeroTracker* p2, bool addRef = true);

  RefPtr(RefPtr const& r);
  RefPtr(RefPtr&& r);

  template <typename T2>
  RefPtr(RefPtr<T2> const& r);
  template <typename T2>
  RefPtr(RefPtr<T2>&& r);

  ~RefPtr();

  RefPtr& operator=(RefPtr const& r);
  RefPtr& operator=(RefPtr&& r);

  template <typename T2>
  RefPtr& operator=(RefPtr<T2> const& r);
  template <typename T2>
  RefPtr& operator=(RefPtr<T2>&& r);

  void reset();

  // FezzedOne: Hack to destroy a referenced object.
  void zero();

  // void reset(T* r, bool addRef = true, bool resetAllRefs = false);
  void reset(T* r, RefZeroTracker* rt, bool addRef = true, bool resetAllRefs = false);

  T& operator*() const;
  T* operator->() const;
  T* get() const;

  explicit operator bool() const;

private:
  template <typename T2>
  friend class RefPtr;

  T* m_ptr;
  RefZeroTracker* m_refTrackerPtr;
};

template <typename T, typename U>
bool operator==(RefPtr<T> const& a, RefPtr<U> const& b);

template <typename T, typename U>
bool operator!=(RefPtr<T> const& a, RefPtr<U> const& b);

template <typename T>
bool operator==(RefPtr<T> const& a, T* b);

template <typename T>
bool operator!=(RefPtr<T> const& a, T* b);

template <typename T>
bool operator==(T* a, RefPtr<T> const& b);

template <typename T>
bool operator!=(T* a, RefPtr<T> const& b);

template <typename T, typename U>
bool operator<(RefPtr<T> const& a, RefPtr<U> const& b);

template <typename Type1, typename Type2>
bool is(RefPtr<Type2> const& p);

template <typename Type1, typename Type2>
bool is(RefPtr<Type2 const> const& p);

template <typename Type1, typename Type2>
RefPtr<Type1> as(RefPtr<Type2> const& p);

template <typename Type1, typename Type2>
RefPtr<Type1 const> as(RefPtr<Type2 const> const& p);

template <typename T, typename... Args>
RefPtr<T> make_ref(Args&&... args);

template <typename T>
struct hash<RefPtr<T>> {
  size_t operator()(RefPtr<T> const& a) const;

  hash<T*> hasher;
};

template <typename T>
RefPtr<T>::RefPtr()
  : m_ptr(nullptr), m_refTrackerPtr(nullptr) {}

// template <typename T>
// RefPtr<T>::RefPtr(T* p, bool addRef)
//   : m_ptr(nullptr), m_refTrackerPtr(nullptr) {
//   reset(p, addRef);
// }

template <typename T>
RefPtr<T>::RefPtr(T* p, RefZeroTracker* p2, bool addRef)
  : m_ptr(nullptr), m_refTrackerPtr(nullptr) {
  reset(p, p2, addRef);
}

template <typename T>
RefPtr<T>::RefPtr(RefPtr const& r)
  : RefPtr(r.m_ptr, r.m_refTrackerPtr) {}

template <typename T>
RefPtr<T>::RefPtr(RefPtr&& r) {
  m_ptr = r.m_ptr;
  m_refTrackerPtr = r.m_refTrackerPtr;
  r.m_ptr = nullptr;
  r.m_refTrackerPtr = nullptr;
}

template <typename T>
template <typename T2>
RefPtr<T>::RefPtr(RefPtr<T2> const& r)
  : RefPtr(r.m_ptr, r.m_refTrackerPtr) {}

template <typename T>
template <typename T2>
RefPtr<T>::RefPtr(RefPtr<T2>&& r) {
  m_ptr = r.m_ptr;
  m_refTrackerPtr = r.m_refTrackerPtr;
  r.m_ptr = nullptr;
  r.m_refTrackerPtr = nullptr;
}

template <typename T>
RefPtr<T>::~RefPtr() {
  if (m_ptr)
    refPtrDecRef(m_ptr);
  if (m_refTrackerPtr)
    refPtrDecRef(m_refTrackerPtr);
}

template <typename T>
RefPtr<T>& RefPtr<T>::operator=(RefPtr const& r) {
  reset(r.m_ptr, r.m_refTrackerPtr);
  return *this;
}

template <typename T>
RefPtr<T>& RefPtr<T>::operator=(RefPtr&& r) {
  if (m_ptr)
    refPtrDecRef(m_ptr);
  if (m_refTrackerPtr)
    refPtrDecRef(m_refTrackerPtr);

  m_ptr = r.m_ptr;
  m_refTrackerPtr = r.m_refTrackerPtr;
  r.m_ptr = nullptr;
  r.m_refTrackerPtr = nullptr;
  return *this;
}

template <typename T>
template <typename T2>
RefPtr<T>& RefPtr<T>::operator=(RefPtr<T2> const& r) {
  reset(r.m_ptr, r.m_refTrackerPtr);
  return *this;
}

template <typename T>
template <typename T2>
RefPtr<T>& RefPtr<T>::operator=(RefPtr<T2>&& r) {
  if (m_ptr)
    refPtrDecRef(m_ptr);
  if (m_refTrackerPtr)
    refPtrDecRef(m_refTrackerPtr);

  m_ptr = r.m_ptr;
  m_refTrackerPtr = r.m_refTrackerPtr;
  r.m_ptr = nullptr;
  r.m_refTrackerPtr = nullptr;
  return *this;
}

template <typename T>
void RefPtr<T>::reset() {
  reset(nullptr, nullptr);
}

template <typename T>
void RefPtr<T>::zero() {
  reset(nullptr, m_refTrackerPtr, true, true);
}

// template <typename T>
// void RefPtr<T>::reset(T* r, bool addRef, bool resetAllRefs) {
//   if (m_ptr == r)
//     return;

//   if (m_ptr) {
//     if (resetAllRefs) {
//       refPtrZeroRef(m_ptr);
//       // FezzedOne: Mark the object as "zeroed" to prevent invalid accesses.
//       setTrackerZeroed(m_refTrackerPtr);
//     } else {
//       refPtrDecRef(m_ptr);
//     }
//   }

//   if (m_refTrackerPtr)
//     refPtrDecRef(m_refTrackerPtr);

//   m_ptr = r;

//   if (m_ptr && addRef) {
//     if (!trackerZeroed(m_refTrackerPtr))
//       refPtrIncRef(m_ptr);
//     refPtrIncRef(m_refTrackerPtr);
//   }
// }


template <typename T>
void RefPtr<T>::reset(T* r, RefZeroTracker* rt, bool addRef, bool resetAllRefs) {
  if (m_ptr == r)
    return;

  if (m_ptr) {
    if (resetAllRefs) {
      refPtrZeroRef(m_ptr);
      // FezzedOne: Mark the object as "zeroed" to prevent invalid accesses.
      setTrackerZeroed(m_refTrackerPtr);
    } else {
      refPtrDecRef(m_ptr);
    }
  }

  if (m_refTrackerPtr)
    refPtrDecRef(m_refTrackerPtr);

  m_ptr = r;
  m_refTrackerPtr = rt;

  if (m_ptr && m_refTrackerPtr && addRef) {
    if (!trackerZeroed(m_refTrackerPtr))
      refPtrIncRef(m_ptr);
  }
  if (m_refTrackerPtr && addRef) {
    refPtrIncRef(m_refTrackerPtr);
  }
}

template <typename T>
T& RefPtr<T>::operator*() const {
  return *m_ptr;
}

template <typename T>
T* RefPtr<T>::operator->() const {
  return m_ptr;
}

// FezzedOne: `RefPtr`s now act "null" whenever the object behind them is "zeroed", except for explicit dereferences.
// But if you're going to dereference a `RefPtr`, do an `if` check on it first — `operator bool` checks for invalidity.
template <typename T>
T* RefPtr<T>::get() const {
  if (m_refTrackerPtr) {
    if (!trackerZeroed(m_refTrackerPtr))
      return m_ptr;
    else
      return nullptr;
  } else {
    return nullptr;
  }
}

template <typename T>
RefPtr<T>::operator bool() const {
  // FezzedOne: Now properly checks if a `RefPtr` was "zeroed".
  if (m_refTrackerPtr) {
    return m_ptr != nullptr && !trackerZeroed(m_refTrackerPtr);
  } else { // FezzedOne: If the tracker can't be found, assume the pointed-to object is corrupted and unsafe to access.
    return false;
  }
}

template <typename T, typename U>
bool operator==(RefPtr<T> const& a, RefPtr<U> const& b) {
  return a.get() == b.get();
}

template <typename T, typename U>
bool operator!=(RefPtr<T> const& a, RefPtr<U> const& b) {
  return a.get() != b.get();
}

template <typename T>
bool operator==(RefPtr<T> const& a, T* b) {
  return a.get() == b;
}

template <typename T>
bool operator!=(RefPtr<T> const& a, T* b) {
  return a.get() != b;
}

template <typename T>
bool operator==(T* a, RefPtr<T> const& b) {
  return a == b.get();
}

template <typename T>
bool operator!=(T* a, RefPtr<T> const& b) {
  return a != b.get();
}

template <typename T, typename U>
bool operator<(RefPtr<T> const& a, RefPtr<U> const& b) {
  return a.get() < b.get();
}

template <typename Type1, typename Type2>
bool is(RefPtr<Type2> const& p) {
  return (bool)dynamic_cast<Type1*>(p.get());
}

template <typename Type1, typename Type2>
bool is(RefPtr<Type2 const> const& p) {
  return (bool)dynamic_cast<Type1 const*>(p.get());
}

template <typename Type1, typename Type2>
RefPtr<Type1> as(RefPtr<Type2> const& p) {
  return RefPtr<Type1>(dynamic_cast<Type1*>(p.get()));
}

template <typename Type1, typename Type2>
RefPtr<Type1 const> as(RefPtr<Type2 const> const& p) {
  return RefPtr<Type1>(dynamic_cast<Type1 const*>(p.get()));
}

template <typename T, typename... Args>
RefPtr<T> make_ref(Args&&... args) {
  return RefPtr<T>(new T(forward<Args>(args)...), new RefZeroTracker());
}

template <typename T>
size_t hash<RefPtr<T>>::operator()(RefPtr<T> const& a) const {
  return hasher(a.get());
}

inline void refPtrIncRef(RefCounter* p) {
  ++p->m_refCounter;
}

inline void refPtrDecRef(RefCounter* p) {
  if (--p->m_refCounter == 0)
    delete p;
}

inline void refPtrZeroRef(RefCounter* p) {
  p->m_refCounter = 0;
  delete p;
}

inline void refPtrIncRef(RefZeroTracker* p) {
  ++p->m_refCounter;
}

inline void refPtrDecRef(RefZeroTracker* p) {
  if (--p->m_refCounter == 0)
    delete p;
}

inline bool trackerZeroed(RefZeroTracker* p) {
  return p->m_objectZeroed;
}

inline void setTrackerZeroed(RefZeroTracker* p) {
  p->m_objectZeroed = true;
}

inline RefCounter::RefCounter()
  : m_refCounter(0) {}

inline RefZeroTracker::RefZeroTracker()
  : m_refCounter(0), m_objectZeroed(false) {}

}

#endif
