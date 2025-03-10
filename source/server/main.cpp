#include "StarFile.hpp"
#include "StarRandom.hpp"
#include "StarLexicalCast.hpp"
#include "StarLogging.hpp"
#include "StarUniverseServer.hpp"
#include "StarRootLoader.hpp"
#include "StarConfiguration.hpp"
#include "StarVersion.hpp"
#include "StarServerQueryThread.hpp"
#include "StarServerRconThread.hpp"
#include "StarSignalHandler.hpp"

#ifdef STAR_USE_RPMALLOC
#include "rpmalloc.h"
#endif

using namespace Star;

// FezzedOne: Fixed an inconsistency that caused world crashes only on the server.
Json const AdditionalAssetsSettings = Json::parseJson(R"JSON(
  {
    "missingImage": "/assetmissing.png",
    "missingAudio": "/assetmissing.wav"
  }
)JSON");

Json const AdditionalDefaultConfiguration = Json::parseJson(R"JSON(
    {
      "configurationVersion" : {
        "server" : 4
      },

      "runQueryServer" : false,
      "queryServerPort" : 21025,
      "queryServerBind" : "::",

      "runRconServer" : false,
      "rconServerPort" : 21026,
      "rconServerBind" : "::",
      "rconServerPassword" : "",
      "rconServerTimeout" : 1000,

      "assetLuaGcPause" : 1.2,
      "assetLuaGcStepMultiplier" : 1.2,

      "allowAssetsMismatch" : true,
      "serverOverrideAssetsDigest" : null
    }
  )JSON");

int main(int argc, char** argv) {
#ifdef STAR_USE_RPMALLOC
  ::rpmalloc_initialize();
#endif
  try {
    RootLoader rootLoader({AdditionalAssetsSettings, AdditionalDefaultConfiguration, String("xserver.log"), LogLevel::Info, false, String("xserver.config")});
    RootUPtr root = rootLoader.commandInitOrDie(argc, argv).first;
    root->fullyLoad();

    SignalHandler signalHandler;
    signalHandler.setHandleFatal(true);
    signalHandler.setHandleInterrupt(true);

    auto configuration = root->configuration();
    {
      Logger::info("xServer v{} [Starbound v{}] ({}) // Source ID: {} // Protocol: {}", xSbVersionString, StarVersionString, StarArchitectureString, StarSourceIdentifierString, StarProtocolVersion);
      float updateRate = 1.0f / GlobalTimestep;
      if (auto jUpdateRate = configuration->get("updateRate")) {
        updateRate = jUpdateRate.toFloat();
        ServerGlobalTimestep = GlobalTimestep = 1.0f / updateRate;
        Logger::info("[Init] Configured tickrate is {:4.2f}hz", updateRate);
      }

      UniverseServerUPtr server = make_unique<UniverseServer>(root->toStoragePath("universe"));
      server->setListeningTcp(true);
      server->start();

      ServerQueryThreadUPtr queryServer;
      if (configuration->get("runQueryServer").toBool()) {
        queryServer = make_unique<ServerQueryThread>(server.get(), HostAddressWithPort(configuration->get("queryServerBind").toString(), configuration->get("queryServerPort").toInt()));
        queryServer->start();
      }

      ServerRconThreadUPtr rconServer;
      if (configuration->get("runRconServer").toBool()) {
        rconServer = make_unique<ServerRconThread>(server.get(), HostAddressWithPort(configuration->get("rconServerBind").toString(), configuration->get("rconServerPort").toInt()));
        rconServer->start();
      }

      while (server->isRunning()) {
        if (signalHandler.interruptCaught()) {
          Logger::info("Interrupt caught!");
          server->stop();
          break;
        }
        Thread::sleep(100);
      }

      server->join();

      if (queryServer) {
        queryServer->stop();
        queryServer->join();
      }

      if (rconServer) {
        rconServer->stop();
        rconServer->join();
      }
    }

    Logger::info("Server shutdown gracefully");
  } catch (std::exception const& e) {
    fatalException(e, true);
  }

  #ifdef STAR_USE_RPMALLOC
    ::rpmalloc_finalize();
  #endif
  return 0;
}
