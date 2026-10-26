#ifndef STAR_DIRECTIVES_HPP
#define STAR_DIRECTIVES_HPP

#include "StarDataStream.hpp"
#include "StarHash.hpp"
#include "StarImageProcessing.hpp"
#include "StarStringView.hpp"
#include "StarThread.hpp"
#include "StarTtlCache.hpp"

#if defined TRACY_ENABLE
#include "tracy/Tracy.hpp"
#else
#define ZoneScoped
#define ZoneScopedN(name)
#endif

namespace Star {

STAR_CLASS(Directives);
STAR_CLASS(DirectivesGroup);
STAR_EXCEPTION(DirectivesException, StarException);

// Kae: Class used to optimise directive parsing and application on images.
class Directives {
private:
  // FezzedOne: Stores the pixel processing limit for image directives globally.
  static std::atomic<size_t>& getDirectivePixelLimit() {
    static std::atomic<size_t> s_directivePixelLimit = 16 * 1024 * 1024;
    return s_directivePixelLimit;
  }

  // FezzedOne: Stores the (raw, not UTF-8) character length limit for image directive strings globally.
  static std::atomic<size_t>& getDirectiveStringLengthLimit() {
    static std::atomic<size_t> s_directiveStringLengthLimit = 16 * 1024 * 1024;
    return s_directiveStringLengthLimit;
  }

  // FezzedOne: Precompiled directive cache.
  static Mutex& cachedDirectivesMutex() {
    static Mutex s_cachedDirectivesMutex;
    return s_cachedDirectivesMutex;
  }

  static HashTtlCache<String, Directives>& directivesCache() {
    static HashTtlCache<String, Directives> s_cachedDirectives(5 * 60 * 1000, 1000, 4096, true);
    return s_cachedDirectives;
  }

public:
  // FezzedOne: Thread-safe functions for getting and setting the image directives limit. Optimised for fast reads.
  static void setDirectivePixelLimit(size_t newLimit) {
    getDirectivePixelLimit().store(newLimit, std::memory_order_release);
  }

  static size_t directivePixelLimit() {
    return getDirectivePixelLimit().load(std::memory_order_acquire);
  }

  // FezzedOne: Ditto for the directive string length limit.
  static void setDirectiveStringLengthLimit(size_t newLimit) {
    getDirectiveStringLengthLimit().store(newLimit, std::memory_order_release);
  }

  static size_t directiveStringLengthLimit() {
    return getDirectiveStringLengthLimit().load(std::memory_order_acquire);
  }

  static Directives cachedDirectives(String const& directiveString) {
    ZoneScopedN("cachedDirectives");
    size_t len = directiveString.utf8().length();
    if (len <= 1024)
      return Directives(directiveString);
    MutexLocker locker(cachedDirectivesMutex());
    if (auto directives = directivesCache().ptr(directiveString))
      return *directives;
    locker.unlock();
    Directives parsedDirectives = Directives(directiveString);
    locker.lock();
    return directivesCache().get(std::move(directiveString), [&parsedDirectives](String const&) {
      return std::move(parsedDirectives);
    });
  }

  static void cleanupDirectivesCache() {
    ZoneScopedN("cleanupDirectivesCache");
    MutexLocker locker(cachedDirectivesMutex());
    directivesCache().cleanup();
  }

  static void setDirectivesCacheTimeToLive(float timeToLive) {
    MutexLocker locker(cachedDirectivesMutex());
    directivesCache().setTimeToLive((int64_t)(timeToLive * 1000.0f));
  }

  struct Shared;
  struct Entry {
    mutable ImageOperation operation;
    size_t begin;
    size_t length;

    ImageOperation const& loadOperation(Shared const& parent) const;
    StringView string(Shared const& parent) const;
    Entry(ImageOperation&& newOperation, size_t begin, size_t end);
    Entry(ImageOperation const& newOperation, size_t begin, size_t end);
    Entry(Entry const& other);
  };

  struct Shared {
    List<Entry> entries;
    String string;
    size_t hash = 0;
    mutable Mutex mutex;

    bool empty() const;
    Shared();
    Shared(List<Entry>&& givenEntries, String&& givenString);
  };

  Directives();
  Directives(String const& directives);
  Directives(String&& directives);
  Directives(const char* directives);
  Directives(Directives const& directives);
  Directives(Directives&& directives) noexcept;
  ~Directives();

  Directives& operator=(String const& s);
  Directives& operator=(String&& s);
  Directives& operator=(const char* s);
  Directives& operator=(Directives&& other) noexcept;
  Directives& operator=(Directives const& other);

  // @LumiKalt: Appends other's operations after this one's, adding the '?' between them if needed
  Directives& operator+=(Directives const& other);

  void loadOperations() const;
  void parse(String&& directives);
  String string() const;
  StringView prefix() const;
  String const* stringPtr() const;
  String buildString() const;
  String& addToString(String& out) const;
  size_t hash() const;
  size_t size() const;
  bool empty() const;
  operator bool() const;

  Shared const& operator*() const;
  Shared const* operator->() const;

  bool equals(Directives const& other) const;
  bool equals(String const& string) const;

  bool operator==(Directives const& other) const;
  bool operator==(String const& string) const;
  bool operator!=(Directives const& other) const;
  bool operator!=(String const& string) const;

  friend DataStream& operator>>(DataStream& ds, Directives& directives);
  friend DataStream& operator<<(DataStream& ds, Directives const& directives);

  std::shared_ptr<Shared const> shared;
};

class DirectivesGroup {
public:
  DirectivesGroup();
  DirectivesGroup(String const& directives);
  DirectivesGroup(String&& directives);

  bool empty() const;
  operator bool() const;
  bool compare(DirectivesGroup const& other) const;
  void append(Directives const& other);
  void clear();

  DirectivesGroup& operator+=(Directives const& other);
  // FezzedOne: Slightly weird to use this operator, but it looks like `<<` without clobbering `<<` or `+=`.
  DirectivesGroup& operator<<=(DirectivesGroup const& other);

  String toString() const;
  void addToString(String& string) const;

  typedef function<void(Directives::Entry const&, Directives const&)> DirectivesCallback;
  typedef function<bool(Directives::Entry const&, Directives const&)> AbortableDirectivesCallback;

  void forEach(DirectivesCallback callback) const;
  bool forEachAbortable(AbortableDirectivesCallback callback) const;

  // @LumiKalt: The paths of all the images that the operations refer to (masks, blends), loading the operations as needed
  StringList imageReferences() const;

  Image applyNewImage(const Image& image, ImageReferenceCallback refCallback = {}) const;
  void applyExistingImage(Image& image, ImageReferenceCallback refCallback = {}) const;

  size_t hash() const;
  const List<Directives>& list() const;

  friend bool operator==(DirectivesGroup const& a, DirectivesGroup const& b);
  friend bool operator!=(DirectivesGroup const& a, DirectivesGroup const& b);

  friend DataStream& operator>>(DataStream& ds, DirectivesGroup& directives);
  friend DataStream& operator<<(DataStream& ds, DirectivesGroup const& directives);

private:
  void buildString(String& string, const DirectivesGroup& directives) const;

  List<Directives> m_directives;
  size_t m_count;
};

template <>
struct hash<DirectivesGroup> {
  size_t operator()(DirectivesGroup const& s) const;
};

typedef DirectivesGroup ImageDirectives;

inline Directives::Shared const& Directives::operator*() const {
  return *shared;
}
inline Directives::Shared const* Directives::operator->() const {
  if (!shared)
    throw DirectivesException("Directives::operator-> nullptr");
  return shared.get();
}

} // namespace Star

#endif
