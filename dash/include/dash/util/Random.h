#ifndef DASH__UTIL__RANDOM_H__
#define DASH__UTIL__RANDOM_H__

#include <string>
#include <random>
#include <algorithm>


namespace dash {
namespace util {
static std::string const default_chars =
  "abcdefghijklmnaoqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";

static std::string random_str(size_t len)
{
#if defined(__POSIX_SOURCE)
  std::mt19937_64 gen { std::random_device()() };
#else
  std::mt19937 gen { std::random_device()() };
#endif

  std::uniform_int_distribution<size_t> dist { 0, default_chars.length() - 1 };

  std::string ret;
  ret.reserve(len);

  std::generate_n(std::back_inserter(ret), len, [&] { return default_chars[dist(gen)]; });
  return ret;
}
}
}
#endif //DASH__UTIL__RANDOM_
