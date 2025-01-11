#include "StarLogging.hpp"
#include "StarFile.hpp"
#include "StarRootLoader.hpp"

#include "gtest/gtest.h"

#ifdef STAR_USE_RPMALLOC
#include "rpmalloc/rpmalloc.h"
#endif

using namespace Star;

struct ErrorLogSink : public LogSink {
  ErrorLogSink() {
    setLevel(LogLevel::Error);
  }

  void log(char const* msg, LogLevel) override {
    ADD_FAILURE() << "Error was logged: " << msg;
  }
};

class TestEnvironment : public testing::Environment {
public:
  unique_ptr<Root> root;
  Root::Settings settings;

  TestEnvironment(Root::Settings settings)
    : settings(std::move(settings)) {}

  virtual void SetUp() {
    Logger::addSink(make_shared<ErrorLogSink>());
    root = make_unique<Root>(settings);
    root->configuration()->set("clearUniverseFiles", true);
    root->configuration()->set("clearPlayerFiles", true);
  }

  virtual void TearDown() {
    root.reset();
  }
};

int main(int argc, char** argv) {
#ifdef STAR_USE_RPMALLOC
  ::rpmalloc_initialize();
#endif
  testing::InitGoogleTest(&argc, argv);
  testing::AddGlobalTestEnvironment(new TestEnvironment(RootLoader({{}, {}, {}, LogLevel::Error, true, {}}).commandParseOrDie(argc, argv).first));
  return RUN_ALL_TESTS();
}
