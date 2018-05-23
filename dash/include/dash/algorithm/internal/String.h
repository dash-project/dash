#ifndef DASH__ALGORITHM__INTERNAL__STRING_H__INCLUDED
#define DASH__ALGORITHM__INTERNAL__STRING_H__INCLUDED

#include <string>


namespace dash {
namespace internal {

template< typename String >
std::string common_prefix(
  String first,
  String second)
{
  std::string prefix;

  if (first == "" || second == "") {
    return prefix;
  }
  for (int i = 0; i < first.length(); i++) {
    if (i >= second.length()) {
      break;
    }
    if (first[i] != second[i]) {
      break;
    }
    prefix.push_back(first[i]);
  }
  return prefix;
}

template< typename SeqContainer >
std::string common_prefix(
  const SeqContainer & strings)
{
  std::string prefix;

  if (strings.size() == 0) {
    return prefix;
  }

  prefix = *strings.begin();
  for (auto & s : strings) {
    prefix = common_prefix(prefix, s);
  }
  return prefix;
}

} // namespace internal
} // namespace dash

#endif // DASH__ALGORITHM__INTERNAL__STRING_H__INCLUDED
