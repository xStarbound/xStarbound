#ifndef STAR_LUA_COMPONENT_HPP
#define STAR_LUA_COMPONENT_HPP

#include "StarPeriodic.hpp"
#include "StarLogging.hpp"
#include "StarListener.hpp"
#include "StarWorld.hpp"
#include "StarRoot.hpp"
#include "StarConfiguration.hpp"
#include "StarWorldLuaBindings.hpp"
#include "StarInputLuaBindings.hpp"

#if defined TRACY_ENABLE
  #include "tracy/Tracy.hpp"
#else
  #define ZoneScoped
  #define ZoneScopedN(name)
#endif

namespace Star {

class WorldClient;

STAR_EXCEPTION(LuaComponentException, LuaException);

// Basic lua component that can be initialized (takes and then owns a script
// context, calls the script context's init function) and uninitialized
// (releases the context, calls the context 'uninit' function).
//
// Callbacks can be added and removed whether or not the context is initialized
// or not, they will be added back during a call to init.  'root' callbacks are
// available by default as well as an ephemeral 'self' table.
//
// All script function calls (init / uninit / invoke) guard against missing
// functions.  If the function is missing, it will do nothing and return
// nothing.  If the function exists but throws an error, the error will be
// logged and the component will go into the error state.
//
// Whenever an error is set, all function calls or eval will fail until the
// error is cleared by re-initializing.
//
// If 'autoReInit' is set, Monitors Root for reloads, and if a root reload
// occurs, will automatically (on the next call to invoke) uninit and then
// re-init the script before calling invoke.  'autoReInit' defaults to true.
class LuaBaseComponent {
public:
  LuaBaseComponent();
  // The LuaBaseComponent destructor does NOT call the 'unint' entry point in
  // the script.  In order to do so, uninit() must be called manually before
  // destruction.  This is because during destruction, it is highly likely that
  // callbacks may not be valid, and highly likely that exceptions could be
  // thrown.
  virtual ~LuaBaseComponent();

  LuaBaseComponent(LuaBaseComponent const& component) = delete;
  LuaBaseComponent& operator=(LuaBaseComponent const& component) = delete;

  StringList const& scripts() const;
  void setScript(String script);
  void setScripts(StringList scripts);

  static void addBaseCallbacks(String groupName, LuaCallbacks callbacks);
  void addCallbacks(String groupName, LuaCallbacks callbacks);
  bool removeCallbacks(String const& groupName);

  // If true, component will automatically uninit and re-init when root is
  // reloaded.
  bool autoReInit() const;
  void setAutoReInit(bool autoReInit);

  // Lua components require access to a LuaRoot object to initialize /
  // uninitialize.
  void setLuaRoot(LuaRootPtr luaRoot);
  LuaRootPtr const& luaRoot();

  // init returns true on success, false if there has been an error
  // initializing the script.  LuaRoot must be set before calling or this will
  // always fail.  Calls the 'init' entry point on the script context.
  bool init();
  // uninit will uninitialize the LuaComponent if it is currently initialized.
  // This calls the 'uninit' entry point on the script context before
  // destroying the context.
  void uninit();

  bool initialized() const;

  template <typename Ret = LuaValue, typename... V>
  Maybe<Ret> invoke(String const& name, V&&... args);

  template <typename Ret = LuaValue>
  Maybe<Ret> eval(String const& code);

  // Returns last error, if there has been an error.  Errors can only be
  // cleared by re-initializing the context.
  Maybe<String> const& error() const;

  Maybe<LuaContext> const& context() const;
  Maybe<LuaContext>& context();

protected:
  virtual void contextSetup();
  virtual void contextShutdown();

  void setError(String error);

  // Checks the initialization state of the script, while also reloading the
  // script and clearing the error state if a root reload has occurred.
  bool checkInitialization();
  bool checkIfClient(World* worldPtr);
  static StringMap<LuaCallbacks> m_baseCallbacks;

private:
  StringList m_scripts;
  StringMap<LuaCallbacks> m_callbacks;
  LuaRootPtr m_luaRoot;
  TrackerListenerPtr m_reloadTracker;
  Maybe<LuaContext> m_context;
  Maybe<String> m_error;
};

// Wraps a basic lua component to add a persistent storage table translated
// into JSON that can be stored outside of the script context.
template <typename Base>
class LuaStorableComponent : public Base {
public:
  JsonObject getScriptStorage() const;
  void setScriptStorage(JsonObject storage);

protected:
  virtual void contextSetup() override;
  virtual void contextShutdown() override;

private:
  JsonObject m_storage;
};

// Wraps a basic lua component with an 'update' method and an embedded tick
// rate.  Every call to 'update' here will only call the internal script
// 'update' at the configured delta.  Adds a update tick controls under the
// 'script' callback table.
template <typename Base>
class LuaUpdatableComponent : public Base {
public:
  LuaUpdatableComponent();

