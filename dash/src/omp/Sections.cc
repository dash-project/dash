
#include <functional>

#include <dash/omp/Sections.h>

namespace dash {
namespace omp {

class SectionsImpl;

SectionsImpl *_current_sections;

void section(const std::function<void(void)>& f)
{
  _current_sections->section(f);
}

void sections(const std::function<void(void)>& f)
{
  SectionsImpl sectimpl;
  _current_sections = &sectimpl;
  f();
  sectimpl.execute();
  _current_sections = nullptr;
}




} // namespace omp
} // namespace dash
