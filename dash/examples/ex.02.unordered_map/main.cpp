#include <libdash.h>

#include <sstream>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <cstdlib>
#include <cassert>

using std::cout;
using std::endl;

#define COUT(expr) {             \
          std::ostringstream os; \
          os << expr;            \
          std::cout << os.str(); \
        } do { } while(0)

/**
 * Hash functor for mapping element keys to units.
 */
template<typename Key>
class MyHash
{
private:
  typedef dash::default_size_t size_type;

public:
  typedef Key                argument_type;
  typedef dash::team_unit_t result_type;

public:
  /**
   * Default constructor.
   */
  MyHash()
  : _team(nullptr),
    _nunits(0),
    _myid(DART_UNDEFINED_UNIT_ID)
  { }

  /**
   * Constructor.
   */
  MyHash(
    dash::Team & team)
  : _team(&team),
    _nunits(team.size()),
    _myid(team.myid())
  { }

  result_type operator()(
    const argument_type & key) const
  {
    if (_nunits == 0) {
      return result_type{0};
    }
    return result_type((key ^ 0xAA) % _nunits);
  }

private:
  dash::Team * _team   = nullptr;
  size_type    _nunits = 0;
  dash::team_unit_t _myid;

}; // class HashLocal

typedef int                                          key_t;
typedef double                                       mapped_t;
typedef MyHash<key_t>                                hash_t;
typedef dash::UnorderedMap<key_t, mapped_t, hash_t>  map_t;
typedef typename map_t::iterator                     map_iterator;
typedef typename map_t::value_type                   value_t;
typedef typename map_t::size_type                    size_type;


template<typename InputIt>
void print_map(InputIt first, InputIt last)
{
  int idx = 0;
  for (auto mit = first; mit != last; ++mit) {
    value_t elem = *mit;
    auto    lpos = mit.lpos();
    COUT(std::setw(3) << idx << ": "
         << "unit:" << std::setw(2) << lpos.unit  << ", "
         << "lidx:" << std::setw(3) << lpos.index << " "
         << "value:"
         << std::setw(5)
         << elem.first
         << " -> "
         << std::setprecision(3) << std::fixed
         << static_cast<mapped_t>(elem.second)
         << endl);
    idx++;
  }
}

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);

  dart_unit_t myid = dash::myid();
  size_t num_units = dash::Team::All().size();

  // Number of preallocated elements:
  size_type init_global_size  = 0;
  // Local buffer size determines initial local capacity and size of
  // new buckets that are allocated when local capacity is reached.
  // In effect, larger buffer sizes reduce allocation calls but may lead
  // to unused allocated memory.
  // Local buffer sizes may differ between units.
  size_type bucket_size       = (myid % 2 == 0) ? 5 : 7;

  size_type min_elem_per_unit =  5;
  size_type max_elem_per_unit = 12;
  if (argc >= 3) {
    min_elem_per_unit = static_cast<size_type>(atoi(argv[1]));
    max_elem_per_unit = static_cast<size_type>(atoi(argv[2]));
  }

  dash::UnorderedMap<key_t, mapped_t, hash_t> map(init_global_size,
                                                  bucket_size);

  if (myid == 0) {
    COUT(endl
         << "ex.02.unordered_map <min inserts> <max inserts>" << endl
         << "  min. number of elements inserted per unit: "
         << min_elem_per_unit << endl
         << "  max. number of elements inserted per unit: "
         << max_elem_per_unit << endl
         << endl
         << "Initial map size: " << map.size()
         << endl);
  }

  dash::barrier();

  COUT("Initial local map size (unit " << myid << "): " << map.lsize());

  dash::barrier();

  // fresh random numbers for every run and unit:
  std::srand(std::time(0) + myid);
  size_type num_add_elem = min_elem_per_unit + (
                            std::rand() %
                              (max_elem_per_unit - min_elem_per_unit));

  for (auto i = 0; i < num_add_elem; i++) {
    key_t    key    = 100 * (myid+1) + i;
    mapped_t mapped = 1.000 * (myid+1) + ((i+1) * 0.001);
    value_t  value  = std::make_pair(key, mapped);

    // Satisfies map concept as specified in STL
    auto insertion   = map.insert(value);
    bool key_existed = !insertion.second;
    auto inserted_it = insertion.first;
    assert(!key_existed);
    assert(map.count(key) == 1);

    mapped_t new_mapped_val = mapped + 400;
    // Access and modify mapped value directly:
    map[key] = new_mapped_val;

    // Read inserted value back:
    auto    read_it    = map.find(key);
    value_t read_value = *read_it;
    assert(read_it != map.end());

    assert(read_value.second == new_mapped_val);
  }
  // Wait for initialization of local values:
  dash::barrier();

  COUT("Local map size after inserts (unit " << myid << "): " << map.lsize());

  dash::barrier();

  if (myid == 0) {
    COUT(endl
         << "Map size before commit: " << map.size() << endl
         << "Elements accessible to unit 0 before commit: " << endl);
    print_map(map.begin(), map.end());
  }

  // Commit elements in local buffer and synchronize local memory spaces:
  map.barrier();

  if (myid == 0) {
    COUT(endl
         << "Size of map after commit: " << map.size() << endl
         << "Elements accessible to unit 0 after commit: ");
    print_map(map.begin(), map.end());
  }
}
