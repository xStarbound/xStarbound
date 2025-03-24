#ifndef STAR_IMAGE_STRETCH_WIDGET_HPP
#define STAR_IMAGE_STRETCH_WIDGET_HPP

#include "StarWidget.hpp"

namespace Star {

STAR_CLASS(ImageStretchWidget);

class ImageStretchWidget : public Widget {
public:
  ImageStretchWidget(ImageStretchSet const& imageStretchSet, GuiDirection direction);
  virtual ~ImageStretchWidget() {}

  // @KrashV: Added the ability to change the images of an image stretch widget.
  void setImageStretchSet(String const& beginImage, String const& innerImage, String const& endImage);

protected:
  virtual void renderImpl();

private:
  ImageStretchSet m_imageStretchSet;
  GuiDirection m_direction;
};

}

#endif
