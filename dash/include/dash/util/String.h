#ifndef DASH__UTIL__STRING_H_
#define DASH__UTIL__STRING_H_

#include <iostream>
#include <vector>
#include <random>
#include <functional> //for std::function
#include <algorithm>  //for std::generate_n

namespace dash {
namespace util {
  /*

std::string random_string( size_t length, std::function<char(void)> rand_char )
{
  std::string str(length, 0);
  std::generate_n( str.begin(), length, rand_char );
  return str;
}

std::string random_string_uniform(size_t length)
{
  std::vector<char> const char_set ( {
    '0', '1', '2', '3', '4',
    '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E', 'F',
    'G', 'H', 'I', 'J', 'K',
    'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U',
    'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k',
    'l', 'm', 'n', 'o', 'p',
    'q', 'r', 's', 't', 'u',
    'v', 'w', 'x', 'y', 'z'
  });

  std::default_random_engine rng(std::random_device {}());
  std::uniform_int_distribution<> dist(0, char_set.size() - 1);
  auto randchar = [ char_set, &dist, &rng ] () {
    return char_set[ dist(rng) ];
  };
  return random_string(length, randchar);
}
*/

} //util
} //dash
#endif //DASH__UTIL__STRING_H_