  unsigned updateDelta() const;
  float updateDt(float dt) const;
  float updateDt() const;
  void setUpdateDelta(unsigned updateDelta);

  // Returns true if the next update will call the internal script update
  // method.
  bool updateReady() const;

  template <typename Ret = LuaValue, typename... V>
  Maybe<Ret> update(V&&... args);

private:
  Periodic m_updatePeriodic;
  // FezzedOne: Why was this not initialised to zero, Kae?
  mutable float m_lastDt = 0.0f;
};

// Wraps a basic lua component so that world callbacks are added on init, and
// removed on uninit, and sets the world LuaRoot as the LuaBaseComponent
// LuaRoot automatically.
template <typename Base>
class LuaWorldComponent : public Base {
public:
  void init(World* world);
  void uninit();

protected:
  using Base::setLuaRoot;
  using Base::init;
  using Base::m_baseCallbacks;
};

// Component for scripts which can be used as entity message handlers, provides
// a 'message' table with 'setHandler' callback to set message handlers.
template <typename Base>
class LuaMessageHandlingComponent : public Base {
public:
  LuaMessageHandlingComponent();

  Maybe<Json> handleMessage(String const& message, bool localMessage, JsonArray const& args = {});

protected:
  virtual void contextShutdown() override;

private:
  struct MessageHandler {
    Maybe<LuaFunction> function;
    String name;
    bool passName = true;
    bool localOnly = false;
  };

