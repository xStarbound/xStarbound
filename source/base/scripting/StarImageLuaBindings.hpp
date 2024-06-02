#pragma once

#include "StarLua.hpp"

namespace Star {

// FezzedOne: Handles byte arrays now. Don't want to bother changing the filename.

STAR_CLASS(Image);
STAR_CLASS(ByteArray);

template <>
struct LuaConverter<Image> : LuaUserDataConverter<Image> {};

template <>
struct LuaConverter<ByteArray> : LuaUserDataConverter<ByteArray> {};

template <>
struct LuaUserDataMethods<Image> {
  static LuaMethods<Image> make();
};

template <>
struct LuaUserDataMethods<ByteArray> {
  static LuaMethods<ByteArray> make();
};

}
