#include "StarImageProcessing.hpp"
#include "StarMatrix3.hpp"
#include "StarInterpolation.hpp"
#include "StarLexicalCast.hpp"
#include "StarColor.hpp"
#include "StarImage.hpp"
#include "StarStringView.hpp"
#include "StarEncode.hpp"
//#include "StarTime.hpp"
#include "StarLogging.hpp"

namespace Star {

Image scaleNearest(Image const& srcImage, Vec2F scale) {
  if (!(scale[0] == 1.0f && scale[1] == 1.0f)) {
    // «Downstreamed» from Kae. Fixes a segfault.
    if ((scale[0] < 0.0f || scale[1] < 0.0f)) {
      Logger::warn("scalenearest: Scale must be non-negative!");
      scale = Vec2F(std::abs(scale[0]), std::abs(scale[1]));
    }
    // FezzedOne: Fixes a CPU pegging exploit.
    if ((scale[0] > 4096.0f || scale[1] > 4096.0f)) {
      Logger::warn("scalenearest: Scale may not exceed 4096x in either dimension!");
      scale = scale.piecewiseMin(Vec2F::filled(4096.0f));
    }
    Vec2U srcSize = srcImage.size();
    Vec2U destSize = Vec2U::round(vmult(Vec2F(srcSize), scale));
    destSize[0] = max(destSize[0], 1u);
    destSize[1] = max(destSize[1], 1u);

    Image destImage(destSize, srcImage.pixelFormat());

    for (unsigned y = 0; y < destSize[1]; ++y) {
      for (unsigned x = 0; x < destSize[0]; ++x)
        destImage.set({x, y}, srcImage.clamp(Vec2I::round(vdiv(Vec2F(x, y), scale))));
    }
    return destImage;
  } else {
    return srcImage;
  }
}

#if defined STAR_COMPILER_MSVC
  // FezzedOne: Needed to use `/fp:strict` for this function on MSVC and disable `-ffast-math` on recent versions of Clang.
  #pragma float_control(precise, on)  // enable precise semantics
  #pragma fenv_access(on)             // enable environment sensitivity
  #pragma float_control(except, on)   // enable exception semantics
#elif defined STAR_COMPILER_CLANG
  // #pragma clang optimize off
#elif defined STAR_COMPILER_GNU
  // FezzedOne: Disable floating-point optimisations on GCC.
  #pragma GCC optimize("float-store")
#endif
Image scaleBilinear(Image const& srcImage, Vec2F scale) {
  #if defined STAR_COMPILER_CLANG
    #pragma clang fp reciprocal(off) reassociate(off) contract(on) exceptions(strict)
  #endif
  if (!(scale[0] == 1.0f && scale[1] == 1.0f)) {
    // «Downstreamed» from Kae. Fixes a segfault.
    if ((scale[0] < 0.0f || scale[1] < 0.0f)) {
      Logger::warn("scalebilinear: Scale must be non-negative!");
      scale = Vec2F(std::abs(scale[0]), std::abs(scale[1]));
    }
    // FezzedOne: Fixes a CPU pegging exploit.
    if ((scale[0] > 4096.0f || scale[1] > 4096.0f)) {
      Logger::warn("scalebilinear: Scale may not exceed 4096x in either dimension!");
      scale = scale.piecewiseMin(Vec2F::filled(4096.0f));
    }
    // union ScaleFloatToHex { float floatVal; int byteVal; };
    // int byteScale_0 = ScaleFloatToHex{.floatVal = scale[0]}.byteVal;
    // int byteScale_1 = ScaleFloatToHex{.floatVal = scale[1]}.byteVal;
    // Logger::info("[Debug] scalebilinear: Scale is {{ {:}, {:} }}, hex {{ {:x}, {:x} }}", scale[0], scale[1], byteScale_0, byteScale_1);
    Vec2U srcSize = srcImage.size();
    Vec2U destSize;
    {
      // #if defined STAR_COMPILER_CLANG
      //   // FezzedOne: Needed to disable `-ffast-math` on Clang.
      //   #pragma clang fp reciprocal(off) reassociate(off) contract(on) exceptions(strict)
      // #endif
      destSize = Vec2U::round(vmult(Vec2F(srcSize), scale));
    }
    destSize[0] = max(destSize[0], 1u);
    destSize[1] = max(destSize[1], 1u);

    Image destImage(destSize, srcImage.pixelFormat());

    for (unsigned y = 0; y < destSize[1]; ++y) {
      for (unsigned x = 0; x < destSize[0]; ++x) {
        Vec4B processedResult;

        {
          // #if defined STAR_COMPILER_CLANG
          //   // FezzedOne: Needed to disable `-ffast-math` on Clang.
          //   #pragma clang fp reciprocal(off) reassociate(off) contract(on) exceptions(strict)
          // #endif
          auto pos = vdiv(Vec2F(x, y), scale);
          auto ipart = Vec2I::floor(pos);
          auto fpart = pos - Vec2F(ipart);

          Vec4F topLeft     = Vec4F(srcImage.clamp(ipart[0],       ipart[1]));
          Vec4F topRight    = Vec4F(srcImage.clamp(ipart[0] + 1,   ipart[1]));
          Vec4F bottomLeft  = Vec4F(srcImage.clamp(ipart[0],       ipart[1] + 1));
          Vec4F bottomRight = Vec4F(srcImage.clamp(ipart[0] + 1,   ipart[1] + 1));

          // FezzedOne: Inlined lerp function.
          auto inlineLerp = [](float offset, Vec4F f0, Vec4F f1) -> Vec4F {
            return f0 * (1.0f - offset) + f1 * (offset);
          };

          // Vec4F left    = inlineLerp(fpart[1], topLeft,   bottomLeft);
          // Vec4F right   = inlineLerp(fpart[1], topRight,  bottomRight);
          // Vec4F result  = inlineLerp(fpart[0], left,      right);

          Vec4F top     = inlineLerp(fpart[0], topLeft,     topRight);
          Vec4F bottom  = inlineLerp(fpart[0], bottomLeft,  bottomRight);
          Vec4F result  = inlineLerp(fpart[1], top,         bottom);

          processedResult = Vec4B(result);
        }

        destImage.set({x, y}, processedResult);
      }
    }

    return destImage;
  } else {
    return srcImage;
  }
}
#if defined STAR_COMPILER_MSVC
  // FezzedOne: Reset everything back to the default MSVC options for the build.
  #pragma float_control(except, off)  // disable exception semantics
  #pragma fenv_access(off)            // disable environment sensitivity
  #pragma float_control(precise, off) // disable precise semantics
#elif defined STAR_COMPILER_CLANG
  // #pragma clang optimize on
#elif defined STAR_COMPILER_GNU
  // FezzedOne: Reset to whatever MinGW GCC options were specified in CMakeLists.txt.
  #pragma GCC reset_options
#endif

Image scaleBicubic(Image const& srcImage, Vec2F scale) {
  if (!(scale[0] == 1.0f && scale[1] == 1.0f)) {
    // «Downstreamed» from Kae. Fixes a segfault.
    if ((scale[0] < 0.0f || scale[1] < 0.0f)) {
      Logger::warn("scalebicubic: Scale must be non-negative!");
      scale = Vec2F(std::abs(scale[0]), std::abs(scale[1]));
    }
    // FezzedOne: Fixes a CPU pegging exploit.
    if ((scale[0] > 4096.0f || scale[1] > 4096.0f)) {
      Logger::warn("scalebicubic: Scale may not exceed 4096x in either dimension!");
      scale = scale.piecewiseMin(Vec2F::filled(4096.0f));
    }
    Vec2U srcSize = srcImage.size();
    Vec2U destSize = Vec2U::round(vmult(Vec2F(srcSize), scale));
    destSize[0] = max(destSize[0], 1u);
    destSize[1] = max(destSize[1], 1u);

    Image destImage(destSize, srcImage.pixelFormat());

    for (unsigned y = 0; y < destSize[1]; ++y) {
      for (unsigned x = 0; x < destSize[0]; ++x) {
        auto pos = vdiv(Vec2F(x, y), scale);
        auto ipart = Vec2I::floor(pos);
        auto fpart = pos - Vec2F(ipart);

        Vec4F a = cubic4(fpart[0],
            Vec4F(srcImage.clamp(ipart[0], ipart[1])),
            Vec4F(srcImage.clamp(ipart[0] + 1, ipart[1])),
            Vec4F(srcImage.clamp(ipart[0] + 2, ipart[1])),
            Vec4F(srcImage.clamp(ipart[0] + 3, ipart[1])));

        Vec4F b = cubic4(fpart[0],
            Vec4F(srcImage.clamp(ipart[0], ipart[1] + 1)),
            Vec4F(srcImage.clamp(ipart[0] + 1, ipart[1] + 1)),
            Vec4F(srcImage.clamp(ipart[0] + 2, ipart[1] + 1)),
            Vec4F(srcImage.clamp(ipart[0] + 3, ipart[1] + 1)));

        Vec4F c = cubic4(fpart[0],
            Vec4F(srcImage.clamp(ipart[0], ipart[1] + 2)),
            Vec4F(srcImage.clamp(ipart[0] + 1, ipart[1] + 2)),
            Vec4F(srcImage.clamp(ipart[0] + 2, ipart[1] + 2)),
            Vec4F(srcImage.clamp(ipart[0] + 3, ipart[1] + 2)));

        Vec4F d = cubic4(fpart[0],
            Vec4F(srcImage.clamp(ipart[0], ipart[1] + 3)),
            Vec4F(srcImage.clamp(ipart[0] + 1, ipart[1] + 3)),
            Vec4F(srcImage.clamp(ipart[0] + 2, ipart[1] + 3)),
            Vec4F(srcImage.clamp(ipart[0] + 3, ipart[1] + 3)));

        auto result = cubic4(fpart[1], a, b, c, d);

        destImage.set({x, y}, Vec4B(
            clamp(result[0], 0.0f, 255.0f),
            clamp(result[1], 0.0f, 255.0f),
            clamp(result[2], 0.0f, 255.0f),
            clamp(result[3], 0.0f, 255.0f)
          ));
      }
    }

    return destImage;
  } else {
    return srcImage;
  }
}

StringList colorDirectivesFromConfig(JsonArray const& directives) {
  List<String> result;

  for (auto entry : directives) {
    if (entry.type() == Json::Type::String) {
      result.append(entry.toString());
    } else if (entry.type() == Json::Type::Object) {
      result.append(paletteSwapDirectivesFromConfig(entry));
    } else {
      throw StarException("Malformed color directives list.");
    }
  }
  return result;
}

String paletteSwapDirectivesFromConfig(Json const& swaps) {
  ColorReplaceImageOperation paletteSwaps;
  for (auto const& swap : swaps.iterateObject()) {
    Vec4B hexColour = Color::fromHex(swap.first).toRgba();
    Vec5B replaceColour = Vec5B(hexColour[0], hexColour[1], hexColour[2], hexColour[3], 0);
    paletteSwaps.colorReplaceMap[replaceColour] = Color::fromHex(swap.second.toString()).toRgba();
  }
  return "?" + imageOperationToString(paletteSwaps);
}

HueShiftImageOperation HueShiftImageOperation::hueShiftDegrees(float degrees) {
  return HueShiftImageOperation{degrees / 360.0f};
}

SaturationShiftImageOperation SaturationShiftImageOperation::saturationShift100(float amount) {
  return SaturationShiftImageOperation{amount / 100.0f};
}

BrightnessMultiplyImageOperation BrightnessMultiplyImageOperation::brightnessMultiply100(float amount) {
  return BrightnessMultiplyImageOperation{amount / 100.0f + 1.0f};
}

FadeToColorImageOperation::FadeToColorImageOperation(Vec3B color, float amount) {
  this->color = color;
  this->amount = amount;

  auto fcl = Color::rgb(color).toLinear();
  for (int i = 0; i <= 255; ++i) {
    auto r = Color::rgb(Vec3B(i, i, i)).toLinear().mix(fcl, amount).toSRGB().toRgb();
    rTable[i] = r[0];
    gTable[i] = r[1];
    bTable[i] = r[2];
  }
}

ImageOperation imageOperationFromString(StringView string) {
  try {
    std::string_view view = string.utf8(); // A view into the string containing all the image operations to be compiled.
    // «Bits» are the `;`/`=`-separated parts of the operation, where the first «bit» is the operation's name and the rest are arguments.
    auto firstBitEnd = view.find_first_of("=;"); // End of the operation's name.
    // Kae's optimised parser for `?replace` operations.
    if (view.substr(0, firstBitEnd).compare("replace") == 0 && (firstBitEnd + 1) != view.size()) {
      ColorReplaceImageOperation operation; // The operation to be returned.

      std::string_view bits = view.substr(firstBitEnd + 1); // The part of the string containing all the hex colour directives.
      operation.colorReplaceMap.reserve(bits.size() / 8); // Reserving enough space for the result.

      char const* hexPtr = nullptr; // Pointer to the current hex color string.
      unsigned int hexLen = 0; // Length of the current hex color string.

      char const* ptr = bits.data(); // Pointer to the current byte of the bits string.
      char const* end = ptr + bits.size(); // Pointer to the end of the bits string.

      char a[5]{}, b[4]{}; // `a` and `b` are the hex color strings being parsed. Hack: `a` has an extra byte for storing whether this colour was subject to any substitution hacks below.
      bool which = true; // `which` is whether we are currently parsing `a` (`true`) or `b` (`false`).

      while (true) { // Loop through each byte of the bits string.
        char ch = *ptr; // Get the current byte as a char.

        if (ch == '=' || ch == ';' || ptr == end) { // If we reached the end of a hex color directive or the end of the bits string,
          if (hexLen != 0) { // If we have a complete hex color directive,
            char* c = which ? a : b; // Get the correct array to store the color in.

            if (hexLen == 3) { // If the hex color string is 3 characters long, it's an `RGB` hex string, so expand it to 6 by doubling each character, then to 8 by adding an alpha of `ff` (fully opaque).
              nibbleDecode(hexPtr, 3, c, 4); // Decodes into {0x0H, 0x0H, 0x0H, 0x00}, where H is each hex character in order.
              c[0] |= (c[0] << 4); // Red. Shift the least significant bits left by 4 bits, copying them into the empty most significant 4 bits.
              c[1] |= (c[1] << 4); // Blue. Ditto.
              c[2] |= (c[2] << 4); // Green. Ditto.
              c[3] = 255; // Add an alpha of `ff` (fully opaque).
              if (which) c[4] = 0; // Substitution hack marker.
            }
            else if (hexLen == 4) { // If the hex color string is 4 characters long, it's an `RGBA` hex string, so expand it to 8 by doubling each character.
              nibbleDecode(hexPtr, 4, c, 4); // Decodes into {0x0H, 0x0H, 0x0H, 0x0H}, where H is each hex character in order.
              c[0] |= (c[0] << 4); // Red. Shift the least significant bits left by 4 bits, copying them into the empty most significant 4 bits.
              c[1] |= (c[1] << 4); // Blue. Ditto.
              c[2] |= (c[2] << 4); // Green. Ditto.
              c[3] |= (c[3] << 4); // Alpha. Ditto.
              if (which) c[4] = 0; // Substitution hack marker.
            }
            else if (hexLen == 6) { // If the hex color string is 6 characters long, it's an `RRGGBB` hex string, so expand it to 8 by adding an alpha of `ff` (fully opaque).
              hexDecode(hexPtr, 6, c, 4); // Decodes into the first three bytes of the array as hex bytes equivalent to their string representation.
              c[3] = 255; // Add an alpha of `ff` (fully opaque).
            #if defined STAR_CROSS_COMPILE
              // FezzedOne: Warning: Disgusting hack for MinGW builds! This makes sure generated sleeves are rendered properly. To bypass this hack, tack an `ff` alpha value onto the end of `bcbc5d` when using
              // it as an `a` colour. The hack replaces an `a` of `bcbc5d` (not `bcbc5dff`) with `bcbc5e`, which is visually nearly indistinguishable anyway.
              // The hack is needed because `scaleBilinear` (way up above) now works very slightly differently from the vanilla version, just enough to impact this one edge case.
              #define COLOUR_NEEDS_SUB(bytes, castType) (bytes[0] == (castType)0xbc && bytes[1] == (castType)0xbc && bytes[2] == (castType)0x5d) // Check if this colour is the one needing substitution.
              #define COLOUR_NEEDS_SUB_RGBA(bytes, castType) (bytes[0] == (castType)0xbc && bytes[1] == (castType)0xbc && bytes[2] == (castType)0x5d && bytes[3] == (castType)0xff) // Same, but for RGBA.
              #define SUBBED_COLOUR Vec5B(0xbc, 0xbc, 0x5e, 0xff, 0xff) // The substituted colour, for lookups.
              #define OLD_COLOUR_BYTE_B (char)0x5d // The byte to replace with...
              #define NEW_COLOUR_BYTE_B (char)0x5e // this byte.

              #define COLOUR_2_NEEDS_SUB(bytes, castType) (bytes[0] == (castType)0xad && bytes[1] == (castType)0x9b && bytes[2] == (castType)0x5a) // Check if this colour is the one needing substitution.
              #define COLOUR_2_NEEDS_SUB_RGBA(bytes, castType) (bytes[0] == (castType)0xad && bytes[1] == (castType)0x9b && bytes[2] == (castType)0x5a && bytes[3] == (castType)0xff) // Same, but for RGBA.
              #define SUBBED_COLOUR_2 Vec5B(0xae, 0x9c, 0x5a, 0xff, 0xff) // The substituted colour, for lookups.
              #define OLD_COLOUR_BYTE_R (char)0xad // The first byte to replace with...
              #define NEW_COLOUR_BYTE_R (char)0xae // this byte.
              #define OLD_COLOUR_BYTE_G (char)0x9b // The second byte to replace with...
              #define NEW_COLOUR_BYTE_G (char)0x9c // this byte.

              bool colourNeedsSub = COLOUR_NEEDS_SUB(c, char); // Micro-optimisation.
              bool colour2NeedsSub = COLOUR_2_NEEDS_SUB(c, char); // Micro-optimisation.
              if (which) c[4] = (colourNeedsSub || colour2NeedsSub) ? (char)255 : 0; // Mark the presence of the following hack for later.

              c[0] = (which && colour2NeedsSub) ? NEW_COLOUR_BYTE_R : c[0]; // Substitute `ad` with `ae` if we're in an `a` of `ad9b5a`.
              c[1] = (which && colour2NeedsSub) ? NEW_COLOUR_BYTE_G : c[1]; // Substitute `9b` with `9c` if we're in an `a` of `ad9b5a`.
              c[2] = (which && colourNeedsSub) ? NEW_COLOUR_BYTE_B : c[2]; // Substitute `5d` with `5e` if we're in an `a` of `bcbc5d`.
            #else
              if (which) c[4] = 0; // Substitution hack marker. Unused on MSVC builds.
            #endif
            }
            else if (hexLen == 8) { // If the hex color string is 8 characters long, it's a full `RRGGBBAA` hex string.
              hexDecode(hexPtr, 8, c, 4); // Decodes into all four bytes of the array as hex bytes equivalent to their string representation.
              if (which) c[4] = 0; // Substitution hack marker.
            }
            else if (!which || (ptr != end && ++ptr != end)) // If we're in `b` of `a=b` and the hex string is of the wrong length, throw an exception.
              throw ImageOperationException(strf("Improper size for hex string '{}' in imageOperationFromString", StringView(hexPtr, hexLen)), false);
            else // We're in `a` of `a=b`. In vanilla, only `a=b` pairs are evaluated, so only throw an exception if `b` is also there.
              return move(operation); // Return the incomplete operation, whose parsing stopped just before the invalid `a`, so no valid replacements after that part will be executed.

            which = !which; // If we parsed a hex colour code, switch `which` to the other colour. I.e., from `a` to `b` and vice versa.
            if (which)
              operation.colorReplaceMap[*(Vec5B*)&a] = *(Vec4B*)&b; // Add the parsed colours to the operation.

            hexLen = 0; // Reset hexLen so we can parse the next hex colour string.
          }
        }
        else if (!hexLen++) // If the current byte is not a separator, increment hexLen.
          hexPtr = ptr; // Set hexPtr to the current byte.

        if (ptr++ == end)
          break; // If we reached the end of the bits string, break out of the loop.
      }

      return operation;
    }


    // Code for non-`?replace` operations.
    List<StringView> bits;

    string.forEachSplitAnyView("=;", [&](StringView split, size_t, size_t) {
      if (!split.empty())
        bits.emplace_back(split);
    });

    String type = bits.at(0);

    if (type == "hueshift") {
      return HueShiftImageOperation::hueShiftDegrees(lexicalCast<float>(bits.at(1)));

    } else if (type == "saturation") {
      return SaturationShiftImageOperation::saturationShift100(lexicalCast<float>(bits.at(1)));

    } else if (type == "brightness") {
      return BrightnessMultiplyImageOperation::brightnessMultiply100(lexicalCast<float>(bits.at(1)));

    } else if (type == "fade") {
      return FadeToColorImageOperation(Color::fromHex(bits.at(1)).toRgb(), lexicalCast<float>(bits.at(2)));

    } else if (type == "scanlines") {
      return ScanLinesImageOperation{
          FadeToColorImageOperation(Color::fromHex(bits.at(1)).toRgb(), lexicalCast<float>(bits.at(2))),
          FadeToColorImageOperation(Color::fromHex(bits.at(3)).toRgb(), lexicalCast<float>(bits.at(4)))};

    } else if (type == "setcolor") {
      return SetColorImageOperation{Color::fromHex(bits.at(1)).toRgb()};

    // } else if (type == "replace") {
    //   // The old `?replace` parser. Now never gets called.
    //   ColorReplaceImageOperation operation;
    //   for (size_t i = 0; i < (bits.size() - 1) / 2; ++i)
    //     operation.colorReplaceMap[Color::hexToVec4B(bits[i * 2 + 1])] = Color::hexToVec4B(bits[i * 2 + 2]);

    //   return operation;

    } else if (type == "addmask" || type == "submask") {
      AlphaMaskImageOperation operation;
      if (type == "addmask")
        operation.mode = AlphaMaskImageOperation::Additive;
      else
        operation.mode = AlphaMaskImageOperation::Subtractive;

      operation.maskImages = String(bits.at(1)).split('+');

      if (bits.size() > 2)
        operation.offset[0] = lexicalCast<int>(bits.at(2));

      if (bits.size() > 3)
        operation.offset[1] = lexicalCast<int>(bits.at(3));

      return operation;

    } else if (type == "blendmult" || type == "blendscreen") {
      BlendImageOperation operation;

      if (type == "blendmult")
        operation.mode = BlendImageOperation::Multiply;
      else
        operation.mode = BlendImageOperation::Screen;

      operation.blendImages = String(bits.at(1)).split('+');

      if (bits.size() > 2)
        operation.offset[0] = lexicalCast<int>(bits.at(2));

      if (bits.size() > 3)
        operation.offset[1] = lexicalCast<int>(bits.at(3));

      return operation;

    } else if (type == "multiply") {
      return MultiplyImageOperation{Color::fromHex(bits.at(1)).toRgba()};

    } else if (type == "border" || type == "outline") {
      BorderImageOperation operation;
      operation.pixels = lexicalCast<unsigned>(bits.at(1));
      operation.startColor = Color::fromHex(bits.at(2)).toRgba();
      if (bits.size() > 3)
        operation.endColor = Color::fromHex(bits.at(3)).toRgba();
      else
        operation.endColor = operation.startColor;
      operation.outlineOnly = type == "outline";
      operation.includeTransparent = false; // Currently just here for anti-aliased fonts

      return operation;

    } else if (type == "scalenearest" || type == "scalebilinear" || type == "scalebicubic" || type == "scale") {
      Vec2F scale;
      // Check if the operation is an explicit «nearest pixel» operation. If so, it will be ignored by the humanoid scale directive handler.
      bool isNearestPixel = false;
      const String skipArgStr = "skip";
      if (bits.remove(skipArgStr))
        isNearestPixel = true;
      if (bits.size() == 2)
        scale = Vec2F::filled(lexicalCast<float>(bits.at(1)));
      else
        scale = Vec2F(lexicalCast<float>(bits.at(1)), lexicalCast<float>(bits.at(2)));

      ScaleImageOperation::Mode mode;
      if (type == "scalenearest")
        mode = isNearestPixel ? ScaleImageOperation::NearestPixel : ScaleImageOperation::Nearest;
      else if (type == "scalebicubic")
        mode = ScaleImageOperation::Bicubic;
      else
        mode = ScaleImageOperation::Bilinear;

      return ScaleImageOperation{mode, scale, scale};

    } else if (type == "crop") {
      return CropImageOperation{RectI(lexicalCast<float>(bits.at(1)), lexicalCast<float>(bits.at(2)),
          lexicalCast<float>(bits.at(3)), lexicalCast<float>(bits.at(4)))};

    } else if (type == "flipx") {
      return FlipImageOperation{FlipImageOperation::FlipX};

    } else if (type == "flipy") {
      return FlipImageOperation{FlipImageOperation::FlipY};

    } else if (type == "flipxy") {
      return FlipImageOperation{FlipImageOperation::FlipXY};

    } else if (type == "setpixel") {
      SetPixelImageOperation operation;
      operation.pixel[0] = lexicalCast<int>(bits.at(1));
      operation.pixel[1] = lexicalCast<int>(bits.at(2));
      operation.colour = Color::fromHex(bits.at(3)).toRgba();
      return operation;

    } else if (type == "blendpixel") {
      BlendPixelImageOperation operation;
      operation.pixel[0] = lexicalCast<int>(bits.at(1));
      operation.pixel[1] = lexicalCast<int>(bits.at(2));
      operation.colour = Color::fromHex(bits.at(3)).toRgba();
      return operation;

    } else if (type == "copyinto") {
      CopyIntoImageOperation operation;

      operation.image = String(bits.at(1));

      if (bits.size() > 2)
        operation.offset[0] = lexicalCast<int>(bits.at(2));

      if (bits.size() > 3)
        operation.offset[1] = lexicalCast<int>(bits.at(3));

      return operation;

    } else if (type == "drawinto") {
      DrawIntoImageOperation operation;

      operation.image = String(bits.at(1));

      if (bits.size() > 2)
        operation.offset[0] = lexicalCast<int>(bits.at(2));

      if (bits.size() > 3)
        operation.offset[1] = lexicalCast<int>(bits.at(3));

      return operation;

    } else {
      throw ImageOperationException(strf("Could not recognize ImageOperation type {}", type), false);
    }
  } catch (OutOfRangeException const& e) {
    throw ImageOperationException("Error reading ImageOperation", e);
  } catch (BadLexicalCast const& e) {
    throw ImageOperationException("Error reading ImageOperation", e);
  }
}

String imageOperationToString(ImageOperation const& operation) {
  if (auto op = operation.ptr<HueShiftImageOperation>()) {
    return strf("hueshift={}", op->hueShiftAmount * 360.0f);
  } else if (auto op = operation.ptr<SaturationShiftImageOperation>()) {
    return strf("saturation={}", op->saturationShiftAmount * 100.0f);
  } else if (auto op = operation.ptr<BrightnessMultiplyImageOperation>()) {
    return strf("brightness={}", (op->brightnessMultiply - 1.0f) * 100.0f);
  } else if (auto op = operation.ptr<FadeToColorImageOperation>()) {
    return strf("fade={}={}", Color::rgb(op->color).toHex(), op->amount);
  } else if (auto op = operation.ptr<ScanLinesImageOperation>()) {
    return strf("scanlines={}={}={}={}",
        Color::rgb(op->fade1.color).toHex(),
        op->fade1.amount,
        Color::rgb(op->fade2.color).toHex(),
        op->fade2.amount);
  } else if (auto op = operation.ptr<SetColorImageOperation>()) {
    return strf("setcolor={}", Color::rgb(op->color).toHex());
  } else if (auto op = operation.ptr<ColorReplaceImageOperation>()) {
    String str = "replace";
    for (auto const& [a, b] : op->colorReplaceMap) {
      // FezzedOne: Transparently convert compiled colour replacements back to the original directives,
      // as if the replacement never happened.
      Vec4B adjustedColour{a[0], a[1], a[2], a[3]};
    #if defined STAR_CROSS_COMPILE
      char colourSubstitutionMode = a[4];
      if (colourSubstitutionMode == (char)255 && adjustedColour[0] == NEW_COLOUR_BYTE_R) {
        adjustedColour[0] = OLD_COLOUR_BYTE_R;
      }
      if (colourSubstitutionMode == (char)255 && adjustedColour[1] == NEW_COLOUR_BYTE_G) {
        adjustedColour[1] = OLD_COLOUR_BYTE_G;
      }
      if (colourSubstitutionMode == (char)255 && adjustedColour[2] == NEW_COLOUR_BYTE_B) {
        adjustedColour[2] = OLD_COLOUR_BYTE_B;
      }
    #endif

      String aStr = Color::rgba(adjustedColour).toHex();
    
    #if defined STAR_CROSS_COMPILE
      if (colourSubstitutionMode == (char)0 && (COLOUR_NEEDS_SUB_RGBA(adjustedColour, unsigned char) || COLOUR_2_NEEDS_SUB_RGBA(adjustedColour, unsigned char))) {
        aStr += "ff";
      }
    #endif

      str += strf(";{}={}", aStr, Color::rgba(b).toHex());
    }
    return str;
  } else if (auto op = operation.ptr<AlphaMaskImageOperation>()) {
    if (op->mode == AlphaMaskImageOperation::Additive)
      return strf("addmask={};{};{}", op->maskImages.join("+"), op->offset[0], op->offset[1]);
    else if (op->mode == AlphaMaskImageOperation::Subtractive)
      return strf("submask={};{};{}", op->maskImages.join("+"), op->offset[0], op->offset[1]);
  } else if (auto op = operation.ptr<BlendImageOperation>()) {
    if (op->mode == BlendImageOperation::Multiply)
      return strf("blendmult={};{};{}", op->blendImages.join("+"), op->offset[0], op->offset[1]);
    else if (op->mode == BlendImageOperation::Screen)
      return strf("blendscreen={};{};{}", op->blendImages.join("+"), op->offset[0], op->offset[1]);
  } else if (auto op = operation.ptr<MultiplyImageOperation>()) {
    return strf("multiply={}", Color::rgba(op->color).toHex());
  } else if (auto op = operation.ptr<BorderImageOperation>()) {
    if (op->outlineOnly)
      return strf("outline={};{};{}", op->pixels, Color::rgba(op->startColor).toHex(), Color::rgba(op->endColor).toHex());
    else
      return strf("border={};{};{}", op->pixels, Color::rgba(op->startColor).toHex(), Color::rgba(op->endColor).toHex());
  } else if (auto op = operation.ptr<ScaleImageOperation>()) {
    if (op->mode == ScaleImageOperation::Nearest)
      return strf("scalenearest={}", op->rawScale);
    // FezzedOne: Faithfully translate explicit nearest-pixel ops with their `skip` argument.
    else if (op->mode == ScaleImageOperation::NearestPixel)
      return strf("scalenearest={};skip", op->rawScale);
    else if (op->mode == ScaleImageOperation::Bilinear)
      return strf("scalebilinear={}", op->rawScale);
    else if (op->mode == ScaleImageOperation::Bicubic)
      return strf("scalebicubic={}", op->rawScale);
  } else if (auto op = operation.ptr<CropImageOperation>()) {
    return strf("crop={};{};{};{}", op->subset.xMin(), op->subset.xMax(), op->subset.yMin(), op->subset.yMax());
  } else if (auto op = operation.ptr<FlipImageOperation>()) {
    if (op->mode == FlipImageOperation::FlipX)
      return "flipx";
    else if (op->mode == FlipImageOperation::FlipY)
      return "flipy";
    else if (op->mode == FlipImageOperation::FlipXY)
      return "flipxy";
  } else if (auto op = operation.ptr<SetPixelImageOperation>()) {
    return strf("setpixel={};{};{}", op->pixel[0], op->pixel[1], Color::rgba(op->colour).toHex());
  } else if (auto op = operation.ptr<BlendPixelImageOperation>()) {
    return strf("blendpixel={};{};{}", op->pixel[0], op->pixel[1], Color::rgba(op->colour).toHex());
  } else if (auto op = operation.ptr<CopyIntoImageOperation>()) {
    return strf("copyinto={};{};{}", op->image, op->offset[0], op->offset[1]);
  } else if (auto op = operation.ptr<DrawIntoImageOperation>()) {
    return strf("drawinto={};{};{}", op->image, op->offset[0], op->offset[1]);
  }

  return "";
}

void parseImageOperations(StringView params, function<void(ImageOperation&&)> outputter) {
  params.forEachSplitView("?", [&](StringView op, size_t, size_t) {
    if (!op.empty())
      outputter(imageOperationFromString(op));
  });
}

List<ImageOperation> parseImageOperations(StringView params) {
  List<ImageOperation> operations;
  params.forEachSplitView("?", [&](StringView op, size_t, size_t) {
    if (!op.empty())
      operations.append(imageOperationFromString(op));
    });

  return operations;
}

String printImageOperations(List<ImageOperation> const& list) {
  String opsToPrint = StringList(list.transformed(imageOperationToString)).join("?");
  if (opsToPrint.empty()) // FezzedOne: Make sure image operations always get a `?` prefix.
    return "";
  else
    return "?" + opsToPrint;
}

void addImageOperationReferences(ImageOperation const& operation, StringList& out) {
  if (auto op = operation.ptr<AlphaMaskImageOperation>())
    out.appendAll(op->maskImages);
  else if (auto op = operation.ptr<BlendImageOperation>())
    out.appendAll(op->blendImages);
  else if (auto op = operation.ptr<CopyIntoImageOperation>())
    out.append(op->image);
  else if (auto op = operation.ptr<DrawIntoImageOperation>())
    out.append(op->image);
}

StringList imageOperationReferences(List<ImageOperation> const& operations) {
  StringList references;
  for (auto const& operation : operations)
    addImageOperationReferences(operation, references);
  return references;
}

void processImageOperation(ImageOperation const& operation, Image& image, ImageReferenceCallback refCallback) {
  if (auto op = operation.ptr<HueShiftImageOperation>()) {
    image.forEachPixel([&op](unsigned, unsigned, Vec4B& pixel) {
      if (pixel[3] != 0)
        pixel = Color::hueShiftVec4B(pixel, op->hueShiftAmount);
    });
  } else if (auto op = operation.ptr<SaturationShiftImageOperation>()) {
    image.forEachPixel([&op](unsigned, unsigned, Vec4B& pixel) {
      if (pixel[3] != 0) {
        Color color = Color::rgba(pixel);
        color.setSaturation(clamp(color.saturation() + op->saturationShiftAmount, 0.0f, 1.0f));
        pixel = color.toRgba();
      }
    });
  } else if (auto op = operation.ptr<BrightnessMultiplyImageOperation>()) {
    image.forEachPixel([&op](unsigned, unsigned, Vec4B& pixel) {
      if (pixel[3] != 0) {
        Color color = Color::rgba(pixel);
        color.setValue(clamp(color.value() * op->brightnessMultiply, 0.0f, 1.0f));
        pixel = color.toRgba();
      }
    });
  } else if (auto op = operation.ptr<FadeToColorImageOperation>()) {
    image.forEachPixel([&op](unsigned, unsigned, Vec4B& pixel) {
      pixel[0] = op->rTable[pixel[0]];
      pixel[1] = op->gTable[pixel[1]];
      pixel[2] = op->bTable[pixel[2]];
    });
  } else if (auto op = operation.ptr<ScanLinesImageOperation>()) {
    image.forEachPixel([&op](unsigned, unsigned y, Vec4B& pixel) {
      if (y % 2 == 0) {
        pixel[0] = op->fade1.rTable[pixel[0]];
        pixel[1] = op->fade1.gTable[pixel[1]];
        pixel[2] = op->fade1.bTable[pixel[2]];
      } else {
        pixel[0] = op->fade2.rTable[pixel[0]];
        pixel[1] = op->fade2.gTable[pixel[1]];
        pixel[2] = op->fade2.bTable[pixel[2]];
      }
    });
  } else if (auto op = operation.ptr<SetColorImageOperation>()) {
    image.forEachPixel([&op](unsigned, unsigned, Vec4B& pixel) {
      pixel[0] = op->color[0];
      pixel[1] = op->color[1];
      pixel[2] = op->color[2];
    });
  } else if (auto op = operation.ptr<ColorReplaceImageOperation>()) {
    image.forEachPixel([&op](unsigned, unsigned, Vec4B& pixel) {
      if (auto m = op->colorReplaceMap.maybe(Vec5B(pixel[0], pixel[1], pixel[2], pixel[3], 0))) {
        // MinGW builds: An explicit `bcbc5e`/`bcbc5eff`/`bcbc5dff` or `ae9c5a`/`ae9c5aff`/`ad9b5aff` replacement takes precedence above anything else.
        // Linux/GCC and MSVC builds: No extra substitutions required.
        pixel = *m; return; 
      }
    #if defined STAR_CROSS_COMPILE
      else if (auto m = op->colorReplaceMap.maybe(Vec5B(pixel[0], pixel[1], pixel[2], pixel[3], 255))) {
        // Execute any tagged `bcbc5d` → `bcbc5eff` replacement if no preceding explicit replacement for `bcbc5e`/`bcbc5eff` is found.
        // MinGW builds: Also execute any tagged `ad9b5a` → `ae9c5aff` replacement if no preceding explicit replacement for `ae9c5a`/`ae9c5aff` is found.
        pixel = *m; return; 
      } else if (COLOUR_NEEDS_SUB_RGBA(pixel, unsigned char)) {
        if (auto m = op->colorReplaceMap.maybe(SUBBED_COLOUR)) {
          // MinGW builds:Execute any tagged `bcbc5d` → `bcbc5eff` replacement if no preceding explicit replacement for `bcbc5e`/`bcbc5eff` is found.
          pixel = *m; return;
        }
      } else if (COLOUR_2_NEEDS_SUB_RGBA(pixel, unsigned char)) {
        if (auto m = op->colorReplaceMap.maybe(SUBBED_COLOUR_2)) {
          // MinGW builds: Additionally execute any tagged `ad9b5a` → `ad9b5aff` replacement if no preceding explicit replacement for `ad9b5a`/`ad9b5aff` is found.
          pixel = *m; return;
        }
      }
    #endif
    });

  } else if (auto op = operation.ptr<AlphaMaskImageOperation>()) {
    if (op->maskImages.empty())
      return;

    if (!refCallback)
      throw StarException("Missing image ref callback during AlphaMaskImageOperation in ImageProcessor::process");

    List<Image const*> maskImages;
    for (auto const& reference : op->maskImages)
      maskImages.append(refCallback(reference));

    image.forEachPixel([&op, &maskImages](unsigned x, unsigned y, Vec4B& pixel) {
      uint8_t maskAlpha = 0;
      Vec2U pos = Vec2U(Vec2I(x, y) + op->offset);
      for (auto mask : maskImages) {
        if (pos[0] < mask->width() && pos[1] < mask->height()) {
          if (op->mode == AlphaMaskImageOperation::Additive) {
            // We produce our mask alpha from the maximum alpha of any of
            // the mask images.
            maskAlpha = std::max(maskAlpha, mask->get(pos)[3]);
          } else if (op->mode == AlphaMaskImageOperation::Subtractive) {
            // We produce our mask alpha from the minimum alpha of any of
            // the mask images.
            maskAlpha = std::min(maskAlpha, mask->get(pos)[3]);
          }
        }
      }
      pixel[3] = std::min(pixel[3], maskAlpha);
    });

  } else if (auto op = operation.ptr<BlendImageOperation>()) {
    if (op->blendImages.empty())
      return;

    if (!refCallback)
      throw StarException("Missing image ref callback during BlendImageOperation in ImageProcessor::process");

    List<Image const*> blendImages;
    for (auto const& reference : op->blendImages)
      blendImages.append(refCallback(reference));

    image.forEachPixel([&op, &blendImages](unsigned x, unsigned y, Vec4B& pixel) {
      Vec2U pos = Vec2U(Vec2I(x, y) + op->offset);
      Vec4F fpixel = Color::v4bToFloat(pixel);
      for (auto blend : blendImages) {
        if (pos[0] < blend->width() && pos[1] < blend->height()) {
          Vec4F blendPixel = Color::v4bToFloat(blend->get(pos));
          if (op->mode == BlendImageOperation::Multiply)
            fpixel = fpixel.piecewiseMultiply(blendPixel);
          else if (op->mode == BlendImageOperation::Screen)
            fpixel = Vec4F::filled(1.0f) - (Vec4F::filled(1.0f) - fpixel).piecewiseMultiply(Vec4F::filled(1.0f) - blendPixel);
        }
      }
      pixel = Color::v4fToByte(fpixel);
    });

  } else if (auto op = operation.ptr<MultiplyImageOperation>()) {
    image.forEachPixel([&op](unsigned, unsigned, Vec4B& pixel) {
      pixel = pixel.combine(op->color, [](uint8_t a, uint8_t b) -> uint8_t {
          return (uint8_t)(((int)a * (int)b) / 255);
        });
    });

  } else if (auto op = operation.ptr<BorderImageOperation>()) {
    Image borderImage(image.size() + Vec2U::filled(op->pixels * 2), PixelFormat::RGBA32);
    borderImage.copyInto(Vec2U::filled(op->pixels), image);
    Vec2I borderImageSize = Vec2I(borderImage.size());

    borderImage.forEachPixel([&op, &image, &borderImageSize](int x, int y, Vec4B& pixel) {
      // FezzedOne: Fixed potential CPU pegging exploit.
      int pixels = std::clamp(op->pixels, 0u, 128u);
      if (op->pixels != pixels)
        Logger::warn("{}: {} width must be between 0 and 128 inclusive!", op->outlineOnly ? "outline" : "border", op->outlineOnly ? "Outline" : "Border");
      bool includeTransparent = op->includeTransparent;
      if (pixel[3] == 0 || (includeTransparent && pixel[3] != 255)) {
        int dist = std::numeric_limits<int>::max();
        for (int j = -pixels; j < pixels + 1; j++) {
          for (int i = -pixels; i < pixels + 1; i++) {
            if (i + x >= pixels && j + y >= pixels && i + x < borderImageSize[0] - pixels && j + y < borderImageSize[1] - pixels) {
              Vec4B remotePixel = image.get(i + x - pixels, j + y - pixels);
              if (remotePixel[3] != 0) {
                dist = std::min(dist, abs(i) + abs(j));
                if (dist == 1) // Early out, if dist is 1 it ain't getting shorter
                  break;
              }
            }
          }
        }

        if (dist < std::numeric_limits<int>::max()) {
          float percent = (dist - 1) / (2.0f * pixels - 1);
          Color color = Color::rgba(op->startColor).mix(Color::rgba(op->endColor), percent);
          if (pixel[3] != 0) {
            if (op->outlineOnly) {
              float pixelA = byteToFloat(pixel[3]);
              color.setAlphaF((1.0f - pixelA) * fminf(pixelA, 0.5f) * 2.0f);
            }
            else {
              Color pixelF = Color::rgba(pixel);
              float pixelA = pixelF.alphaF(), colorA = color.alphaF();
              colorA += pixelA * (1.0f - colorA);
              pixelF.convertToLinear(); //Mix in linear color space as it is more perceptually accurate
              color.convertToLinear();
              color = color.mix(pixelF, pixelA);
              color.convertToSRGB();
              color.setAlphaF(colorA);
            }
          }
          pixel = color.toRgba();
        }
      } else if (op->outlineOnly) {
        pixel = Vec4B(0, 0, 0, 0);
      }
    });

    image = borderImage;

  } else if (auto op = operation.ptr<ScaleImageOperation>()) {
    if (op->mode == ScaleImageOperation::Bilinear)
      image = scaleBilinear(image, op->scale);
    else if (op->mode == ScaleImageOperation::Bicubic)
      image = scaleBicubic(image, op->scale);
    else // FezzedOne: It's either `Nearest` or `NearestPixel`, which should be treated the same here.
      image = scaleNearest(image, op->scale);

  } else if (auto op = operation.ptr<CropImageOperation>()) {
    auto min = op->subset.min();
    auto size = op->subset.size();
     min[0] =  min[0] < 0 ? 0 :  min[0];  min[1] = min[1]  < 0 ? 0 :  min[1];
    size[0] = size[0] < 0 ? 0 : size[0]; size[1] = size[1] < 0 ? 0 : size[1];
    image = image.subImage(Vec2U(min), Vec2U(size));

  } else if (auto op = operation.ptr<FlipImageOperation>()) {
    if (op->mode == FlipImageOperation::FlipX || op->mode == FlipImageOperation::FlipXY) {
      for (size_t y = 0; y < image.height(); ++y) {
        for (size_t xLeft = 0; xLeft < image.width() / 2; ++xLeft) {
          size_t xRight = image.width() - 1 - xLeft;

          auto left = image.get(xLeft, y);
          auto right = image.get(xRight, y);

          image.set(xLeft, y, right);
          image.set(xRight, y, left);
        }
      }
    }

    if (op->mode == FlipImageOperation::FlipY || op->mode == FlipImageOperation::FlipXY) {
      for (size_t x = 0; x < image.width(); ++x) {
        for (size_t yTop = 0; yTop < image.height() / 2; ++yTop) {
          size_t yBottom = image.height() - 1 - yTop;

          auto top = image.get(x, yTop);
          auto bottom = image.get(x, yBottom);

          image.set(x, yTop, bottom);
          image.set(x, yBottom, top);
        }
      }
    }

  } else if (auto op = operation.ptr<SetPixelImageOperation>()) {
    auto& pixel = op->pixel;
    auto size = image.size();

    if (pixel[0] >= size[0] || pixel[1] >= size[1]) return;

    image.set(pixel, op->colour);

  } else if (auto op = operation.ptr<BlendPixelImageOperation>()) {
    auto& pixel = op->pixel;
    auto size = image.size();

    if (pixel[0] >= size[0] || pixel[1] >= size[1]) return;

    auto dest = image.get(pixel);
    auto& src = op->colour;

    Vec3U destMultiplied = Vec3U(dest[0], dest[1], dest[2]) * dest[3] / 255;
    Vec3U srcMultiplied = Vec3U(src[0], src[1], src[2]) * src[3] / 255;

    // Src over dest alpha composition
    Vec3U over = srcMultiplied + destMultiplied * (255 - src[3]) / 255;
    unsigned alpha = src[3] + dest[3] * (255 - src[3]) / 255;

    image.set(pixel, Vec4B(over[0], over[1], over[2], alpha));

  } else if (auto op = operation.ptr<CopyIntoImageOperation>()) {
    if (!refCallback)
      throw StarException("Missing image ref callback during CopyIntoImageOperation in ImageProcessor::process");

    image.copyInto(op->offset, *refCallback(op->image));

  } else if (auto op = operation.ptr<DrawIntoImageOperation>()) {
    if (!refCallback)
      throw StarException("Missing image ref callback during DrawIntoImageOperation in ImageProcessor::process");

    image.drawInto(op->offset, *refCallback(op->image));
  }
}

Image processImageOperations(List<ImageOperation> const& operations, Image image, ImageReferenceCallback refCallback) {
  for (auto const& operation : operations)
    processImageOperation(operation, image, refCallback);

  return image;
}

}