  StringMap<MessageHandler> m_messageHandlers;
};

template <typename Ret, typename... V>
Maybe<Ret> LuaBaseComponent::invoke(String const& name, V&&... args) {
  ZoneScoped;
#ifdef TRACY_ENABLE
  ZoneTextF("Function '%s'", name.utf8().c_str());
#endif

  if (!checkInitialization())
    return {};

  try {
    auto method = m_context->getPath(name);
    if (method == LuaNil)
      return {};
    return m_context->luaTo<LuaFunction>(std::move(method)).invoke<Ret>(std::forward<V>(args)...);
  } catch (LuaException const& e) {
    Logger::error("Exception while invoking lua function '{}'. {}", name, outputException(e, true));
    setError(printException(e, false));
    return {};
  }
}

template <typename Ret>
Maybe<Ret> LuaBaseComponent::eval(String const& code) {
  ZoneScoped;
  if (!checkInitialization())
    return {};

  try {
    return m_context->eval<Ret>(code);
  } catch (LuaException const& e) {
    Logger::error("Exception while evaluating lua in context: {}", outputException(e, true));
    return {};
  }
}

template <typename Base>
JsonObject LuaStorableComponent<Base>::getScriptStorage() const {
  if (Base::initialized())
    return Base::context()->template getPath<JsonObject>("storage");
  else
    return m_storage;
}

template <typename Base>
void LuaStorableComponent<Base>::setScriptStorage(JsonObject storage) {
  if (Base::initialized())
    Base::context()->setPath("storage", std::move(storage));
  else
    m_storage = std::move(storage);
}

template <typename Base>
void LuaStorableComponent<Base>::contextSetup() {
  Base::contextSetup();
  Base::context()->setPath("storage", std::move(m_storage));
}

template <typename Base>
void LuaStorableComponent<Base>::contextShutdown() {
  m_storage = Base::context()->template getPath<JsonObject>("storage");
  Base::contextShutdown();
}

template <typename Base>
LuaUpdatableComponent<Base>::LuaUpdatableComponent() {
  m_updatePeriodic.setStepCount(1);

  LuaCallbacks scriptCallbacks;
  scriptCallbacks.registerCallback("updateDt", [this]() {
      return updateDt();
    });
  scriptCallbacks.registerCallback("setUpdateDelta", [this](unsigned d) {
      setUpdateDelta(d);
    });

  Base::addCallbacks("script", std::move(scriptCallbacks));
}

template <typename Base>
unsigned LuaUpdatableComponent<Base>::updateDelta() const {
  return m_updatePeriodic.stepCount();
}

template <typename Base>
float LuaUpdatableComponent<Base>::updateDt(float dt) const {
  m_lastDt = dt;
  return m_updatePeriodic.stepCount() * dt;
}

template <typename Base>
float LuaUpdatableComponent<Base>::updateDt() const {
  float retDt = m_updatePeriodic.stepCount() * m_lastDt;
  // FezzedOne: Fix for a bug where this callback returns `0.0f` or a negative number when called before the first `update`.
  if (retDt <= 0.0f) {
    return 0.01666666667f * GlobalTimescale;
  } else {
    return retDt;
  }
}


template <typename Base>
void LuaUpdatableComponent<Base>::setUpdateDelta(unsigned updateDelta) {
  m_updatePeriodic.setStepCount(updateDelta);
}

template <typename Base>
bool LuaUpdatableComponent<Base>::updateReady() const {
  return m_updatePeriodic.ready();
}

template <typename Base>
template <typename Ret, typename... V>
Maybe<Ret> LuaUpdatableComponent<Base>::update(V&&... args) {
  if (!m_updatePeriodic.tick())
    return {};

  return Base::template invoke<Ret>("update", std::forward<V>(args)...);
}

template <typename Base>
void LuaWorldComponent<Base>::init(World* world) {
  if (Base::initialized())
    uninit();

  auto config = Root::singleton().configuration();
  // if (config->get("safeScripts").toBool())
  Base::setLuaRoot(make_shared<LuaRoot>());
  // else
  //   Base::setLuaRoot(world->luaRoot());

  auto assets = Root::singleton().assets();

  // FezzedOne: The base callbacks are all client-side. Make sure they're only added client-side.
  // Also make sure to get the appropriate garbage collection settings.
  if (Base::checkIfClient(world)) {
    for (auto const& p : m_baseCallbacks)
      Base::addCallbacks(p.first, p.second);
    
    Json config = assets->json("/client.config");
    Base::luaRoot()->tuneAutoGarbageCollection(config.getFloat("luaGcPause"), config.getFloat("luaGcStepMultiplier"));
  } else {
    Json config = assets->json("/worldserver.config");
    Base::luaRoot()->tuneAutoGarbageCollection(config.getFloat("luaGcPause"), config.getFloat("luaGcStepMultiplier"));
  }

  Base::addCallbacks("world", LuaBindings::makeWorldCallbacks(world));
  Base::init();
}

template <typename Base>
void LuaWorldComponent<Base>::uninit() {
  Base::uninit();
  Base::removeCallbacks("world");
}

template <typename Base>
LuaMessageHandlingComponent<Base>::LuaMessageHandlingComponent() {
  LuaCallbacks scriptCallbacks;
  scriptCallbacks.registerCallback("setHandler", [this](Variant<String, Json> message, Maybe<LuaFunction> handler) {
      MessageHandler handlerInfo = {};

      if (Json* config = message.ptr<Json>()) {
        handlerInfo.passName = config->getBool("passName", false);
        handlerInfo.localOnly = config->getBool("localOnly", false);
        handlerInfo.name = config->getString("name");
      } else {
        handlerInfo.passName = true;
        handlerInfo.localOnly = false;
        handlerInfo.name = message.get<String>();
      }

      if (handler) {
        handlerInfo.function.emplace(handler.take());
        m_messageHandlers.set(handlerInfo.name, handlerInfo);
      }
      else
        m_messageHandlers.remove(handlerInfo.name);
    });

  Base::addCallbacks("message", std::move(scriptCallbacks));
}

template <typename Base>
Maybe<Json> LuaMessageHandlingComponent<Base>::handleMessage(
    String const& message, bool localMessage, JsonArray const& args) {
  if (!Base::initialized())
    return {};

  if (auto handler = m_messageHandlers.ptr(message)) {
    try {
      if (handler->localOnly) {
        if (!localMessage)
          return {};
        else if (handler->passName)
          return handler->function->template invoke<Json>(message, luaUnpack(args));
        else
          return handler->function->template invoke<Json>(luaUnpack(args));
      }
      else if (handler->passName)
        return handler->function->template invoke<Json>(message, localMessage, luaUnpack(args));
      else
        return handler->function->template invoke<Json>(localMessage, luaUnpack(args));
    } catch (LuaException const& e) {
      Logger::error(
          "Exception while invoking lua message handler for message '{}'. {}", message, outputException(e, true));
      Base::setError(String(printException(e, false)));
    }
  }
  return {};
}

template <typename Base>
void LuaMessageHandlingComponent<Base>::contextShutdown() {
  m_messageHandlers.clear();
  Base::contextShutdown();
}
}

#endif
