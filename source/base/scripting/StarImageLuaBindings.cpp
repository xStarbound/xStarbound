#include "StarImageLuaBindings.hpp"
#include "StarLuaConverters.hpp"
#include "StarImage.hpp"
#include "StarRootBase.hpp"

namespace Star {

LuaMethods<Image> LuaUserDataMethods<Image>::make() {
  // These act on stuff returned by `assets.image` and `assets.newImage`.
  LuaMethods<Image> methods;

  methods.registerMethodWithSignature<Vec2U, Image&>("size", mem_fn(&Image::size));
  methods.registerMethodWithSignature<void, Image&, Vec2U, Image&>("drawInto", mem_fn(&Image::drawInto));
  methods.registerMethodWithSignature<void, Image&, Vec2U, Image&>("copyInto", mem_fn(&Image::copyInto));
  methods.registerMethod("set", [](Image& image, unsigned x, unsigned y, Color const& color) {
    image.set(x, y, color.toRgba()); 
  });

  methods.registerMethod("get", [](Image& image, unsigned x, unsigned y) {
    return Color::rgba(image.get(x, y));
  });

  methods.registerMethod("subImage", [](Image& image, Vec2U const& min, Vec2U const& size) {
    return image.subImage(min, size);
  });

  methods.registerMethod("process", [](Image& image, String const& directives) {
    return processImageOperations(parseImageOperations(directives), image, [](String const& path) -> Image const* {
      if (auto root = RootBase::singletonPtr())
        return root->assets()->image(path).get();
      else // FezzedOne: This ought to really throw an exception; otherwise `processImageOperations` will segfault on some directive operations.
        throw StarException("Star::Root not loaded");
    });
  });

  return methods;
}

LuaMethods<ByteArray> LuaUserDataMethods<ByteArray>::make() {
  // These act on stuff returned by `assets.rawBytes` and `assets.newRawBytes`.
  LuaMethods<ByteArray> methods;

  methods.registerMethod("set", [](ByteArray& bytes, String const& byteStr) {
    bytes = ByteArray::fromCStringWithNull(byteStr.utf8().c_str());
  });

  methods.registerMethod("setByte", [](ByteArray& bytes, size_t pos, String const& singleByte) {
    // FezzedOne: Starts at 1 because that's how Lua does things.
    size_t byteSize = singleByte.size();
    if (pos != 0 && (pos - 1) < bytes.size() && byteSize == 1)
      bytes[pos - 1] = *singleByte.utf8().c_str();
    else {
      if (singleByte.size() != 1)
        throw LuaException(strf("setByte: Specified {} characters", byteSize == 0 ? "too few" : "too many"));
      else
        throw LuaException(strf("setByte: Attempted to set byte out of bounds at {}; size of byte array is {}", pos, bytes.size()));
    }
  });

  methods.registerMethod("get", [](ByteArray& bytes) {
    return String(bytes.ptr(), bytes.size());
  });

  return methods;
}

}
