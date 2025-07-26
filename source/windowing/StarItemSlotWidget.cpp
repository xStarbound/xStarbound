#include "StarItemSlotWidget.hpp"
#include "StarArmors.hpp"
#include "StarAssets.hpp"
#include "StarDurabilityItem.hpp"
#include "StarGameTypes.hpp"
#include "StarImageMetadataDatabase.hpp"
#include "StarItem.hpp"
#include "StarJsonExtra.hpp"
#include "StarRoot.hpp"
#include "StarWidgetParsing.hpp"

namespace Star {

// From grbr404 and Novaenia.
static String formatShortSize(uint64_t n) {
  if (n < 10000)
    return toString(n);

  uint64_t divisor = 1000ull;
  char suffix = 'k';

  if (n >= 1000000000000000000ull) {
    divisor = 1000000000000000000ull;
    suffix = 'Q';
  } else if (n >= 1000000000000000ull) {
    divisor = 1000000000000000ull;
    suffix = 'q';
  } else if (n >= 1000000000000ull) {
    divisor = 1000000000000ull;
    suffix = 't';
  } else if (n >= 1000000000ull) {
    divisor = 1000000000ull;
    suffix = 'b';
  } else if (n >= 1000000ull) {
    divisor = 1000000ull;
    suffix = 'm';
  }

  uint64_t whole = n / divisor;
  if (whole >= 100ull)
    return strf("{}{:c}", whole, suffix);

  uint64_t remainder = n - (whole * divisor);
  uint64_t frac = (remainder / (divisor / 1000));

  if (frac == 0)
    return strf("{}{:c}", whole, suffix);
  else if (whole >= 10)
    return strf("{}.{}{:c}", whole, frac / 100, suffix);
  else
    return strf("{}.{:02d}{:c}", whole, frac / 10, suffix);
}

ItemSlotWidget::ItemSlotWidget(ItemPtr const& item, String const& backingImage)
    : m_item(item), m_backingImage(backingImage) {
  m_drawBackingImageWhenFull = false;
  m_drawBackingImageWhenEmpty = true;
  m_fontSize = 0;
  m_progress = 1;

  auto assets = Root::singleton().assets();
  auto interfaceConfig = assets->json("/interface.config");
  m_countPosition = TextPositioning(jsonToVec2F(interfaceConfig.get("itemCountRightAnchor")), HorizontalAnchor::RightAnchor);
  m_countFontMode = FontMode::Shadow;
  m_fontSize = interfaceConfig.query("font.itemSize").toInt();
  m_font = interfaceConfig.query("font.defaultFont").toString();
  m_fontColor = Color::rgb(jsonToVec3B(interfaceConfig.query("font.defaultColor")));
  m_fontEscapes = interfaceConfig.query("font.escapeCodes", "?border=1;111a;1114").toString();
  m_itemDraggableArea = jsonToRectI(interfaceConfig.get("itemDraggableArea"));
  m_durabilityOffset = jsonToVec2I(interfaceConfig.get("itemIconDurabilityOffset"));

  auto newItemIndicatorConfig = interfaceConfig.get("newItemAnimation");
  m_newItemIndicator = Animation(newItemIndicatorConfig);
  // End animation before it begins, only display when triggered
  m_newItemIndicator.update(newItemIndicatorConfig.getDouble("animationCycle") * newItemIndicatorConfig.getDouble("loops", 1.0f));

  Json highlightAnimationConfig = interfaceConfig.get("highlightAnimation");
  m_highlightAnimation = Animation(highlightAnimationConfig);
  m_highlightEnabled = false;
  m_cosmeticHighlightEnabled = false;

  m_underlaidMarkerDirectives = interfaceConfig.optString("underlaidMarkerDirectives").value("?replace;000f=b33f");
  m_cosmeticStackFontColor = Color::rgb(jsonToVec3B(interfaceConfig.query("font.cosmeticStackColor", JsonArray{210, 50, 50})));

  Vec2I backingImageSize;
  if (m_backingImage.size()) {
    auto imgMetadata = Root::singleton().imageMetadataDatabase();
    backingImageSize = Vec2I(imgMetadata->imageSize(m_backingImage));
  }
  setSize(m_itemDraggableArea.max().piecewiseMax(backingImageSize));

  WidgetParser parser;

  parser.construct(assets->json("/interface/itemSlot.config").get("config"), this);
  m_durabilityBar = fetchChild<ProgressWidget>("durabilityBar");
  m_durabilityBar->hide();
  m_showDurability = false;
  m_showCount = true;
  m_showSingleCountOnStackables = false;
  m_showRarity = true;
  m_showLinkIndicator = false;

  disableScissoring();
}

void ItemSlotWidget::update(float dt) {
  if (m_item)
    m_newItemIndicator.update(dt);
  if (m_highlightEnabled)
    m_highlightAnimation.update(dt);
  Widget::update(dt);
}

bool ItemSlotWidget::sendEvent(InputEvent const& event) {
  if (m_visible) {
    if (auto mouseButton = event.ptr<MouseButtonDownEvent>()) {
      if (mouseButton->mouseButton == MouseButton::Left
        || (m_rightClickCallback && mouseButton->mouseButton == MouseButton::Right)
        || (m_middleClickCallback && mouseButton->mouseButton == MouseButton::Middle)) {
        Vec2I mousePos = *context()->mousePosition(event);
        RectI itemArea = m_itemDraggableArea.translated(screenPosition());
        if (itemArea.contains(mousePos)) {
          if (mouseButton->mouseButton == MouseButton::Right)
            m_rightClickCallback(this);
          else if (mouseButton->mouseButton == MouseButton::Middle)
            m_middleClickCallback(this);
          else
            m_callback(this);
          return true;
        }
      }
    }
  }

  return false;
}

void ItemSlotWidget::setCallback(WidgetCallbackFunc callback) {
  m_callback = callback;
}

void ItemSlotWidget::setRightClickCallback(WidgetCallbackFunc callback) {
  m_rightClickCallback = callback;
}

void ItemSlotWidget::setMiddleClickCallback(WidgetCallbackFunc callback) {
  m_middleClickCallback = callback;
}

void ItemSlotWidget::setItem(ItemPtr const& item) {
  m_item = item;
}

ItemPtr ItemSlotWidget::item() const {
  return m_item;
}

void ItemSlotWidget::setProgress(float progress) {
  m_progress = progress;
}

void ItemSlotWidget::setBackingImageAffinity(bool full, bool empty) {
  m_drawBackingImageWhenFull = full;
  m_drawBackingImageWhenEmpty = empty;
}

void ItemSlotWidget::setCountPosition(TextPositioning textPositioning) {
  m_countPosition = textPositioning;
}

void ItemSlotWidget::setCountFontMode(FontMode fontMode) {
  m_countFontMode = fontMode;
}

void ItemSlotWidget::showDurability(bool show) {
  m_showDurability = show;
}

void ItemSlotWidget::showCount(bool show) {
  m_showCount = show;
}

void ItemSlotWidget::showSingleCountOnStackables(bool show) {
  m_showSingleCountOnStackables = show;
}

void ItemSlotWidget::showRarity(bool showRarity) {
  m_showRarity = showRarity;
}

void ItemSlotWidget::showLinkIndicator(bool showLinkIndicator) {
  m_showLinkIndicator = showLinkIndicator;
}

void ItemSlotWidget::showSecondaryIcon(bool showSecondaryIcon) {
  m_showSecondaryIcon = showSecondaryIcon;
}

void ItemSlotWidget::indicateNew() {
  m_newItemIndicator.reset();
}

void ItemSlotWidget::setHighlightEnabled(bool highlight) {
  if (!m_highlightEnabled && highlight)
    m_highlightAnimation.reset();
  m_highlightEnabled = highlight;
}

void ItemSlotWidget::setCosmeticHighlightEnabled(bool cosmeticHighlight) {
  m_cosmeticHighlightEnabled = cosmeticHighlight;
}

void ItemSlotWidget::renderImpl() {
  if (m_item) {
    if (m_drawBackingImageWhenFull && m_backingImage != "")
      context()->drawInterfaceQuad(m_backingImage, Vec2F(screenPosition()));

    List<Drawable> iconDrawables = m_showSecondaryIcon ? m_item->secondaryDrawables().value(m_item->iconDrawables()) : m_item->iconDrawables();

    if (m_showRarity) {
      String border = rarityBorder(m_item->rarity());
      if (auto directives = m_item->borderDirectives()) {
        border += *directives;
      } else if (auto armourItem = as<ArmorItem>(m_item)) {
        if (armourItem->isUnderlaid())
          border += m_underlaidMarkerDirectives;
      }
      context()->drawInterfaceQuad(border, Vec2F(screenPosition()));
    }

    if (m_showLinkIndicator) {
      // TODO: Hardcoded
      context()->drawInterfaceQuad(String("/interface/inventory/itemlinkindicator.png"), Vec2F(screenPosition() - Vec2I(1, 1)));
    }

    for (auto i : iconDrawables)
      context()->drawInterfaceDrawable(i, Vec2F(screenPosition() + size() / 2));

    if (!m_newItemIndicator.isComplete())
      context()->drawInterfaceDrawable(m_newItemIndicator.drawable(1.0), Vec2F(screenPosition() + size() / 2), Color::White.toRgba());

    if (m_showDurability) {
      if (auto durabilityItem = as<DurabilityItem>(m_item)) {
        float amount = durabilityItem->durabilityStatus();
        m_durabilityBar->setCurrentProgressLevel(amount);

        if (amount < 1)
          m_durabilityBar->show();
        else
          m_durabilityBar->hide();
      } else {
        m_durabilityBar->hide();
      }
    }

    int frame = (int)roundf(m_progress * 18); // TODO: Hardcoded lol
    context()->drawInterfaceQuad(String(strf("/interface/cooldown.png:{}", frame)), Vec2F(screenPosition()));

    if (m_showCount) {
      if (m_item->maxStack() == 1) {
        if (auto countString = m_item->countString()) { // FezzedOne: Useful for ammo counters and the like.
          context()->setFont(m_font);
          context()->setFontSize(m_fontSize);
          context()->setFontColor(m_fontColor.toRgba());
          context()->setFontMode(m_countFontMode);
          context()->renderInterfaceText(*countString, m_countPosition.translated(Vec2F(screenPosition())));
          context()->setFontMode(FontMode::Normal);
          context()->setDefaultFont();
        } else if (auto armourItem = as<ArmorItem>(m_item)) {
          constexpr size_t maxCosmeticStack = 99;
          size_t cosmeticStackCount = armourItem->cosmeticStackCount();
          if (cosmeticStackCount != 0 || m_cosmeticHighlightEnabled) {
            context()->setFont(m_font);
            context()->setFontSize(m_fontSize);
            context()->setFontColor(m_cosmeticStackFontColor.toRgba());
            context()->setFontMode(m_countFontMode);
            String plusSign = (m_cosmeticHighlightEnabled && cosmeticStackCount < maxCosmeticStack) ? "+" : "";
            context()->renderInterfaceText(cosmeticStackCount == 0 ? plusSign : toString(cosmeticStackCount + 1) + plusSign,
                m_countPosition.translated(Vec2F(screenPosition())));
            context()->setFontMode(FontMode::Normal);
            context()->setDefaultFont();
          }
        }
      } else if (m_item->maxStack() > 1 && (m_showSingleCountOnStackables || m_item->count() > 1)) {
        // FezzedOne: Stackable items always show their stack size in inventory slots.
        context()->setFont(m_font);
        context()->setFontSize(m_fontSize);
        context()->setFontColor(m_fontColor.toRgba());
        context()->setFontMode(m_countFontMode);
        // grbr404/Novaenia: Use better stack size formatting for huge stack sizes.
        context()->renderInterfaceText(toString(formatShortSize(m_item->count())), m_countPosition.translated(Vec2F(screenPosition())));
        context()->setFontMode(FontMode::Normal);
        context()->setDefaultFont();
      }
    }

  } else if (m_drawBackingImageWhenEmpty && m_backingImage != "") {
    context()->drawInterfaceQuad(m_backingImage, Vec2F(screenPosition()));
    int frame = (int)roundf(m_progress * 18); // TODO: Hardcoded lol
    context()->drawInterfaceQuad(String(strf("/interface/cooldown.png:{}", frame)), Vec2F(screenPosition()));
  }

  if (m_highlightEnabled) {
    context()->drawInterfaceDrawable(m_highlightAnimation.drawable(1.0), Vec2F(screenPosition() + size() / 2), Color::White.toRgba());
  }

  if (!m_item)
    m_durabilityBar->hide();
}

} // namespace Star
