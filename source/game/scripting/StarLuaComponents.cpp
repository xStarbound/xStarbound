#include "StarLuaComponents.hpp"
#include "StarUtilityLuaBindings.hpp"
#include "StarRoot.hpp"
#include "StarRootLuaBindings.hpp"
#include "StarAssets.hpp"
#include "StarWorld.hpp"
#include "StarWorldClient.hpp"
#include "StarWorldServer.hpp"

namespace Star {

LuaBaseComponent::LuaBaseComponent() {
  addCallbacks("xsb", LuaBindings::makeXsbCallbacks());
  addCallbacks("sb", LuaBindings::makeUtilityCallbacks());
  addCallbacks("root", LuaBindings::makeRootCallbacks());
  setAutoReInit(true);
}

LuaBaseComponent::~LuaBaseComponent() {}

StringMap<LuaCallbacks> LuaBaseComponent::m_baseCallbacks = {};

StringList const& LuaBaseComponent::scripts() const {
  return m_scripts;
}

void LuaBaseComponent::setScript(String script) {
  setScripts({std::move(script)});
}

void LuaBaseComponent::setScripts(StringList scripts) {
  if (initialized())
    throw LuaComponentException("Cannot call LuaWorldComponent::setScripts when LuaWorldComponent is initialized");

  m_scripts = std::move(scripts);
}

void LuaBaseComponent::addBaseCallbacks(String groupName, LuaCallbacks callbacks) {
  if (!m_baseCallbacks.insert(groupName, callbacks).second)
    m_baseCallbacks.at(groupName) = callbacks;
    // throw LuaComponentException::format("Duplicate base callbacks named '{}' in LuaBaseComponent", groupName);
}

void LuaBaseComponent::addCallbacks(String groupName, LuaCallbacks callbacks) {
  if (!m_callbacks.insert(groupName, callbacks).second) {
    if (m_baseCallbacks.contains(groupName)) {
      return;
    } else { // FezzedOne: Allowed `addCallbacks` to overwrite callbacks.
      LuaBaseComponent::removeCallbacks(groupName);
      m_callbacks.insert(groupName, callbacks); // Need to reinsert callbacks. Whoops!
    }
    // throw LuaComponentException::format("Duplicate callbacks named '{}' in LuaBaseComponent", groupName);
  }

  if (m_context)
    m_context->setCallbacks(groupName, callbacks);
}

bool LuaBaseComponent::removeCallbacks(String const& groupName) {
  if (m_callbacks.remove(groupName)) {
    if (m_context)
      m_context->remove(groupName);
    return true;
  }
  return false;
}

bool LuaBaseComponent::autoReInit() const {
  return (bool)m_reloadTracker;
}

void LuaBaseComponent::setAutoReInit(bool autoReInit) {
  if (autoReInit) {
    m_reloadTracker = make_shared<TrackerListener>();
    Root::singleton().registerReloadListener(m_reloadTracker);
  } else {
    m_reloadTracker.reset();
  }
}

void LuaBaseComponent::setLuaRoot(LuaRootPtr luaRoot) {
  m_luaRoot = std::move(luaRoot);
}

LuaRootPtr const& LuaBaseComponent::luaRoot() {
  return m_luaRoot;
}

bool LuaBaseComponent::init() {
  uninit();

  if (!m_luaRoot)
    return false;

  m_error.reset();
  try {
    m_context = m_luaRoot->createContext(m_scripts);
  } catch (LuaException const& e) {
    Logger::error("Exception while creating lua context for scripts '{}': {}", m_scripts, outputException(e, true));
    m_error = String(printException(e, false));
    m_context.reset();
    return false;
  }
  contextSetup();

  if (m_context->containsPath("init")) {
    try {
      m_context->invokePath("init");
    } catch (LuaException const& e) {
      Logger::error("Exception while calling script init: {}", outputException(e, true));
      m_error = String(printException(e, false));
      m_context.reset();
      return false;
    }
  }

  return true;
}

void LuaBaseComponent::uninit() {
  if (m_context) {
    if (m_context->containsPath("uninit")) {
      try {
        m_context->invokePath("uninit");
      } catch (LuaException const& e) {
        Logger::error("Exception while calling script uninit: {}", outputException(e, true));
        m_error = String(printException(e, false));
      }
    }
    contextShutdown();
    m_context.reset();
  }

  m_error.reset();
}

bool LuaBaseComponent::initialized() const {
  return m_context.isValid();
}

Maybe<String> const& LuaBaseComponent::error() const {
  return m_error;
}

Maybe<LuaContext> const& LuaBaseComponent::context() const {
  return m_context;
}

Maybe<LuaContext>& LuaBaseComponent::context() {
  return m_context;
}

void LuaBaseComponent::contextSetup() {
  m_context->setPath("self", m_context->createTable());

  for (auto const& p : m_callbacks)
    m_context->setCallbacks(p.first, p.second);
}

void LuaBaseComponent::contextShutdown() {
  // FezzedOne: Tell the Lua root to clean up its fucking garbage whenever any script context is shut down.
  if (m_luaRoot) {}
    // m_luaRoot->collectGarbage();
}

void LuaBaseComponent::setError(String error) {
  m_context.reset();
  m_error = std::move(error);
}

bool LuaBaseComponent::checkInitialization() {
  // We should re-initialize if we are either already initialized or in an
  // error state (which means we WERE initialized until we had an error)
  bool shouldBeInitialized = initialized() || error();
  if (shouldBeInitialized && m_reloadTracker && m_reloadTracker->pullTriggered())
    init();
  return initialized();
}

bool LuaBaseComponent::checkIfClient(World* worldPtr) {
  return (bool)as<WorldClient>(worldPtr);
}

}
