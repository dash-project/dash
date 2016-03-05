#include <libdash.h>
#include <array>
#include <sstream>

#include "TestBase.h"
#include "ConfigTest.h"

TEST_F(ConfigTest, Deallocate) {
  DASH_TEST_LOCAL_ONLY();

  using dash::util::Config;

  ASSERT_FALSE(Config::is_set("CONFIG_TEST_INT"));
  Config::set("CONFIG_TEST_INT", 123);

  ASSERT_TRUE(Config::is_set("CONFIG_TEST_INT"));
  ASSERT_EQ(123, Config::get<int>("CONFIG_TEST_INT"));
  Config::set("CONFIG_TEST_INT", 234);
  ASSERT_EQ(234, Config::get<int>("CONFIG_TEST_INT"));

  Config::set("CONFIG_TEST_STRING", "foo");
  ASSERT_EQ("foo", Config::get<std::string>("CONFIG_TEST_STRING"));

  ASSERT_FALSE(Config::get<bool>("CONFIG_TEST_BOOL"));
  Config::set("CONFIG_TEST_BOOL", true);
  ASSERT_TRUE(Config::get<bool>("CONFIG_TEST_BOOL"));
  Config::set("CONFIG_TEST_BOOL", false);
  ASSERT_FALSE(Config::get<bool>("CONFIG_TEST_BOOL"));

  size_t size_bytes;
  Config::set("CONFIG_TEST_SIZE", "2K");
  ASSERT_TRUE(Config::is_set("CONFIG_TEST_SIZE_BYTES"));
  size_bytes = Config::get<size_t>("CONFIG_TEST_SIZE_BYTES");
  ASSERT_EQ(2 * 1024, size_bytes);

  Config::set("CONFIG_TEST_SIZE", "23M");
  ASSERT_TRUE(Config::is_set("CONFIG_TEST_SIZE_BYTES"));
  size_bytes = Config::get<size_t>("CONFIG_TEST_SIZE_BYTES");
  ASSERT_EQ(23 * 1024 * 1024, size_bytes);

  for (auto kv = Config::begin(); kv != Config::end(); ++kv) {
    LOG_MESSAGE("Configuration key: %s value: %s",
                kv->first.c_str(), kv->second.c_str());
    ASSERT_TRUE(Config::is_set(kv->first));
  }
}

