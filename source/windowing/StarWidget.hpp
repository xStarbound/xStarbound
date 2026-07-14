#ifndef STAR_WIDGET_HPP
#define STAR_WIDGET_HPP

#include "StarCasting.hpp"
#include "StarGuiContext.hpp"
#include "StarInputEvent.hpp"
#include "StarLua.hpp"
#include "StarText.hpp"
#include "StarVector.hpp"

namespace Star {

STAR_EXCEPTION(GuiException, StarException);

STAR_CLASS(Widget);
STAR_CLASS(Pane);

enum class KeyboardCaptureMode {
  None,
  KeyEvents,
  TextInput
};

typedef function<void(Widget*)> WidgetCallbackFunc;

// FezzedOne: Wrapper class for pointers passed into Lua binding constructors.
// Throws instead of dereferencing when asked to dereference a null pointer.
// Used to wrap around all `fetchChild` calls because I don't want to have to
// rewrite them individually. This wrapper is needed because of `removeChild` and
// `removeAllChildren` callbacks and some widgets not having `if (widget)` checks
// in C++.
template <typename ObjectType>
class SafeWidgetPtr {
public:
  // FezzedOne: Internal shared pointer to an object.
  std::shared_ptr<ObjectType> m_ptr;

  SafeWidgetPtr(std::shared_ptr<ObjectType> objectPointer /*, size_t uniqueId */) {
    m_ptr = objectPointer;
  }


  SafeWidgetPtr(ObjectType* objectPointer) {
    m_ptr = objectPointer;
  }

  SafeWidgetPtr() : SafeWidgetPtr(std::shared_ptr<ObjectType>()) {}

  SafeWidgetPtr(std::nullptr_t) : m_ptr((ObjectType*)nullptr) {}

  SafeWidgetPtr(SafeWidgetPtr const& r) : SafeWidgetPtr(r.m_ptr /*, r.m_uniqueId */) {}

  SafeWidgetPtr(SafeWidgetPtr&& r) {
    m_ptr = std::move(r.m_ptr);
    r.m_ptr = std::shared_ptr<ObjectType>();
  }

  SafeWidgetPtr& operator=(SafeWidgetPtr const& r) {
    m_ptr = r.m_ptr;
    return *this;
  }

  SafeWidgetPtr& operator=(SafeWidgetPtr&& r) {
    m_ptr = std::move(r.m_ptr);
    r.m_ptr = std::shared_ptr<ObjectType>();
    return *this;
  }

  // Just as an extra sanity check on assignment to member variables.
  operator std::shared_ptr<ObjectType>() const {
    if (m_ptr.get())
      return m_ptr;
    throw GuiException("Dereferenced pointer to nonexistent widget");
  }

  operator bool() const {
    return (bool)m_ptr.get();
  }

  ObjectType& operator*() const {
    if (auto ptr = m_ptr.get())
      return *ptr;
    throw GuiException("Dereferenced pointer to nonexistent widget");
  }

  ObjectType* operator->() const {
    if (auto ptr = m_ptr.get())
      return ptr;
    throw GuiException("Dereferenced pointer to nonexistent widget");
  }

  ObjectType* get() const {
    if (auto ptr = m_ptr.get())
      return ptr;
    throw GuiException("Dereferenced pointer to nonexistent widget");
  }

  void checkValid() const {
    if (!m_ptr.get())
      throw GuiException("Dereferenced pointer to nonexistent widget");
  }
};

// FezzedOne: Needed for dynamic casting of `SafeWidgetPtr`'s.
template <typename Type1, typename Type2>
SafeWidgetPtr<Type1> as(SafeWidgetPtr<Type2> const& p) {
  return SafeWidgetPtr(dynamic_pointer_cast<Type1>(p.m_ptr));
}

template <typename Type1, typename Type2>
SafeWidgetPtr<Type1 const> as(SafeWidgetPtr<Type2 const> const& p) {
  return SafeWidgetPtr(dynamic_pointer_cast<Type1 const>(p.m_ptr));
}

class Widget {
public:
  Widget();
  virtual ~Widget();

  Widget(Widget const& copy) = delete;
  Widget& operator=(Widget const&) = delete;

  virtual void render(RectI const& region) final;
  virtual void update(float dt);

  GuiContext* context() const;

