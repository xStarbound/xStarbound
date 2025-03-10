#include "StarRootLoader.hpp"
#include "StarRootLuaBindings.hpp"
#include "StarUtilityLuaBindings.hpp"
#include "StarRootLuaBindings.hpp"

#ifdef STAR_USE_RPMALLOC
#include "rpmalloc.h"
#endif

using namespace Star;

int main(int argc, char** argv) {
#ifdef STAR_USE_RPMALLOC
  ::rpmalloc_initialize();
#endif
  RootLoader rootLoader({{}, {}, {}, LogLevel::Error, false, {}});
  RootUPtr root;
  OptionParser::Options options;
  tie(root, options) = rootLoader.commandInitOrDie(argc, argv);

  auto engine = LuaEngine::create(true);
  auto context = engine->createContext();
  context.setCallbacks("xsb", LuaBindings::makeXsbCallbacks());
  context.setCallbacks("sb", LuaBindings::makeUtilityCallbacks());
  context.setCallbacks("root", LuaBindings::makeRootCallbacks());

  String code;
  bool continuation = false;
  while (!std::cin.eof()) {
    auto getline = [](std::istream& stream) -> String {
      std::string line;
      std::getline(stream, line);
      return String(std::move(line));
    };

    if (continuation) {
      std::cout << ">> ";
      std::cout.flush();
      code += getline(std::cin);
      code += '\n';
    } else {
      std::cout << "> ";
      std::cout.flush();
      code = getline(std::cin);
      code += '\n';
    }

    try {
      auto result = context.eval<LuaVariadic<LuaValue>>(code);
      for (auto r : result)
        coutf("{}\n", r);
      continuation = false;
    } catch (LuaIncompleteStatementException const&) {
      continuation = true;
    } catch (std::exception const& e) {
      coutf("Error: {}\n", outputException(e, false));
      continuation = false;
    }
  }
#ifdef STAR_USE_RPMALLOC
  ::rpmalloc_finalize();
#endif
  return 0;
}
