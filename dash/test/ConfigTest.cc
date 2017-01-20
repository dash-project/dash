
#include "ConfigTest.h"

#include <dash/util/Config.h>

#include <array>
#include <sstream>


TEST_F(ConfigTest, BasicSetGet) {
  DASH_TEST_LOCAL_ONLY();

  using dash::util::Config;

  EXPECT_FALSE(Config::is_set("CONFIG_TEST_INT"));
  Config::set("CONFIG_TEST_INT", 123);

  EXPECT_TRUE(Config::is_set("CONFIG_TEST_INT"));
  EXPECT_EQ(123, Config::get<int>("CONFIG_TEST_INT"));
  Config::set("CONFIG_TEST_INT", 234);
  EXPECT_EQ(234, Config::get<int>("CONFIG_TEST_INT"));

  Config::set("CONFIG_TEST_STRING", "foo");
  EXPECT_EQ("foo", Config::get<std::string>("CONFIG_TEST_STRING"));

  EXPECT_FALSE(Config::get<bool>("CONFIG_TEST_BOOL"));
  Config::set("CONFIG_TEST_BOOL", true);
  EXPECT_TRUE(Config::get<bool>("CONFIG_TEST_BOOL"));
  Config::set("CONFIG_TEST_BOOL", false);
  EXPECT_FALSE(Config::get<bool>("CONFIG_TEST_BOOL"));

  size_t size_bytes;
  Config::set("CONFIG_TEST_SIZE", "2K");
  EXPECT_TRUE(Config::is_set("CONFIG_TEST_SIZE_BYTES"));
  size_bytes = Config::get<size_t>("CONFIG_TEST_SIZE_BYTES");
  EXPECT_EQ(2 * 1024, size_bytes);

  Config::set("CONFIG_TEST_SIZE", "23M");
  EXPECT_TRUE(Config::is_set("CONFIG_TEST_SIZE_BYTES"));
  size_bytes = Config::get<size_t>("CONFIG_TEST_SIZE_BYTES");
  EXPECT_EQ(23 * 1024 * 1024, size_bytes);

  for (auto kv = Config::begin(); kv != Config::end(); ++kv) {
    LOG_MESSAGE("Configuration key: %s value: %s",
                kv->first.c_str(), kv->second.c_str());
    EXPECT_TRUE(Config::is_set(kv->first));
  }
}