  // Position of widget with drawing offset (useful for drawing)
  virtual Vec2I position() const;
  // Position of widget ignoring offset (useful for moving and placing widgets
  // relative to its current position)
  virtual Vec2I relativePosition() const;
  // Set position of widget, ignoring offset
  virtual void setPosition(Vec2I const& position);

  virtual Vec2I drawingOffset() const;
  virtual void setDrawingOffset(Vec2I const& offset);

  virtual Vec2I size() const;
  virtual void setSize(Vec2I const& size);

  virtual RectI relativeBoundRect() const;
  virtual RectI screenBoundRect() const;

  virtual bool inMember(Vec2I const& position) const;

  virtual bool sendEvent(InputEvent const& event);

  virtual void show();
  virtual void hide();
  virtual bool visibility() const;
  virtual void toggleVisibility();
  virtual void setVisibility(bool visibility);

  virtual void mouseOver();
  virtual void mouseOut();
  virtual void mouseReturnStillDown();
  virtual void setMouseTransparent(bool transparent);
  virtual bool mouseTransparent();

  virtual bool active() const;

  virtual bool interactive() const;

  virtual bool hasFocus() const;
  virtual void focus();
  virtual void blur();

  virtual Widget* parent() const;
  virtual void setParent(Widget* parent);

  virtual Pane const* window() const;
  virtual Pane* window();

  virtual void addChild(String const& name, WidgetPtr member);
  virtual void addChildAt(String const& name, WidgetPtr member, size_t at);
  virtual bool removeChild(Widget* member);
  virtual bool removeChild(String const& name);
  virtual bool removeChildAt(size_t at);
  virtual WidgetPtr getChildAt(Vec2I const& pos);
  virtual bool containsChild(String const& name);
  virtual WidgetPtr fetchChild(String const& name);
  template <typename WidgetType>
  SafeWidgetPtr<WidgetType> fetchChild(String const& name);

  virtual WidgetPtr findChild(String const& name);
  template <typename WidgetType>
  SafeWidgetPtr<WidgetType> findChild(String const& name);

  WidgetPtr childPtr(Widget const* widget) const;

  virtual size_t numChildren() const;
  virtual WidgetPtr getChildNum(size_t num) const;
  template <typename WidgetType>
  SafeWidgetPtr<WidgetType> getChildNum(size_t num) const;
  virtual void removeAllChildren();

  virtual String const& name() const;
  virtual void setName(String const& name);
  String fullName() const;

  unsigned windowHeight() const;
  unsigned windowWidth() const;
  Vec2I windowSize() const;
  virtual Vec2I screenPosition() const;
  void disableScissoring();
  void enableScissoring();
  void determineSizeFromChildren();
  void markAsContainer();

  virtual KeyboardCaptureMode keyboardCaptured() const;

  void setData(Json const& data);
  Json const& data();

  bool setLabel(String const& name, String const& value);

protected:
  friend std::ostream& operator<<(std::ostream& os, Widget const& widget);
  String toStringImpl(int indentLevel) const;

  virtual void renderImpl();
  virtual void drawChildren();
  bool setupDrawRegion(RectI const& region);
  virtual RectI getScissorRect() const;
  virtual RectI noScissor() const;

  Widget* m_parent;

  GuiContext* m_context;

  bool m_visible;
  PolyF m_boundPoly;

  Vec2I m_position;
  Vec2I m_size;
  RectI m_drawingArea;
  Vec2I m_drawingOffset;
  String m_name;
  List<WidgetPtr> m_members;
  StringMap<WidgetPtr> m_memberHash;
  Vec2I m_memberSize;
  bool m_focus;
  bool m_doScissor;
  bool m_container;
  bool m_mouseTransparent;

  Json m_data;
};

std::ostream& operator<<(std::ostream& os, Widget const& widget);

template <typename WidgetType>
SafeWidgetPtr<WidgetType> Widget::getChildNum(size_t num) const {
  return SafeWidgetPtr(as<WidgetType>(getChildNum(num)));
}

template <typename WidgetType>
SafeWidgetPtr<WidgetType> Widget::fetchChild(String const& name) {
  return SafeWidgetPtr(as<WidgetType>(fetchChild(name)));
}

template <typename WidgetType>
SafeWidgetPtr<WidgetType> Widget::findChild(String const& name) {
  return SafeWidgetPtr(as<WidgetType>(findChild(name)));
}

} // namespace Star

template <>
struct fmt::formatter<Star::Widget> : ostream_formatter {};

#endif
