#include "StarImageLuaBindings.hpp"
#include "StarLuaConverters.hpp"
#include "StarImage.hpp"
#include "StarRootBase.hpp"

namespace Star {

LuaMethods<Image> LuaUserDataMethods<Image>::make() {
  // These act on stuff returned by `assets.image` and `assets.newImage`.
  LuaMethods<Image> methods;

  methods.registerMethodWithSignature<Vec2U, Image&>("size", mem_fn(&Image::size));

  // FezzedOne: Not returning the image is unintuitive for asset modders. Fixed that.
  /* { */
  methods.registerMethod("copyInto", [](Image& image, Vec2U pos, Image const& subImage) {
    image.copyInto(pos, subImage);
    return image;
  });

  methods.registerMethod("drawInto", [](Image& image, Vec2U pos, Image const& subImage) {
    image.drawInto(pos, subImage);
    return image;
  });

  methods.registerMethod("set", [](Image& image, unsigned x, unsigned y, Color const& color) {
    image.set(x, y, color.toRgba());
    return image;
  });

  methods.registerMethod("setHex", [](Image& image, unsigned x, unsigned y, String const& hex) {
    image.set(x, y, Color::fromHex(hex).toRgba());
    return image;
  });
  /* } */

  methods.registerMethod("get", [](Image& image, unsigned x, unsigned y) {
    return Color::rgba(image.get(x, y));
  });

  methods.registerMethod("getHex", [](Image& image, unsigned x, unsigned y) {
    return Color::rgba(image.get(x, y)).toHex();
  });

  methods.registerMethod("subImage", [](Image& image, Vec2U const& min, Vec2U const& size) {
    return image.subImage(min, size);
  });

  methods.registerMethod("process", [](Image& image, String const& directives) {
    return processImageOperations(parseImageOperations(directives), image, [](String const& path) -> Image const* {
      if (auto root = RootBase::singletonPtr())
        return root->assets()->image(path).get();
      else // FezzedOne: Throws an exception on directives that take image paths if called in an asset preprocessor or patch script.
        throw StarException("Star::Root not loaded");
    });
  });

  methods.registerMethod("copy", [](Image& image, Vec2U const& min, Vec2U const& size) {
    return Image(image);
  });

  return methods;
}

LuaMethods<ByteArray> LuaUserDataMethods<ByteArray>::make() {
  // These act on stuff returned by `assets.rawBytes` and `assets.newRawBytes`.
  LuaMethods<ByteArray> methods;

  methods.registerMethodWithSignature<size_t, ByteArray&>("size", mem_fn(&ByteArray::size));

  methods.registerMethod("clear", [](ByteArray& bytes) {
    bytes.reset();
    return bytes;
  });

  methods.registerMethod("set", [](ByteArray& bytes, LuaEngine& engine, LuaValue const& byteSequence) {
    bytes.reset();
    if (auto newByteList = engine.luaMaybeTo<List<uint8_t>>(byteSequence)) {
      const size_t byteLen = newByteList->count();
      bytes.reserve(byteLen);
      bytes.append((char*)newByteList->ptr(), byteLen);
    } else if (auto newByteString = engine.luaMaybeTo<LuaString>(byteSequence)) {
      const size_t byteStrLen = newByteString->length();
      bytes.reserve(byteStrLen);
      bytes.append(newByteString->ptr(), byteStrLen);
    } else if (auto newByteArray = engine.luaMaybeTo<ByteArray>(byteSequence)) {
      const size_t byteArrayLen = newByteArray->size();
      bytes.reserve(byteArrayLen);
      bytes.append(*newByteArray);
    } else {
      throw LuaException("set: Expected an array of 8-bit unsigned integers, a string or a ByteArray.");
    }
    return bytes;
  });

  methods.registerMethod("setByte", [](ByteArray& bytes, LuaEngine& engine, size_t pos, LuaValue const& singleByte) {
    // FezzedOne: Starts at 1 because that's how Lua does things.
    if (auto newByteChar = engine.luaMaybeTo<uint8_t>(singleByte)) {
      if (pos != 0 && (pos - 1) < bytes.size())
        bytes[pos - 1] = (char)*newByteChar;
      else {
        throw LuaException(strf("setByte: Attempted to set byte out of bounds at {}; size of byte array is {}", pos, bytes.size()));
      }
    } else if (auto newByteString = engine.luaMaybeTo<LuaString>(singleByte)) {
      const size_t byteSize = newByteString->length();
      if (pos != 0 && (pos - 1) < bytes.size() && byteSize == 1)
        bytes[pos - 1] = *newByteString->ptr();
      else {
        if (byteSize != 1)
          throw LuaException(strf("setByte: Specified {} characters", byteSize == 0 ? "too few" : "too many"));
        else
          throw LuaException(strf("setByte: Attempted to set byte out of bounds at {}; size of byte array is {}", pos, bytes.size()));
      }
    } else {
      throw LuaException("setByte: Expected 8-bit unsigned integer or single-character string.");
    }
    return bytes;
  });

  methods.registerMethod("append", [](ByteArray& bytes, LuaEngine& engine, LuaValue const& newBytes, Maybe<size_t> maybeTimes) {
    const size_t times = maybeTimes.value(1);
    if (times > 0) {
      const char nullByte = '\0';
      const size_t existingSize = bytes.size();
      if (auto newByteChar = engine.luaMaybeTo<uint8_t>(newBytes)) {
        bytes.reserve(existingSize + times);
        for (size_t n = 0; n < times; n++) {
          bytes.appendByte(*newByteChar);
        }
      } else if (auto newByteList = engine.luaMaybeTo<List<uint8_t>>(newBytes)) {
        const size_t listSize = newByteList->count();
        bytes.reserve(existingSize + listSize * times);
        for (size_t n = 0; n < times; n++) {
          bytes.append((char*)newByteList->ptr(), listSize);
        }
      } else if (auto newByteArray = engine.luaMaybeTo<ByteArray>(newBytes)) {
        const size_t chunkSize = newByteArray->size();
        const size_t fullSize = existingSize + chunkSize * times;
        bytes.reserve(fullSize);
        for (size_t n = 0; n < times; n++) {
          bytes.append(*newByteArray);
        }
      } else if (auto newByteString = engine.luaMaybeTo<LuaString>(newBytes)) {
        const size_t chunkSize = newByteString->length();
        const size_t fullSize = existingSize + chunkSize * times;
        bytes.reserve(fullSize);
        for (size_t n = 0; n < times; n++) {
          bytes.append(newByteString->ptr(), chunkSize);
        }
      } else if (newBytes == LuaNil) {
        bytes.reserve(existingSize + times);
        for (size_t n = 0; n < times; n++) {
          bytes.appendByte('\0');
        }
      } else {
        throw LuaException("append: Expected a ByteArray, array of 8-bit unsigned integers, a single 8-bit unsigned integer, a string or nil.");
      }
    }
    return bytes;
  });

  methods.registerMethod("get", [](ByteArray& bytes, LuaEngine& engine) {
    return engine.createString(bytes.ptr(), bytes.size());
  });

  methods.registerMethod("getBytes", [](ByteArray& bytes) {
    List<uint8_t> byteArray{};
    const size_t size = bytes.size();
    for (size_t n = 0; n < size; n++)
      byteArray.append(bytes[n]);
    return byteArray;
  });

  methods.registerMethod("getByte", [](ByteArray& bytes, LuaEngine& engine, size_t pos) {
    // FezzedOne: Starts at 1 because that's how Lua does things.
    if (pos != 0 && (pos - 1) < bytes.size())
      return engine.createString(bytes.ptr() + (pos - 1), 1);
    else {
      throw LuaException(strf("getByte: Attempted to get byte out of bounds at {}; size of byte array is {}", pos, bytes.size()));
    }
  });

  methods.registerMethod("getByteChar", [](ByteArray& bytes, size_t pos) {
    // FezzedOne: Starts at 1 because that's how Lua does things.
    if (pos != 0 && (pos - 1) < bytes.size())
      return (uint8_t)(*(bytes.ptr() + (pos - 1)));
    else {
      throw LuaException(strf("getByte: Attempted to get byte out of bounds at {}; size of byte array is {}", pos, bytes.size()));
    }
  });

  methods.registerMethod("getSubBytes", [](ByteArray& bytes, size_t pos, size_t len) {
    // FezzedOne: Starts at 1 because that's how Lua does things.
    if (len == 0 && (pos - 1) < bytes.size())
      return ByteArray();
    if (pos != 0 && len != 0 && (pos - 1) < bytes.size() && ((pos - 1) + len) <= bytes.size())
      return ByteArray(bytes.ptr() + (pos - 1), len);
    else {
      throw LuaException(strf("getByte: Attempted to get bytes out of bounds from {} to {}; size of byte array is {}", pos, pos + len - 1, bytes.size()));
    }
  });

  methods.registerMethod("copy", [](ByteArray& bytes, size_t pos, size_t len) {
    return ByteArray(bytes);
  });

  return methods;
}

}
