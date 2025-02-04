#include "StarFont.hpp"
#include "StarFile.hpp"
#include "StarFormat.hpp"

#include <freetype/freetype.h>

namespace Star {

constexpr int FontLoadFlags = FT_LOAD_FORCE_AUTOHINT;

struct FTContext {
  FT_Library library;

  FTContext() {
    library = nullptr;
    if (FT_Init_FreeType(&library))
      throw FontException("Could not initialize freetype library.");
  }

  ~FTContext() {
    if (library) {
      FT_Done_FreeType(library);
      library = nullptr;
    }
  }
};

FTContext ftContext;

struct FontImpl {
  FT_Face face;
};

FontPtr Font::loadTrueTypeFont(String const& fileName, unsigned pixelSize) {
  return loadTrueTypeFont(make_shared<ByteArray>(File::readFile(fileName)), pixelSize);
}

FontPtr Font::loadTrueTypeFont(ByteArrayConstPtr const& bytes, unsigned pixelSize) {
  FontPtr font = make_shared<Font>();
  font->m_fontBuffer = bytes;

  shared_ptr<FontImpl> fontImpl = make_shared<FontImpl>();
  if (FT_New_Memory_Face(
          ftContext.library, (FT_Byte const*)font->m_fontBuffer->ptr(), font->m_fontBuffer->size(), 0, &fontImpl->face))
    throw FontException("Could not load font from buffer");

  font->m_fontImpl = fontImpl;
  font->setPixelSize(pixelSize);

  return font;
}

// Downstreamed @bmdhacks's lazy-loading code. Helps with memory usage on the client a bit.
void Font::lazyLoadFont() {
  if (!this->m_fontImpl) {
    shared_ptr<FontImpl> fontImpl = make_shared<FontImpl>();
    if (FT_New_Memory_Face(
            ftContext.library, (FT_Byte const*)this->m_fontBuffer->ptr(), this->m_fontBuffer->size(), 0, &fontImpl->face))
      throw FontException("Could not load font from buffer");

    this->m_fontImpl = fontImpl;
  }
}

Font::Font() : m_pixelSize(0), m_alphaThreshold(0) {}

Font::~Font() {
  // Downstreamed oSB bugfix for a potential memory leak here.
  FT_Done_Face(m_fontImpl->face);
}

FontPtr Font::clone() const {
  return Font::loadTrueTypeFont(m_fontBuffer, m_pixelSize);
}

void Font::setPixelSize(unsigned pixelSize) {
  if (pixelSize == 0) {
    pixelSize = 1;
  }

  if (m_pixelSize == pixelSize)
    return;

  if (m_fontImpl && FT_Set_Pixel_Sizes(m_fontImpl->face, pixelSize, 0))
    throw FontException(strf("Cannot set font pixel size to: {}", pixelSize));
  m_pixelSize = pixelSize;
}

void Font::setAlphaThreshold(uint8_t alphaThreshold) {
  m_alphaThreshold = alphaThreshold;
}

unsigned Font::height() const {
  return m_pixelSize;
}

unsigned Font::width(String::Char c) {
  if (auto width = m_widthCache.maybe({c, m_pixelSize})) {
    return *width;
  } else {
    if (!m_fontBuffer)
      throw FontException("Font::width called on uninitialized font.");

    lazyLoadFont();

    FT_Load_Char(m_fontImpl->face, c, FontLoadFlags);
    unsigned newWidth = (m_fontImpl->face->glyph->linearHoriAdvance + 32768) / 65536;
    m_widthCache.insert({c, m_pixelSize}, newWidth);
    return newWidth;
  }
}

bool Font::exists(String::Char c) {
  if (!m_fontBuffer)
    throw FontException("Font::exists called on uninitialized font.");

  lazyLoadFont();

  return (bool)FT_Get_Char_Index(m_fontImpl->face, c);
}

std::pair<Image, Vec2I> Font::render(String::Char c) {
  if (!m_fontBuffer)
    throw FontException("Font::render called on uninitialized font.");

  lazyLoadFont();

  if (!m_fontImpl)
    throw FontException("Font::render called on uninitialized font.");

  FT_Face face = m_fontImpl->face;
  FT_UInt glyph_index = FT_Get_Char_Index(face, c);
  if (FT_Load_Glyph(face, glyph_index, FontLoadFlags) != 0)
    return {};

  /* convert to an anti-aliased bitmap */
  if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL) != 0)
    return {};

  FT_GlyphSlot slot = face->glyph;

  unsigned width = slot->bitmap.width;
  unsigned height = slot->bitmap.rows;

  Image image(width + 2, height + 2, PixelFormat::RGBA32);
  Vec4B white(255, 255, 255, 0);
  image.fill(white);

  for (unsigned y = 0; y != height; ++y) {
    uint8_t* p = slot->bitmap.buffer + y * slot->bitmap.pitch;
    for (unsigned x = 0; x != width; ++x) {
      if (x >= 0 && y >= 0 && x < width && y < height) {
        uint8_t value = *(p + x);
        if (m_alphaThreshold) {
          if (value >= m_alphaThreshold) {
            white[3] = 255;
            image.set(x + 1, height - y, white);
          }
        } else if (value) {
          white[3] = value;
          image.set(x + 1, height - y, white);
        }
      }
    }
  }

  return { std::move(image), {slot->bitmap_left - 1, (slot->bitmap_top - height) + (m_pixelSize / 4) - 1} };
}

}
