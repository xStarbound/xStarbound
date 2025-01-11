#include "StarFile.hpp"
#include "StarVersioningDatabase.hpp"

#ifdef STAR_USE_RPMALLOC
#include "rpmalloc/rpmalloc.h"
#endif

using namespace Star;

int main(int argc, char** argv) {
#ifdef STAR_USE_RPMALLOC
  ::rpmalloc_initialize();
#endif
  try {
    if (argc != 3) {
      coutf("Usage, {} <versioned_json_binary> <versioned_json_json>\n", argv[0]);
      return -1;
    }

    auto versionedJson = VersionedJson::readFile(argv[1]);
    File::writeFile(versionedJson.toJson().printJson(2), argv[2]);
    return 0;
  } catch (std::exception const& e) {
    coutf("Error! Caught exception {}\n", outputException(e, true));
    return 1;
  }
}
