#include "StarWorldCamera.hpp"

namespace Star {

static bool isFuckedNumber(float valueToCheck) {
  volatile uint32_t bits; // FezzedOne: Needs to be volatile to force the sanity check, because compilers are smart.
  std::memcpy((void*)&bits, &valueToCheck, sizeof(float));
  constexpr uint32_t EXP_MASK = 0x7F800000;
  // constexpr uint32_t MANT_MASK = 0x007FFFFF;

  if ((bits & EXP_MASK) == EXP_MASK) // && (bits & MANT_MASK) != 0
    return true;
  return false;
}

void WorldCamera::setCenterWorldPosition(Vec2F position, bool force) {
  // FezzedOne: Fixes a potential out-of-bounds crash caused by the Lua intepreter somehow being able to generate NaNs from
  // things like division by zero even with `-ffast-math` / `/fp:fast` enabled. And also because `math.huge` exists in Lua and
  // can fuck with world wrapping checks.
  if (isFuckedNumber(position[0]) || isFuckedNumber(position[1])) return;

  m_rawWorldCenter = position;
  // Only actually move the world center if a half pixel distance has been
  // moved in any direction.  This is sort of arbitrary, but helps prevent
  // judder if the camera is at a boundary and floating point inaccuracy is
  // causing the focus to jitter back and forth across the boundary.
  if (fabs(position[0] - m_worldCenter[0]) < 1.0f / (TilePixels * m_pixelRatio * 2) && fabs(position[1] - m_worldCenter[1]) < 1.0f / (TilePixels * m_pixelRatio * 2) && !force)
    return;

  // First, make sure the camera center position is inside the main x
  // coordinate bounds, and that the top and bottom of the screen are not
  // outside of the y coordinate bounds.
  m_worldCenter = m_worldGeometry.xwrap(position);
  m_worldCenter[1] = clamp(m_worldCenter[1],
      (float)m_screenSize[1] / (TilePixels * m_pixelRatio * 2),
      m_worldGeometry.height() - (float)m_screenSize[1] / (TilePixels * m_pixelRatio * 2));

  // Then, position the camera center position so that the tile grid is as
  // close as possible aligned to whole pixel boundaries.  This is incredibly
  // important, because this means that even without any complicated rounding,
  // elements drawn in world space that are aligned with TilePixels will
  // eventually also be aligned to real screen pixels.

  float ratio = TilePixels * m_pixelRatio;

  if (m_screenSize[0] % 2 == 0)
    m_worldCenter[0] = round(m_worldCenter[0] * ratio) / ratio;
  else
    m_worldCenter[0] = (round(m_worldCenter[0] * ratio + 0.5f) - 0.5f) / ratio;

  if (m_screenSize[1] % 2 == 0)
    m_worldCenter[1] = round(m_worldCenter[1] * ratio) / ratio;
  else
    m_worldCenter[1] = (round(m_worldCenter[1] * ratio + 0.5f) - 0.5f) / ratio;
}

} // namespace Star
