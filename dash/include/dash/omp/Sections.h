#ifndef DASH__OMP_SECTIONS_H__INCLUDED
#define DASH__OMP_SECTIONS_H__INCLUDED

#include <deque>
#include <dash/omp/Mutex.h>
#include <dash/Pattern.h>

namespace dash {
namespace omp {


class SectionsImpl
{
private:
  Team&                                 _team;
  std::deque<std::function<void(void)>> _sections;

  static std::unordered_map<std::string, Mutex>  _mutexes;

public:
  /**
   * Create a new sections code block. Individual sections can be added using
   * section(), the sections block is finalized by calling execute().
   */
  SectionsImpl( Team& team = Team::All() ): 
    _team(team) {}
  
  /**
   * Add a section, i.e. a code block wrapped in a lambda function, to this
   * sections construct.
   */
  void section(std::function<void(void)> f) {
    _sections.push_back(f);
  }
  
  /**
   * Finish the sections construct by distributing all sections to available
   * team members and executing them.
   */
  void execute(bool wait = true) {
    dash::Pattern<1> pat(SizeSpec<1>(_sections.size()),
			 DistributionSpec<1>(CYCLIC),
			 _team);
    
    for (size_t i = 0; i < pat.local_size(); ++i) {
      (_sections[pat.global(i)])();
    }
    
    if (wait) _team.barrier();
  }
  
};


extern SectionsImpl *_current_sections;


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


}  // namespace omp
}  // namespace dash

#endif // DASH__OMP_SECTIONS_H__INCLUDED
