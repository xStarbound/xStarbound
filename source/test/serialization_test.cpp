#include "StarDataStreamDevices.hpp"

#include "gtest/gtest.h"

using namespace Star;

template <typename T>
void testMap(T const& map) {
  auto byteArray = DataStreamBuffer::serializeMapContainer(map);
  auto mapOut = DataStreamBuffer::deserializeMapContainer<T>(byteArray);
  EXPECT_EQ(map, mapOut);
}

TEST(DataStreamTest, All) {
  Map<int, int> map1 = {
      {1, 2},
      {3, 4},
      {5, 6},
  };

  Map<String, int> map2 = {
      {"asdf", 1},
      {"asdf1", 2},
      {"omg", 2},
  };

  Map<String, int> map3 = {};

  testMap(map1);
  testMap(map2);
  testMap(map3);

  // From @Bottinator22.
  DataStreamBuffer ds;
  std::string testStr = "\x80\x80";
  std::string outStdStr;
  String outStr;
  String sanitized = String(testStr).sanitize();

  ds << testStr.c_str();
  ds << testStr;
  ds << String(testStr);
  ds.seek(0);
  ds >> outStdStr;
  EXPECT_EQ(outStdStr, sanitized.utf8());
  ds >> outStdStr;
  EXPECT_EQ(outStdStr, sanitized.utf8());
  ds >> outStr;
  EXPECT_EQ(outStr, sanitized);

  // Per @Bottinator22, but for xSB: Enable this and add `bool m_isLegacy` to `DataStreamBuffer` or whatever serialises strings if serialisation ever needs to be changed.
  // @FezzedOne: Which should not ever need to happen. Changing serialisation breaks xStarbound/OpenStarbound hidden player properties and potentially even cross-client voice chat anyway.
  /*
  ds.seek(0);

  ds << testStr.c_str();
  ds << testStr;
  ds << String(testStr);
  ds.seek(0);
  ds >> outStdStr;
  EXPECT_EQ(outStdStr, testStr);
  ds >> outStdStr;
  EXPECT_EQ(outStdStr, testStr);
  ds >> outStr;
  EXPECT_EQ(outStr, String(testStr));
  */
}
