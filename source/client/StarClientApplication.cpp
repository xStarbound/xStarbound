#include "StarClientApplication.hpp"
#include "StarAssets.hpp"
#include "StarConfiguration.hpp"
#include "StarCurve25519.hpp"
#include "StarEncode.hpp"
#include "StarFile.hpp"
#include "StarInput.hpp"
#include "StarJsonExtra.hpp"
#include "StarLogging.hpp"
#include "StarPlayer.hpp"
#include "StarPlayerLog.hpp"
#include "StarPlayerStorage.hpp"
#include "StarRoot.hpp"
#include "StarRootLoader.hpp"
#include "StarVersion.hpp"
#include "StarVoice.hpp"
#include "StarWorldClient.hpp"
#include "StarWorldTemplate.hpp"

#include "StarInputLuaBindings.hpp"
#include "StarInterfaceLuaBindings.hpp"
#include "StarVoiceLuaBindings.hpp"

// Include Tracy here to measure frame times.
#if defined TRACY_ENABLE
#include "tracy/Tracy.hpp"
#else
#define ZoneScoped
#define ZoneScopedN(name)
#endif

namespace Star {

Json const AdditionalAssetsSettings = Json::parseJson(R"JSON(
    {
      "missingImage" : "/assetmissing.png",
      "missingAudio" : "/assetmissing.wav"
    }
  )JSON");

Json const AdditionalDefaultConfiguration = Json::parseJson(R"JSON(
    {
      "configurationVersion" : {
        "client" : 8
      },

      "allowAssetsMismatch" : false,
      "vsync" : true,
      "limitTextureAtlasSize" : false,
      "useMultiTexturing" : true,
      "audioChannelSeparation" : [-25, 25],

      "assetLuaGcPause" : 1.2,
      "assetLuaGcStepMultiplier" : 1.2,

      "sfxVol" : 100,
      "musicVol" : 70,
      "instrumentVol" : 100,
      "windowedResolution" : [1000, 600],
      "fullscreenResolution" : [1920, 1080],
      "fullscreen" : false,
      "borderless" : false,
      "maximized" : true,
      "zoomLevel" : 3.0,
      "interfaceScale": 3.0,
      "speechBubbles" : true,
      "playerHeadRotation" : true,

      "title" : {
        "multiPlayerAddress" : "",
        "multiPlayerPort" : "",
        "multiPlayerAccount" : ""
      },

      "bindings" : {
        "PlayerUp" :  [ { "type" : "key", "value" : "W", "mods" : [] } ],
        "PlayerDown" :  [ { "type" : "key", "value" : "S", "mods" : [] } ],
        "PlayerLeft" :  [ { "type" : "key", "value" : "A", "mods" : [] } ],
        "PlayerRight" :  [ { "type" : "key", "value" : "D", "mods" : [] } ],
        "PlayerJump" :  [ { "type" : "key", "value" : "Space", "mods" : [] } ],
        "PlayerDropItem" :  [ { "type" : "key", "value" : "Q", "mods" : [] } ],
        "PlayerInteract" :  [ { "type" : "key", "value" : "E", "mods" : [] } ],
        "PlayerShifting" :  [ { "type" : "key", "value" : "RShift", "mods" : [] }, { "type" : "key", "value" : "LShift", "mods" : [] } ],
        "PlayerTechAction1" :  [ { "type" : "key", "value" : "F", "mods" : [] } ],
        "PlayerTechAction2" :  [],
        "PlayerTechAction3" :  [],
        "EmoteBlabbering" :  [ { "type" : "key", "value" : "Right", "mods" : ["LCtrl", "LShift"] } ],
        "EmoteShouting" :  [ { "type" : "key", "value" : "Up", "mods" : ["LCtrl", "LAlt"] } ],
        "EmoteHappy" :  [ { "type" : "key", "value" : "Up", "mods" : [] } ],
        "EmoteSad" :  [ { "type" : "key", "value" : "Down", "mods" : [] } ],
        "EmoteNeutral" :  [ { "type" : "key", "value" : "Left", "mods" : [] } ],
        "EmoteLaugh" :  [ { "type" : "key", "value" : "Left", "mods" : [ "LCtrl" ] } ],
        "EmoteAnnoyed" :  [ { "type" : "key", "value" : "Right", "mods" : [] } ],
        "EmoteOh" :  [ { "type" : "key", "value" : "Right", "mods" : [ "LCtrl" ] } ],
        "EmoteOooh" :  [ { "type" : "key", "value" : "Down", "mods" : [ "LCtrl" ] } ],
        "EmoteBlink" :  [ { "type" : "key", "value" : "Up", "mods" : [ "LCtrl" ] } ],
        "EmoteWink" :  [ { "type" : "key", "value" : "Up", "mods" : ["LCtrl", "LShift"] } ],
        "EmoteEat" :  [ { "type" : "key", "value" : "Down", "mods" : ["LCtrl", "LShift"] } ],
        "EmoteSleep" :  [ { "type" : "key", "value" : "Left", "mods" : ["LCtrl", "LShift"] } ],
        "ShowLabels" :  [ { "type" : "key", "value" : "RAlt", "mods" : [] }, { "type" : "key", "value" : "LAlt", "mods" : [] } ],
        "CameraShift" :  [ { "type" : "key", "value" : "RCtrl", "mods" : [] }, { "type" : "key", "value" : "LCtrl", "mods" : [] } ],
        "TitleBack" :  [ { "type" : "key", "value" : "Esc", "mods" : [] } ],
        "CinematicSkip" :  [ { "type" : "key", "value" : "Esc", "mods" : [] } ],
        "CinematicNext" :  [ { "type" : "key", "value" : "Right", "mods" : [] }, { "type" : "key", "value" : "Return", "mods" : [] } ],
        "GuiClose" :  [ { "type" : "key", "value" : "Esc", "mods" : [] } ],
        "GuiShifting" :  [ { "type" : "key", "value" : "RShift", "mods" : [] }, { "type" : "key", "value" : "LShift", "mods" : [] } ],
        "KeybindingCancel" :  [ { "type" : "key", "value" : "Esc", "mods" : [] } ],
        "KeybindingClear" :  [ { "type" : "key", "value" : "Del", "mods" : [] }, { "type" : "key", "value" : "Backspace", "mods" : [] } ],
        "ChatPageUp" :  [ { "type" : "key", "value" : "PageUp", "mods" : [] } ],
        "ChatPageDown" :  [ { "type" : "key", "value" : "PageDown", "mods" : [] } ],
        "ChatPreviousLine" :  [ { "type" : "key", "value" : "Up", "mods" : [] } ],
        "ChatNextLine" :  [ { "type" : "key", "value" : "Down", "mods" : [] } ],
        "ChatSendLine" :  [ { "type" : "key", "value" : "Return", "mods" : [] } ],
        "ChatBegin" :  [ { "type" : "key", "value" : "Return", "mods" : [] } ],
        "ChatBeginCommand" :  [ { "type" : "key", "value" : "/", "mods" : [] } ],
        "ChatStop" :  [ { "type" : "key", "value" : "Esc", "mods" : [] } ],
        "InterfaceHideHud" :  [ { "type" : "key", "value" : "Z", "mods" : [ "LAlt" ] } ],
        "InterfaceChangeBarGroup" :  [ { "type" : "key", "value" : "X", "mods" : [] } ],
        "InterfaceDeselectHands" :  [ { "type" : "key", "value" : "Z", "mods" : [] } ],
        "InterfaceBar1" :  [ { "type" : "key", "value" : "1", "mods" : [] } ],
        "InterfaceBar2" :  [ { "type" : "key", "value" : "2", "mods" : [] } ],
        "InterfaceBar3" :  [ { "type" : "key", "value" : "3", "mods" : [] } ],
        "InterfaceBar4" :  [ { "type" : "key", "value" : "4", "mods" : [] } ],
        "InterfaceBar5" :  [ { "type" : "key", "value" : "5", "mods" : [] } ],
        "InterfaceBar6" :  [ { "type" : "key", "value" : "6", "mods" : [] } ],
        "InterfaceBar7" :  [],
        "InterfaceBar8" :  [],
        "InterfaceBar9" :  [],
        "InterfaceBar10" :  [],
        "EssentialBar1" :  [ { "type" : "key", "value" : "R", "mods" : [] } ],
        "EssentialBar2" :  [ { "type" : "key", "value" : "T", "mods" : [] } ],
        "EssentialBar3" :  [ { "type" : "key", "value" : "Y", "mods" : [] } ],
        "EssentialBar4" :  [ { "type" : "key", "value" : "N", "mods" : [] } ],
        "InterfaceRepeatCommand" :  [ { "type" : "key", "value" : "P", "mods" : [] } ],
        "InterfaceToggleFullscreen" :  [ { "type" : "key", "value" : "F11", "mods" : [] } ],
        "InterfaceReload" :  [ ],
        "InterfaceEscapeMenu" :  [ { "type" : "key", "value" : "Esc", "mods" : [] } ],
        "InterfaceInventory" :  [ { "type" : "key", "value" : "I", "mods" : [] } ],
        "InterfaceCodex" :  [ { "type" : "key", "value" : "L", "mods" : [] } ],
        "InterfaceQuest" :  [ { "type" : "key", "value" : "J", "mods" : [] } ],
        "InterfaceCrafting" :  [ { "type" : "key", "value" : "C", "mods" : [] } ]
      }
    }
  )JSON");

void ClientApplication::startup(StringList const& cmdLineArgs) {
  RootLoader rootLoader({AdditionalAssetsSettings, AdditionalDefaultConfiguration, String("xclient.log"), LogLevel::Info, false, String("xclient.config")});
  auto rootAndOptions = rootLoader.initOrDie(cmdLineArgs);
  m_root = std::move(rootAndOptions.first);
  auto& options = rootAndOptions.second;
  // FezzedOne: Skip loading Steam Workshop mods if `-noworkshop` is passed.
  if (options.switches.contains("noworkshop"))
    m_skipWorkshop = true;

  Logger::info("xClient v{} [Starbound v{}] ({}) // Source ID: {} // Protocol: {}", xSbVersionString, StarVersionString, StarArchitectureString, StarSourceIdentifierString, StarProtocolVersion);
}

void ClientApplication::shutdown() {
  if (m_mainInterface)
    m_mainInterface->clean();

  if (m_universeClient)
    m_universeClient->disconnect();

  if (m_mainInterface)
    m_mainInterface->deregisterPanes(); // FezzedOne: Needed because of `interface.bindRegisteredPane`.
  m_mainInterface.reset();

  if (m_universeServer) {
    m_universeServer->stop();
    m_universeServer->join();
    m_universeServer.reset();
  }

  if (m_statistics) {
    m_statistics->writeStatistics();
    m_statistics.reset();
  }

  m_universeClient.reset();
  m_statistics.reset();
}

void ClientApplication::applicationInit(ApplicationControllerPtr appController) {
  Application::applicationInit(appController);

  auto assets = m_root->assets();
  // m_minInterfaceScale = assets->json("/interface.config:minInterfaceScale").toInt();
  // m_maxInterfaceScale = assets->json("/interface.config:maxInterfaceScale").toInt();
  // m_crossoverRes = jsonToVec2F(assets->json("/interface.config:interfaceCrossoverRes"));

  appController->setCursorVisible(true);

  AudioFormat audioFormat = appController->enableAudio();
  m_mainMixer = make_shared<MainMixer>(audioFormat.sampleRate, audioFormat.channels);
  m_mainMixer->setVolume(0.5);

  m_worldPainter = make_shared<WorldPainter>();
  m_guiContext = make_shared<GuiContext>(m_mainMixer->mixer(), appController);
  m_input = make_shared<Input>();
  m_voice = make_shared<Voice>(appController);

  auto configuration = m_root->configuration();
  bool vsync = configuration->get("vsync").toBool();
  Vec2U windowedSize = jsonToVec2U(configuration->get("windowedResolution"));
  Vec2U fullscreenSize = jsonToVec2U(configuration->get("fullscreenResolution"));
  bool fullscreen = configuration->get("fullscreen").toBool();
  bool borderless = configuration->get("borderless").toBool();
  bool maximized = configuration->get("maximized").toBool();

  float updateRate = 1.0f / GlobalTimestep;
  if (auto jUpdateRate = configuration->get("updateRate")) {
    updateRate = jUpdateRate.toFloat();
    GlobalTimestep = 1.0f / updateRate;
  }

  if (auto jServerUpdateRate = configuration->get("serverUpdateRate"))
    ServerGlobalTimestep = 1.0f / jServerUpdateRate.toFloat();

  if (auto interfaceScale = configuration->get("interfaceScale")) {
  } else
    configuration->set("interfaceScale", 3.0f);

  appController->setTargetUpdateRate(updateRate);
  appController->setApplicationTitle(assets->json("/client.config:windowTitle").toString());
  appController->setVSyncEnabled(vsync);

  if (fullscreen)
    appController->setFullscreenWindow(fullscreenSize);
  else if (borderless)
    appController->setBorderlessWindow();
  else if (maximized)
    appController->setMaximizedWindow();
  else
    appController->setNormalWindow(windowedSize);

  appController->setMaxFrameSkip(assets->json("/client.config:maxFrameSkip").toUInt());
  appController->setUpdateTrackWindow(assets->json("/client.config:updateTrackWindow").toFloat());

  if (auto jVoice = configuration->get("voice"))
    m_voice->loadJson(jVoice.toObject(), true);

  m_voice->init();
  m_voice->setLocalSpeaker(0);
}

void ClientApplication::renderInit(RendererPtr renderer) {
  Application::renderInit(renderer);
  auto assets = m_root->assets();

  auto loadEffectConfig = [&](String const& name) {
    String path = strf("/rendering/effects/{}.config", name);
    if (assets->assetExists(path)) {
      StringMap<String> shaders;
      auto config = assets->json(path);
      auto shaderConfig = config.getObject("effectShaders");
      for (auto& entry : shaderConfig) {
        if (entry.second.isType(Json::Type::String)) {
          String shader = entry.second.toString();
          if (!shader.hasChar('\n')) {
            auto shaderBytes = assets->bytes(AssetPath::relativeTo(path, shader));
            shader = std::string(shaderBytes->ptr(), shaderBytes->size());
          }
          shaders[entry.first] = shader;
        }
      }

      renderer->loadEffectConfig(name, config, shaders);
    } else
      Logger::warn("No rendering config found for renderer with id '{}'", renderer->rendererId());
  };

  renderer->loadConfig(assets->json("/rendering/opengl20.config"));

  loadEffectConfig("world");
  loadEffectConfig("interface");

  if (m_root->configuration()->get("limitTextureAtlasSize").optBool().value(false))
    renderer->setSizeLimitEnabled(true);

  renderer->setMultiTexturingEnabled(m_root->configuration()->get("useMultiTexturing").optBool().value(true));

  m_guiContext->renderInit(renderer);

  m_cinematicOverlay = make_shared<Cinematic>();
  m_errorScreen = make_shared<ErrorScreen>();

  if (m_titleScreen)
    m_titleScreen->renderInit(renderer);
  if (m_worldPainter)
    m_worldPainter->renderInit(renderer);

  changeState(MainAppState::Mods);
}

void ClientApplication::windowChanged(WindowMode windowMode, Vec2U screenSize) {
  auto config = m_root->configuration();
  if (windowMode == WindowMode::Fullscreen) {
    config->set("fullscreenResolution", jsonFromVec2U(screenSize));
    config->set("fullscreen", true);
    config->set("borderless", false);
  } else if (windowMode == WindowMode::Borderless) {
    config->set("borderless", true);
    config->set("fullscreen", false);
  } else if (windowMode == WindowMode::Maximized) {
    config->set("maximized", true);
    config->set("fullscreen", false);
    config->set("borderless", false);
  } else {
    config->set("maximized", false);
    config->set("fullscreen", false);
    config->set("borderless", false);
    config->set("windowedResolution", jsonFromVec2U(screenSize));
  }
}

void ClientApplication::processInput(InputEvent const& event) {
  if (auto keyDown = event.ptr<KeyDownEvent>()) {
    m_heldKeyEvents.append(*keyDown);
    m_edgeKeyEvents.append(*keyDown);
  } else if (auto keyUp = event.ptr<KeyUpEvent>()) {
    eraseWhere(m_heldKeyEvents, [&](auto& keyEvent) {
      return keyEvent.key == keyUp->key;
    });

    Maybe<KeyMod> modKey = KeyModNames.maybeLeft(KeyNames.getRight(keyUp->key));
    if (modKey)
      m_heldKeyEvents.transform([&](auto& keyEvent) {
        return KeyDownEvent{keyEvent.key, keyEvent.mods & ~*modKey};
      });
  }
  /* FezzedOne: Getting rid of this for now. */
  // else if (auto cAxis = event.ptr<ControllerAxisEvent>()) {
  //   if (cAxis->controllerAxis == ControllerAxis::LeftX)
  //     m_controllerLeftStick[0] = cAxis->controllerAxisValue;
  //   else if (cAxis->controllerAxis == ControllerAxis::LeftY)
  //     m_controllerLeftStick[1] = cAxis->controllerAxisValue;
  //   else if (cAxis->controllerAxis == ControllerAxis::RightX)
  //     m_controllerRightStick[0] = cAxis->controllerAxisValue;
  //   else if (cAxis->controllerAxis == ControllerAxis::RightY)
  //     m_controllerRightStick[1] = cAxis->controllerAxisValue;
  // }

  if (!m_errorScreen->accepted() && m_errorScreen->handleInputEvent(event))
    return;

  bool processed = false;

  if (m_state == MainAppState::Splash) {
    processed = m_cinematicOverlay->handleInputEvent(event);
  } else if (m_state == MainAppState::Title) {
    if (!(processed = m_cinematicOverlay->handleInputEvent(event)))
      processed = m_titleScreen->handleInputEvent(event);

  } else if (m_state == MainAppState::SinglePlayer || m_state == MainAppState::MultiPlayer) {
    if (!(processed = m_cinematicOverlay->handleInputEvent(event)))
      processed = m_mainInterface->handleInputEvent(event);
  }

  m_input->handleInput(event, processed);
  WorldCamera& camera = m_worldPainter->camera();

  auto config = m_root->configuration();

  int zoomOffset = 0;

  if (auto presses = m_input->bindDown("xsb", "zoomIn"))
    zoomOffset += (*presses) * 16;
  if (auto presses = m_input->bindDown("xsb", "zoomOut"))
    zoomOffset -= (*presses) * 16;
  if (auto presses = m_input->bindDown("xsb", "incrementalZoomIn"))
    zoomOffset += (*presses);
  if (auto presses = m_input->bindDown("xsb", "incrementalZoomOut"))
    zoomOffset -= (*presses);

  if (zoomOffset != 0)
    config->set("zoomLevel", min(100.0f, max(4.0f, round(config->get("zoomLevel").toFloat() * 16.0f + (float)zoomOffset)) * 0.0625f));

  int interfaceScaleOffset = 0;

  if (auto presses = m_input->bindDown("xsb", "interfaceZoomIn"))
    interfaceScaleOffset += (*presses) * 16;
  if (auto presses = m_input->bindDown("xsb", "interfaceZoomOut"))
    interfaceScaleOffset -= (*presses) * 16;
  if (auto presses = m_input->bindDown("xsb", "interfaceIncrementalZoomIn"))
    interfaceScaleOffset += (*presses);
  if (auto presses = m_input->bindDown("xsb", "interfaceIncrementalZoomOut"))
    interfaceScaleOffset -= (*presses);

  if (interfaceScaleOffset != 0)
    config->set("interfaceScale", min(100.0f, max(8.0f, round(config->get("interfaceScale").toFloat() * 16.0f + (float)interfaceScaleOffset)) * 0.0625f));
}

void ClientApplication::update() {
  ZoneScoped;

  float dt = GlobalTimestep * GlobalTimescale;
  if (m_state >= MainAppState::Title) {
    if (auto p2pNetworkingService = appController()->p2pNetworkingService()) {
      if (auto join = p2pNetworkingService->pullPendingJoin()) {
        m_pendingMultiPlayerConnection = PendingMultiPlayerConnection{join.takeValue(), {}, {}};
        changeState(MainAppState::Title);
      }

      if (auto req = p2pNetworkingService->pullJoinRequest())
        m_mainInterface->queueJoinRequest(*req);

      p2pNetworkingService->update();
    }
  }

  if (!m_errorScreen->accepted())
    m_errorScreen->update(dt);

  if (m_state == MainAppState::Mods)
    updateMods(dt);
  else if (m_state == MainAppState::ModsWarning)
    updateModsWarning(dt);

  if (m_state == MainAppState::Splash)
    updateSplash(dt);
  else if (m_state == MainAppState::Error)
    updateError(dt);
  else if (m_state == MainAppState::Title)
    updateTitle(dt);
  else if (m_state > MainAppState::Title)
    updateRunning(dt);

  // Swallow leftover encoded voice data if we aren't in-game to allow mic read to continue for settings.
  if (m_state <= MainAppState::Title) {
    DataStreamBuffer ext;
    m_voice->send(ext);
  } // TODO: directly disable encoding at menu so we don't have to do this

  m_guiContext->cleanup();
  m_edgeKeyEvents.clear();
  m_input->reset();
}

void ClientApplication::render() {
  auto config = m_root->configuration();
  auto assets = m_root->assets();
  auto& renderer = Application::renderer();

  renderer->switchEffectConfig("interface");

  m_guiContext->setInterfaceScale(clamp(config->get("interfaceScale").toFloat(), 0.5f, 100.0f));

  // if (m_guiContext->windowWidth() >= m_crossoverRes[0] && m_guiContext->windowHeight() >= m_crossoverRes[1])
  //   m_guiContext->setInterfaceScale(m_maxInterfaceScale);
  // else
  //   m_guiContext->setInterfaceScale(m_minInterfaceScale);

  if (m_state == MainAppState::Mods || m_state == MainAppState::Splash) {
    m_cinematicOverlay->render();

  } else if (m_state == MainAppState::Title) {
    m_titleScreen->render();
    m_cinematicOverlay->render();

  } else if (m_state > MainAppState::Title) {
    WorldClientPtr worldClient = m_universeClient->worldClient();
    if (worldClient) {
      auto totalStart = Time::monotonicMicroseconds();
      renderer->switchEffectConfig("world");
      auto clientStart = totalStart;
      worldClient->render(m_renderData, TilePainter::BorderTileSize);
      LogMap::set("client_render_world_client", strf(u8"{:05d}\u00b5s", Time::monotonicMicroseconds() - clientStart));

      auto paintStart = Time::monotonicMicroseconds();
      auto asyncLightingCallback = [&]() { worldClient->waitForLighting(); };
      m_worldPainter->render(m_renderData, asyncLightingCallback, worldClient->getLightMultiplier(), worldClient->getShaderParameters());
      LogMap::set("client_render_world_painter", strf(u8"{:05d}\u00b5s", Time::monotonicMicroseconds() - paintStart));
      LogMap::set("client_render_world_total", strf(u8"{:05d}\u00b5s", Time::monotonicMicroseconds() - totalStart));
    }
    renderer->switchEffectConfig("interface");
    auto start = Time::monotonicMicroseconds();
    m_mainInterface->renderInWorldElements();
    m_mainInterface->render();
    m_cinematicOverlay->render();
    LogMap::set("client_render_interface", strf(u8"{:05d}\u00b5s", Time::monotonicMicroseconds() - start));
  }

  if (!m_errorScreen->accepted())
    m_errorScreen->render(m_state == MainAppState::ModsWarning || m_state == MainAppState::Error);
}

void ClientApplication::getAudioData(int16_t* sampleData, size_t frameCount) {
  if (m_mainMixer) {
    m_mainMixer->read(sampleData, frameCount, [&](int16_t* buffer, size_t frames, unsigned channels) {
      if (m_voice)
        m_voice->mix(buffer, frames, channels);
    });
  }
}

void ClientApplication::changeState(MainAppState newState) {
  MainAppState oldState = m_state;
  m_state = newState;

  if (m_state == MainAppState::Quit)
    appController()->quit();

  if (newState == MainAppState::Mods)
    m_cinematicOverlay->load(m_root->assets()->json("/cinematics/mods/modloading.cinematic"));

  if (newState == MainAppState::Splash) {
    m_cinematicOverlay->load(m_root->assets()->json("/cinematics/splash.cinematic"));
    m_rootLoader = Thread::invoke("Async root loader", [this]() {
      m_root->fullyLoad();
    });
  }

  if (oldState > MainAppState::Title && m_state <= MainAppState::Title) {
    if (m_mainInterface)
      m_mainInterface->clean();

    if (m_universeClient)
      m_universeClient->disconnect();

    if (m_mainInterface)
      m_mainInterface->deregisterPanes(); // FezzedOne: Needed because of `interface.bindRegisteredPane`.
    m_mainInterface.reset();
    m_cinematicOverlay->stop();

    if (m_universeServer) {
      m_universeServer->stop();
      m_universeServer->join();
      m_universeServer.reset();
    }

    m_voice->clearSpeakers();

    if (auto p2pNetworkingService = appController()->p2pNetworkingService()) {
      p2pNetworkingService->setJoinUnavailable();
      p2pNetworkingService->setAcceptingP2PConnections(false);
    }
  }

  if (oldState > MainAppState::Title && m_state == MainAppState::Title) {
    m_titleScreen->resetState();
    if (m_statistics) {
      m_statistics->writeStatistics();
      m_statistics.reset();
    }
    m_statistics = make_shared<Statistics>(m_root->toStoragePath("player"), appController()->statisticsService());
    // FezzedOne: Might be causing some memory ballooning.
    // {
    //   m_universeClient.reset();
    //   m_playerStorage.reset();
    //   if (m_statistics) {
    //     m_statistics->writeStatistics();
    //     m_statistics.reset();
    //   }
    //   m_playerStorage = make_shared<PlayerStorage>(m_root->toStoragePath("player"));
    //   m_statistics = make_shared<Statistics>(m_root->toStoragePath("player"), appController()->statisticsService());
    //   m_universeClient = make_shared<UniverseClient>(m_playerStorage, m_statistics);

    //   m_universeClient->setLuaCallbacks("input", LuaBindings::makeInputCallbacks());
    //   m_universeClient->setLuaCallbacks("voice", LuaBindings::makeVoiceCallbacks());
    // }
    m_mainMixer->setUniverseClient({});
  }
  if (oldState >= MainAppState::Title && m_state < MainAppState::Title) {
    m_playerStorage.reset();

    if (m_statistics) {
      m_statistics->writeStatistics();
      m_statistics.reset();
    }

    m_universeClient.reset();
    m_mainMixer->setUniverseClient({});
    m_titleScreen.reset();
  }

  if (oldState < MainAppState::Title && m_state >= MainAppState::Title) {
    if (m_rootLoader)
      m_rootLoader.finish();

    m_cinematicOverlay->stop();

    m_playerStorage = make_shared<PlayerStorage>(m_root->toStoragePath("player"));
    m_statistics = make_shared<Statistics>(m_root->toStoragePath("player"), appController()->statisticsService());
    m_universeClient = make_shared<UniverseClient>(m_playerStorage, m_statistics);

    m_universeClient->setLuaCallbacks("input", LuaBindings::makeInputCallbacks());
    m_universeClient->setLuaCallbacks("voice", LuaBindings::makeVoiceCallbacks());

    // auto heldScriptPanes = make_shared<List<MainInterface::ScriptPaneInfo>>();

    m_universeClient->playerReloadPreCallback() = [&](bool resetInterface) {
      if (!resetInterface)
        return;

      // m_mainInterface->takeScriptPanes(*heldScriptPanes);
      if (m_mainInterface)
        m_mainInterface->clean();
    };

    m_universeClient->playerReloadCallback() = [&](bool resetInterface) {
      // auto paneManager = m_mainInterface->paneManager();
      // if (auto inventory = paneManager->registeredPane<InventoryPane>(MainInterfacePanes::Inventory))
      //   inventory->clearChangedSlots();

      if (m_mainInterface && resetInterface) {
        m_mainInterface->reset();
        // m_mainInterface->reviveScriptPanes(*heldScriptPanes);
        // heldScriptPanes->clear();
      }
    };

    m_mainMixer->setUniverseClient(m_universeClient);
    m_titleScreen = make_shared<TitleScreen>(m_playerStorage, m_mainMixer->mixer());
    if (auto renderer = Application::renderer())
      m_titleScreen->renderInit(renderer);
  }

  if (m_state == MainAppState::Title) {
    auto configuration = m_root->configuration();

    if (m_pendingMultiPlayerConnection) {
      if (auto address = m_pendingMultiPlayerConnection->server.ptr<HostAddressWithPort>()) {
        m_titleScreen->setMultiPlayerAddress((m_pendingMultiPlayerConnection->forceLegacyConnection ? "@" : "") +
                                             toString(address->address()));
        m_titleScreen->setMultiPlayerPort(toString(address->port()));
        m_titleScreen->setMultiPlayerAccount(configuration->getPath("title.multiPlayerAccount").toString());
        m_titleScreen->goToMultiPlayerSelectCharacter(false);
      } else {
        m_titleScreen->goToMultiPlayerSelectCharacter(true);
      }
    } else {
      m_titleScreen->setMultiPlayerAddress(configuration->getPath("title.multiPlayerAddress").toString());
      m_titleScreen->setMultiPlayerPort(configuration->getPath("title.multiPlayerPort").toString());
      m_titleScreen->setMultiPlayerAccount(configuration->getPath("title.multiPlayerAccount").toString());
    }
  }

  if (m_state > MainAppState::Title) {
    if (m_titleScreen->currentlySelectedPlayer()) {
      m_player = m_titleScreen->currentlySelectedPlayer();
    } else {
      if (auto uuid = m_playerStorage->playerUuidAt(0))
        m_player = m_playerStorage->loadPlayer(*uuid);

      if (!m_player) {
        setError("Error loading player!");
        return;
      }
    }

    m_mainMixer->setUniverseClient(m_universeClient);
    m_universeClient->setMainPlayer(m_player);
    m_cinematicOverlay->setPlayer(m_player);

    auto assets = m_root->assets();
    String loadingCinematic = assets->json("/client.config:loadingCinematic").toString();
    m_cinematicOverlay->load(assets->json(loadingCinematic));
    if (!m_player->log()->introComplete()) {
      String introCinematic = assets->json("/client.config:introCinematic").toString();
      introCinematic = introCinematic.replaceTags(StringMap<String>{{"species", m_player->species()}});
      m_player->setPendingCinematic(Json(introCinematic));
    } else {
      m_player->setPendingCinematic(Json());
    }

    if (m_state == MainAppState::MultiPlayer) {
      PacketSocketUPtr packetSocket;

      auto multiPlayerConnection = m_pendingMultiPlayerConnection.take();

      if (auto address = multiPlayerConnection.server.ptr<HostAddressWithPort>()) {
        try {
          // FezzedOne: Make the socket timeout configurable.
          unsigned timeout /* In milliseconds. */ = m_root->configuration()->get("clientPacketTimeout").optUInt().value(60000);
          packetSocket = TcpPacketSocket::open(TcpSocket::connectTo(*address, timeout));
        } catch (StarException const& e) {
          setError(strf("Join failed! Error connecting to '{}'", *address), e);
          return;
        }

      } else {
        auto p2pPeerId = multiPlayerConnection.server.ptr<P2PNetworkingPeerId>();

        if (auto p2pNetworkingService = appController()->p2pNetworkingService()) {
          auto result = p2pNetworkingService->connectToPeer(*p2pPeerId);
          if (result.isLeft()) {
            setError(strf("Cannot join peer: {}", result.left()));
            return;
          } else {
            packetSocket = P2PPacketSocket::open(std::move(result.right()));
          }
        } else {
          setError("Internal error, no p2p networking service when joining p2p networking peer");
          return;
        }
      }

      bool allowAssetsMismatch = m_root->configuration()->get("allowAssetsMismatch").toBool();
      if (auto errorMessage = m_universeClient->connect(UniverseConnection(std::move(packetSocket)), allowAssetsMismatch,
              multiPlayerConnection.account, multiPlayerConnection.password,
              multiPlayerConnection.forceLegacyConnection)) {
        setError(*errorMessage);
        return;
      }

      if (auto address = multiPlayerConnection.server.ptr<HostAddressWithPort>())
        m_currentRemoteJoin = *address;
      else
        m_currentRemoteJoin.reset();

    } else {
      if (!m_universeServer) {
        try {
          m_universeServer = make_shared<UniverseServer>(m_root->toStoragePath("universe"));
          m_universeServer->start();
        } catch (StarException const& e) {
          setError("Unable to start local server", e);
          return;
        }
      }

      if (auto errorMessage = m_universeClient->connect(m_universeServer->addLocalClient(), "", "")) {
        setError(strf("Error connecting locally: {}", *errorMessage));
        return;
      }
    }

    m_titleScreen->stopMusic();

    m_mainInterface = make_shared<MainInterface>(m_universeClient, m_worldPainter, m_cinematicOverlay);
    m_universeClient->setLuaCallbacks("interface", LuaBindings::makeInterfaceCallbacks(m_mainInterface.get(), true), 1);
    m_universeClient->setLuaCallbacks("interface", LuaBindings::makeInterfaceCallbacks(m_mainInterface.get(), false), 2);
    // [OpenStarbound] Kae: Added camera callbacks.
    m_universeClient->setLuaCallbacks("camera", LuaBindings::makeCameraCallbacks(&m_worldPainter->camera()));
    m_universeClient->setLuaCallbacks("clipboard", LuaBindings::makeClipboardCallbacks(m_mainInterface.get()));
    m_universeClient->setLuaCallbacks("chat", LuaBindings::makeChatCallbacks(m_mainInterface.get()));
    m_universeClient->startLua();

    m_mainMixer->setWorldPainter(m_worldPainter);

    if (auto renderer = Application::renderer()) {
      m_worldPainter->renderInit(renderer);
    }
  }
}

void ClientApplication::setError(String const& error) {
  Logger::error(error.utf8Ptr());
  m_errorScreen->setMessage(error);
  changeState(MainAppState::Title);
}

void ClientApplication::setError(String const& error, std::exception const& e) {
  Logger::error("{}\n{}", error, outputException(e, true));
  m_errorScreen->setMessage(strf("{}\n{}", error, outputException(e, false)));
  changeState(MainAppState::Title);
}

void ClientApplication::updateMods(float dt) {
  m_cinematicOverlay->update(dt);
  auto ugcService = appController()->userGeneratedContentService();
  if (ugcService && !m_skipWorkshop) {
    if (ugcService->triggerContentDownload()) {
      StringList modDirectories;
      for (auto contentId : ugcService->subscribedContentIds()) {
        if (auto contentDirectory = ugcService->contentDownloadDirectory(contentId)) {
          Logger::info("Loading Steam Workshop item with ID '{}' from directory '{}'", contentId, *contentDirectory);
          modDirectories.append(*contentDirectory);
        } else {
          Logger::warn("Steam Workshop item with ID '{}' is not available", contentId);
        }
      }

      if (modDirectories.empty()) {
        Logger::info("No subscribed Steam Workshop content");
        changeState(MainAppState::Splash);
      } else {
        Logger::info("Reloading to include all Steam Workshop content");
        Root::singleton().reloadWithMods(modDirectories);

        auto configuration = m_root->configuration();
        auto assets = m_root->assets();

        if (configuration->get("modsWarningShown").optBool().value()) {
          changeState(MainAppState::Splash);
        } else {
          configuration->set("modsWarningShown", true);
          m_errorScreen->setMessage(assets->json("/interface.config:modsWarningMessage").toString());
          changeState(MainAppState::ModsWarning);
        }
      }
    }
  } else {
    if (ugcService)
      Logger::info("Skipping Steam workshop content");
    changeState(MainAppState::Splash);
  }
}

void ClientApplication::updateModsWarning(float dt) {
  if (m_errorScreen->accepted())
    changeState(MainAppState::Splash);
}

void ClientApplication::updateSplash(float dt) {
  m_cinematicOverlay->update(dt);
  if (!m_rootLoader.isRunning() && (m_cinematicOverlay->completable() || m_cinematicOverlay->completed()))
    changeState(MainAppState::Title);
}

void ClientApplication::updateError(float dt) {
  if (m_errorScreen->accepted())
    changeState(MainAppState::Title);
}

void ClientApplication::updateTitle(float dt) {
  m_cinematicOverlay->update(dt);

  m_titleScreen->update(dt);
  m_mainMixer->update(dt);
  m_mainMixer->setSpeed(GlobalTimescale);

  appController()->setAcceptingTextInput(m_titleScreen->textInputActive());

  auto p2pNetworkingService = appController()->p2pNetworkingService();
  if (p2pNetworkingService)
    p2pNetworkingService->setActivityData("In Main Menu", {});

  if (m_titleScreen->currentState() == TitleState::StartSinglePlayer) {
    changeState(MainAppState::SinglePlayer);

  } else if (m_titleScreen->currentState() == TitleState::StartMultiPlayer) {
    if (!m_pendingMultiPlayerConnection || m_pendingMultiPlayerConnection->server.is<HostAddressWithPort>()) {
      auto rawAddressString = m_titleScreen->multiPlayerAddress().trim();
      bool forceLegacyConnection = false;
      if (rawAddressString.beginsWith('@'))
        forceLegacyConnection = true;
      auto addressString = rawAddressString.trimBeg("@");
      auto portString = m_titleScreen->multiPlayerPort().trim();
      portString = portString.empty() ? toString(m_root->configuration()->get("gameServerPort").toUInt()) : portString;
      if (auto port = maybeLexicalCast<uint16_t>(portString)) {
        auto address = HostAddressWithPort::lookup(addressString, *port);
        if (address.isLeft()) {
          setError(address.left());
        } else {
          m_pendingMultiPlayerConnection = PendingMultiPlayerConnection{
              address.right(),
              m_titleScreen->multiPlayerAccount(),
              m_titleScreen->multiPlayerPassword(),
              forceLegacyConnection};

          auto configuration = m_root->configuration();
          configuration->setPath("title.multiPlayerAddress", m_titleScreen->multiPlayerAddress());
          configuration->setPath("title.multiPlayerPort", m_titleScreen->multiPlayerPort());
          configuration->setPath("title.multiPlayerAccount", m_titleScreen->multiPlayerAccount());

          changeState(MainAppState::MultiPlayer);
        }
      } else {
        setError(strf("invalid port: {}", portString));
      }
    } else {
      changeState(MainAppState::MultiPlayer);
    }

  } else if (m_titleScreen->currentState() == TitleState::Quit) {
    changeState(MainAppState::Quit);
  }
}

void ClientApplication::updateRunning(float dt) {
  ZoneScoped;
  try {
    {
      ZoneScopedN("Steam/Discord networking update");
      auto p2pNetworkingService = appController()->p2pNetworkingService();
      bool clientIPJoinable = m_root->configuration()->get("clientIPJoinable").toBool();
      bool clientP2PJoinable = m_root->configuration()->get("clientP2PJoinable").toBool();
      Maybe<pair<uint16_t, uint16_t>> party = make_pair(m_universeClient->players(), m_universeClient->maxPlayers());

      if (m_state == MainAppState::MultiPlayer) {
        if (p2pNetworkingService) {
          p2pNetworkingService->setAcceptingP2PConnections(false);
          if (clientP2PJoinable && m_currentRemoteJoin)
            p2pNetworkingService->setJoinRemote(*m_currentRemoteJoin);
          else
            p2pNetworkingService->setJoinUnavailable();
        }
      } else {
        m_universeServer->setListeningTcp(clientIPJoinable);
        if (p2pNetworkingService) {
          p2pNetworkingService->setAcceptingP2PConnections(clientP2PJoinable);
          if (clientP2PJoinable) {
            p2pNetworkingService->setJoinLocal(m_universeServer->maxClients());
          } else {
            p2pNetworkingService->setJoinUnavailable();
            party = {};
          }
        }
      }

      if (p2pNetworkingService)
        p2pNetworkingService->setActivityData("In Game", party);
    }

    {
      ZoneScopedN("Input update");
      if (!m_mainInterface->inputFocus() && !m_cinematicOverlay->suppressInput()) {
        m_player->setShifting(isActionTaken(InterfaceAction::PlayerShifting));

        if (isActionTaken(InterfaceAction::PlayerRight))
          m_player->moveRight();
        if (isActionTaken(InterfaceAction::PlayerLeft))
          m_player->moveLeft();
        if (isActionTaken(InterfaceAction::PlayerUp))
          m_player->moveUp();
        if (isActionTaken(InterfaceAction::PlayerDown))
          m_player->moveDown();
        if (isActionTaken(InterfaceAction::PlayerJump))
          m_player->jump();

        if (isActionTaken(InterfaceAction::PlayerTechAction1))
          m_player->special(1);
        if (isActionTaken(InterfaceAction::PlayerTechAction2))
          m_player->special(2);
        if (isActionTaken(InterfaceAction::PlayerTechAction3))
          m_player->special(3);

        if (isActionTakenEdge(InterfaceAction::PlayerInteract))
          m_player->beginTrigger();
        else if (!isActionTaken(InterfaceAction::PlayerInteract))
          m_player->endTrigger();

        if (isActionTakenEdge(InterfaceAction::PlayerDropItem))
          m_player->dropItem();

        if (isActionTakenEdge(InterfaceAction::EmoteBlabbering))
          m_player->addEmote(HumanoidEmote::Blabbering);
        if (isActionTakenEdge(InterfaceAction::EmoteShouting))
          m_player->addEmote(HumanoidEmote::Shouting);
        if (isActionTakenEdge(InterfaceAction::EmoteHappy))
          m_player->addEmote(HumanoidEmote::Happy);
        if (isActionTakenEdge(InterfaceAction::EmoteSad))
          m_player->addEmote(HumanoidEmote::Sad);
        if (isActionTakenEdge(InterfaceAction::EmoteNeutral))
          m_player->addEmote(HumanoidEmote::NEUTRAL);
        if (isActionTakenEdge(InterfaceAction::EmoteLaugh))
          m_player->addEmote(HumanoidEmote::Laugh);
        if (isActionTakenEdge(InterfaceAction::EmoteAnnoyed))
          m_player->addEmote(HumanoidEmote::Annoyed);
        if (isActionTakenEdge(InterfaceAction::EmoteOh))
          m_player->addEmote(HumanoidEmote::Oh);
        if (isActionTakenEdge(InterfaceAction::EmoteOooh))
          m_player->addEmote(HumanoidEmote::OOOH);
        if (isActionTakenEdge(InterfaceAction::EmoteBlink))
          m_player->addEmote(HumanoidEmote::Blink);
        if (isActionTakenEdge(InterfaceAction::EmoteWink))
          m_player->addEmote(HumanoidEmote::Wink);
        if (isActionTakenEdge(InterfaceAction::EmoteEat))
          m_player->addEmote(HumanoidEmote::Eat);
        if (isActionTakenEdge(InterfaceAction::EmoteSleep))
          m_player->addEmote(HumanoidEmote::Sleep);
      }
    }

    /* FezzedOne: Getting rid of this for now. */
    // if (m_controllerLeftStick.magnitudeSquared() > 0.001f)
    //   m_player->setMoveVector(m_controllerLeftStick);
    // else
    //   m_player->setMoveVector(Vec2F());

    auto checkDisconnection = [this]() {
      if (!m_universeClient->isConnected()) {
        m_cinematicOverlay->stop();
        String errMessage;
        if (auto disconnectReason = m_universeClient->disconnectReason())
          errMessage = strf("You were disconnected from the server for the following reason:\n{}", *disconnectReason);
        else
          errMessage = "Client-server connection no longer valid!";
        setError(errMessage);
        changeState(MainAppState::Title);
        return true;
      }

      return false;
    };

    if (checkDisconnection())
      return;

    DataStreamBuffer voiceData;
    bool needsToSendVoice = false;
    {
      ZoneScopedN("Voice audio encoding");
      m_voice->setInput(m_input->bindHeld("xsb", "pushToTalk"));
      voiceData.setByteOrder(ByteOrder::LittleEndian);
      //voiceData.writeBytes(VoiceBroadcastPrefix.utf8Bytes()); transmitting with SE compat for now
      needsToSendVoice = m_voice->send(voiceData, 5000);
    }

    // FezzedOne: Needs to be moved up here to prevent a callback from returning a position of {0, 0} the tick after a world is loaded.
    updateCamera(dt);
    m_universeClient->update(dt);
    // FezzedOne: Make sure inputs are always passed to the active player.
    m_player = m_universeClient->mainPlayer();
    m_cinematicOverlay->setPlayer(m_player);

    if (checkDisconnection())
      return;

    if (auto worldClient = m_universeClient->worldClient()) {
      m_worldPainter->update(dt);
      auto& broadcastCallback = worldClient->broadcastCallback();
      if (!broadcastCallback) {
        broadcastCallback = [&](PlayerPtr player, StringView broadcast) -> bool {
          auto& view = broadcast.utf8();
          if (view.rfind(VoiceBroadcastPrefix.utf8(), 0) != NPos) {
            ZoneScopedN("Voice audio decoding");
            auto entityId = player->entityId();
            auto speaker = m_voice->speaker(connectionForEntity(entityId));
            speaker->entityId = entityId;
            speaker->name = player->name();
            speaker->position = player->mouthPosition();
            m_voice->receive(speaker, view.substr(VoiceBroadcastPrefix.utf8Size()));
          }
          return true;
        };
      }

      if (worldClient->inWorld()) {
        if (needsToSendVoice) {
          ZoneScopedN("Outbound voice networking");
          auto signature = Curve25519::sign(voiceData.ptr(), voiceData.size());
          std::string_view signatureView((char*)signature.data(), signature.size());
          std::string_view audioDataView(voiceData.ptr(), voiceData.size());
          auto broadcast = strf("data\0voice\0{}{}"s, signatureView, audioDataView);
          worldClient->sendSecretBroadcast(broadcast, true, false); // Already compressed by Opus.
        }
        if (auto mainPlayer = m_universeClient->mainPlayer()) {
          auto localSpeaker = m_voice->localSpeaker();
          localSpeaker->position = mainPlayer->position();
          localSpeaker->entityId = mainPlayer->entityId();
          localSpeaker->name = mainPlayer->name();
        }
        m_voice->setLocalSpeaker(worldClient->connection());
      }
      worldClient->setInteractiveHighlightMode(isActionTaken(InterfaceAction::ShowLabels));
    }

    m_cinematicOverlay->update(dt);
    m_mainInterface->update(dt);
    m_mainMixer->update(dt, m_cinematicOverlay->muteSfx(), m_cinematicOverlay->muteMusic(),
        m_cinematicOverlay->muteMusic() || m_cinematicOverlay->muteSfx());
    m_mainMixer->setSpeed(GlobalTimescale);

    bool inputActive = m_mainInterface->textInputActive();
    appController()->setAcceptingTextInput(inputActive);
    m_input->setTextInputActive(inputActive);

    {
      ZoneScopedN("Voice networking");
      for (auto const& interactAction : m_player->pullInteractActions())
        m_mainInterface->handleInteractAction(interactAction);
    }

    if (m_universeServer) {
      ZoneScopedN("Steam/Discord connection handling");
      if (auto p2pNetworkingService = appController()->p2pNetworkingService()) {
        for (auto& p2pClient : p2pNetworkingService->acceptP2PConnections())
          m_universeServer->addClient(UniverseConnection(P2PPacketSocket::open(std::move(p2pClient))));
      }

      m_universeServer->setPause(m_mainInterface->escapeDialogOpen());
    }

    Vec2F aimPosition = m_player->aimPosition();
    float fps = appController()->renderFps();
    LogMap::set("client_render_rate", strf("{:4.2f} FPS ({:4.2f}ms)", fps, (1.0f / appController()->renderFps()) * 1000.0f));
    LogMap::set("client_update_rate", strf("{:4.2f}Hz", appController()->updateRate()));
    LogMap::set("player_pos", strf("[ ^#f45;{:4.2f}^reset;, ^#49f;{:4.2f}^reset; ]", m_player->position()[0], m_player->position()[1]));
    LogMap::set("player_vel", strf("[ ^#f45;{:4.2f}^reset;, ^#49f;{:4.2f}^reset; ]", m_player->velocity()[0], m_player->velocity()[1]));
    LogMap::set("player_aim", strf("[ ^#f45;{:4.2f}^reset;, ^#49f;{:4.2f}^reset; ]", aimPosition[0], aimPosition[1]));
    if (auto world = m_universeClient->worldClient()) {
      auto aim = Vec2I::floor(aimPosition);
      LogMap::set("tile_liquid_level", toString(world->liquidLevel(aim).level));
      LogMap::set("tile_dungeon_id", world->isTileProtected(aim) ? strf("^red;{}", world->dungeonId(aim)) : toString(world->dungeonId(aim)));
    }

    if (m_mainInterface->currentState() == MainInterface::ReturnToTitle) {
      // FezzedOne: Clean up cached directives in the image metadata cache upon exiting to the menu.
      Root::singleton().cleanUpImageMetadata();
      changeState(MainAppState::Title);
    }

  } catch (std::exception& e) {
    setError("Exception caught in client main-loop", e);
  }
}

bool ClientApplication::isActionTaken(InterfaceAction action) const {
  for (auto keyEvent : m_heldKeyEvents) {
    if (m_guiContext->actions(keyEvent).contains(action))
      return true;
  }

  return false;
}

bool ClientApplication::isActionTakenEdge(InterfaceAction action) const {
  for (auto keyEvent : m_edgeKeyEvents) {
    if (m_guiContext->actions(keyEvent).contains(action))
      return true;
  }

  return false;
}

void ClientApplication::updateCamera(float dt) {
  ZoneScoped;

  if (!m_universeClient->worldClient())
    return;

  WorldCamera& camera = m_worldPainter->camera();
  camera.update(dt);

  Maybe<Vec2F> cameraOverridePosition = m_mainInterface->cameraPositionOverride();
  if (cameraOverridePosition) {
    m_worldPainter->setCameraPosition(m_universeClient->worldClient()->geometry(), *cameraOverridePosition);
    m_mainInterface->passCameraPosition(*cameraOverridePosition);
    m_universeClient->worldClient()->setClientWindow(camera.worldTileRect());
    m_mainInterface->setCameraPositionOverride(Maybe<Vec2F>{});
    return;
  }

  if (m_mainInterface->fixedCamera())
    return;

  auto assets = m_root->assets();

  const float triggerRadius = 100.0f;
  const float deadzone = 0.1f;
  const float smoothFactor = 30.0f;
  const float panFactor = 1.5f;

  auto playerCameraPosition = m_player->cameraPosition();

  if (isActionTaken(InterfaceAction::CameraShift)) {
    m_snapBackCameraOffset = false;
    m_cameraOffsetDownTicks++;
    Vec2F aim = m_universeClient->worldClient()->geometry().diff(m_mainInterface->cursorWorldPosition(), playerCameraPosition);

    float magnitude = aim.magnitude() / (triggerRadius / camera.pixelRatio());
    if (magnitude > deadzone) {
      float cameraXOffset = aim.x() / magnitude;
      float cameraYOffset = aim.y() / magnitude;
      magnitude = (magnitude - deadzone) / (1.0 - deadzone);
      if (magnitude > 1)
        magnitude = 1;
      cameraXOffset *= magnitude * 0.5f * camera.pixelRatio() * panFactor;
      cameraYOffset *= magnitude * 0.5f * camera.pixelRatio() * panFactor;
      m_cameraXOffset = (m_cameraXOffset * (smoothFactor - 1.0) + cameraXOffset) / smoothFactor;
      m_cameraYOffset = (m_cameraYOffset * (smoothFactor - 1.0) + cameraYOffset) / smoothFactor;
    }
  } else {
    if ((m_cameraOffsetDownTicks > 0) && (m_cameraOffsetDownTicks < 20))
      m_snapBackCameraOffset = true;
    if (m_snapBackCameraOffset) {
      m_cameraXOffset = (m_cameraXOffset * (smoothFactor - 1.0)) / smoothFactor;
      m_cameraYOffset = (m_cameraYOffset * (smoothFactor - 1.0)) / smoothFactor;
    }
    m_cameraOffsetDownTicks = 0;
  }
  Vec2F newCameraPosition;

  newCameraPosition.setX(playerCameraPosition.x());
  newCameraPosition.setY(playerCameraPosition.y());

  auto baseCamera = newCameraPosition;

  const float cameraSmoothRadius = assets->json("/interface.config:cameraSmoothRadius").toFloat();
  const float cameraSmoothFactor = assets->json("/interface.config:cameraSmoothFactor").toFloat();

  auto cameraSmoothDistance = m_universeClient->worldClient()->geometry().diff(m_cameraPositionSmoother, newCameraPosition).magnitude();
  if (cameraSmoothDistance > cameraSmoothRadius) {
    auto cameraDelta = m_universeClient->worldClient()->geometry().diff(m_cameraPositionSmoother, newCameraPosition);
    m_cameraPositionSmoother = newCameraPosition + cameraDelta.normalized() * cameraSmoothRadius;
    m_cameraSmoothDelta = {};
  }

  auto cameraDelta = m_universeClient->worldClient()->geometry().diff(m_cameraPositionSmoother, newCameraPosition);
  if (cameraDelta.magnitude() > assets->json("/interface.config:cameraSmoothDeadzone").toFloat())
    newCameraPosition = newCameraPosition + cameraDelta * (cameraSmoothFactor - 1.0) / cameraSmoothFactor;
  m_cameraPositionSmoother = newCameraPosition;

  newCameraPosition.setX(newCameraPosition.x() + m_cameraXOffset / camera.pixelRatio());
  newCameraPosition.setY(newCameraPosition.y() + m_cameraYOffset / camera.pixelRatio());

  auto smoothDelta = newCameraPosition - baseCamera;

  Vec2F finalCameraPosition = baseCamera + (smoothDelta + m_cameraSmoothDelta) * 0.5f;
  m_worldPainter->setCameraPosition(m_universeClient->worldClient()->geometry(), finalCameraPosition);
  m_mainInterface->passCameraPosition(finalCameraPosition);
  m_cameraSmoothDelta = smoothDelta;

  m_universeClient->worldClient()->setClientWindow(camera.worldTileRect());
}

} // namespace Star

STAR_MAIN_APPLICATION(Star::ClientApplication);
