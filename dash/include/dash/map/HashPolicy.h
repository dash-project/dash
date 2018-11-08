#ifndef DASH__MAP__HASH_POLICY_H__INCLUDED
#define DASH__MAP__HASH_POLICY_H__INCLUDED

#include <dash/Team.h>

namespace dash {
template <typename Key>
class HashLocal {
 private:
  typedef dash::default_size_t size_type;

 public:
  typedef Key argument_type;
  typedef team_unit_t result_type;

 public:
  /**
   * Default constructor.
   */
  HashLocal()
    : _team(nullptr)
    , _nunits(0)
    , _myid(DART_UNDEFINED_UNIT_ID)
  {
  }

  /**
   * Constructor.
   */
  HashLocal(dash::Team& team)
    : _team(&team)
    , _nunits(team.size())
    , _myid(team.myid())
  {
  }

  result_type operator()(const argument_type& key) const { return _myid; }
 private:
  dash::Team* _team = nullptr;
  size_type _nunits = 0;
  team_unit_t _myid;
};  // class HashLocal

namespace detail {

struct HashNodeBase {
  HashNodeBase* _next;

  HashNodeBase() noexcept
    : _next()
  {
  }
  HashNodeBase(HashNodeBase* other) noexcept
    : _next(other)
  {
  }
};

template <typename V>
struct HashNode {
  V _val;

  HashNode* next() const noexcept { return static_cast<HashNode *>(this->_next); }

  V val() noexcept {
    return _val;
  }
};

struct prime_number_hash_policy {
  size_t index_for_hash(size_t hash, size_t /*num_slots_minus_one*/) const
  {
    switch (prime_index) {
      case 0:
        return 0llu;
      case 1:
        return hash % 2llu;
      case 2:
        return hash % 3llu;
      case 3:
        return hash % 5llu;
      case 4:
        return hash % 7llu;
      case 5:
        return hash % 11llu;
      case 6:
        return hash % 13llu;
      case 7:
        return hash % 17llu;
      case 8:
        return hash % 23llu;
      case 9:
        return hash % 29llu;
      case 10:
        return hash % 37llu;
      case 11:
        return hash % 47llu;
      case 12:
        return hash % 59llu;
      case 13:
        return hash % 73llu;
      case 14:
        return hash % 97llu;
      case 15:
        return hash % 127llu;
      case 16:
        return hash % 151llu;
      case 17:
        return hash % 197llu;
      case 18:
        return hash % 251llu;
      case 19:
        return hash % 313llu;
      case 20:
        return hash % 397llu;
      case 21:
        return hash % 499llu;
      case 22:
        return hash % 631llu;
      case 23:
        return hash % 797llu;
      case 24:
        return hash % 1009llu;
      case 25:
        return hash % 1259llu;
      case 26:
        return hash % 1597llu;
      case 27:
        return hash % 2011llu;
      case 28:
        return hash % 2539llu;
      case 29:
        return hash % 3203llu;
      case 30:
        return hash % 4027llu;
      case 31:
        return hash % 5087llu;
      case 32:
        return hash % 6421llu;
      case 33:
        return hash % 8089llu;
      case 34:
        return hash % 10193llu;
      case 35:
        return hash % 12853llu;
      case 36:
        return hash % 16193llu;
      case 37:
        return hash % 20399llu;
      case 38:
        return hash % 25717llu;
      case 39:
        return hash % 32401llu;
      case 40:
        return hash % 40823llu;
      case 41:
        return hash % 51437llu;
      case 42:
        return hash % 64811llu;
      case 43:
        return hash % 81649llu;
      case 44:
        return hash % 102877llu;
      case 45:
        return hash % 129607llu;
      case 46:
        return hash % 163307llu;
      case 47:
        return hash % 205759llu;
      case 48:
        return hash % 259229llu;
      case 49:
        return hash % 326617llu;
      case 50:
        return hash % 411527llu;
      case 51:
        return hash % 518509llu;
      case 52:
        return hash % 653267llu;
      case 53:
        return hash % 823117llu;
      case 54:
        return hash % 1037059llu;
      case 55:
        return hash % 1306601llu;
      case 56:
        return hash % 1646237llu;
      case 57:
        return hash % 2074129llu;
      case 58:
        return hash % 2613229llu;
      case 59:
        return hash % 3292489llu;
      case 60:
        return hash % 4148279llu;
      case 61:
        return hash % 5226491llu;
      case 62:
        return hash % 6584983llu;
      case 63:
        return hash % 8296553llu;
      case 64:
        return hash % 10453007llu;
      case 65:
        return hash % 13169977llu;
      case 66:
        return hash % 16593127llu;
      case 67:
        return hash % 20906033llu;
      case 68:
        return hash % 26339969llu;
      case 69:
        return hash % 33186281llu;
      case 70:
        return hash % 41812097llu;
      case 71:
        return hash % 52679969llu;
      case 72:
        return hash % 66372617llu;
      case 73:
        return hash % 83624237llu;
      case 74:
        return hash % 105359939llu;
      case 75:
        return hash % 132745199llu;
      case 76:
        return hash % 167248483llu;
      case 77:
        return hash % 210719881llu;
      case 78:
        return hash % 265490441llu;
      case 79:
        return hash % 334496971llu;
      case 80:
        return hash % 421439783llu;
      case 81:
        return hash % 530980861llu;
      case 82:
        return hash % 668993977llu;
      case 83:
        return hash % 842879579llu;
      case 84:
        return hash % 1061961721llu;
      case 85:
        return hash % 1337987929llu;
      case 86:
        return hash % 1685759167llu;
      case 87:
        return hash % 2123923447llu;
      case 88:
        return hash % 2675975881llu;
      case 89:
        return hash % 3371518343llu;
      case 90:
        return hash % 4247846927llu;
      case 91:
        return hash % 5351951779llu;
      case 92:
        return hash % 6743036717llu;
      case 93:
        return hash % 8495693897llu;
      case 94:
        return hash % 10703903591llu;
      case 95:
        return hash % 13486073473llu;
      case 96:
        return hash % 16991387857llu;
      case 97:
        return hash % 21407807219llu;
      case 98:
        return hash % 26972146961llu;
      case 99:
        return hash % 33982775741llu;
      case 100:
        return hash % 42815614441llu;
      case 101:
        return hash % 53944293929llu;
      case 102:
        return hash % 67965551447llu;
      case 103:
        return hash % 85631228929llu;
      case 104:
        return hash % 107888587883llu;
      case 105:
        return hash % 135931102921llu;
      case 106:
        return hash % 171262457903llu;
      case 107:
        return hash % 215777175787llu;
      case 108:
        return hash % 271862205833llu;
      case 109:
        return hash % 342524915839llu;
      case 110:
        return hash % 431554351609llu;
      case 111:
        return hash % 543724411781llu;
      case 112:
        return hash % 685049831731llu;
      case 113:
        return hash % 863108703229llu;
      case 114:
        return hash % 1087448823553llu;
      case 115:
        return hash % 1370099663459llu;
      case 116:
        return hash % 1726217406467llu;
      case 117:
        return hash % 2174897647073llu;
      case 118:
        return hash % 2740199326961llu;
      case 119:
        return hash % 3452434812973llu;
      case 120:
        return hash % 4349795294267llu;
      case 121:
        return hash % 5480398654009llu;
      case 122:
        return hash % 6904869625999llu;
      case 123:
        return hash % 8699590588571llu;
      case 124:
        return hash % 10960797308051llu;
      case 125:
        return hash % 13809739252051llu;
      case 126:
        return hash % 17399181177241llu;
      case 127:
        return hash % 21921594616111llu;
      case 128:
        return hash % 27619478504183llu;
      case 129:
        return hash % 34798362354533llu;
      case 130:
        return hash % 43843189232363llu;
      case 131:
        return hash % 55238957008387llu;
      case 132:
        return hash % 69596724709081llu;
      case 133:
        return hash % 87686378464759llu;
      case 134:
        return hash % 110477914016779llu;
      case 135:
        return hash % 139193449418173llu;
      case 136:
        return hash % 175372756929481llu;
      case 137:
        return hash % 220955828033581llu;
      case 138:
        return hash % 278386898836457llu;
      case 139:
        return hash % 350745513859007llu;
      case 140:
        return hash % 441911656067171llu;
      case 141:
        return hash % 556773797672909llu;
      case 142:
        return hash % 701491027718027llu;
      case 143:
        return hash % 883823312134381llu;
      case 144:
        return hash % 1113547595345903llu;
      case 145:
        return hash % 1402982055436147llu;
      case 146:
        return hash % 1767646624268779llu;
      case 147:
        return hash % 2227095190691797llu;
      case 148:
        return hash % 2805964110872297llu;
      case 149:
        return hash % 3535293248537579llu;
      case 150:
        return hash % 4454190381383713llu;
      case 151:
        return hash % 5611928221744609llu;
      case 152:
        return hash % 7070586497075177llu;
      case 153:
        return hash % 8908380762767489llu;
      case 154:
        return hash % 11223856443489329llu;
      case 155:
        return hash % 14141172994150357llu;
      case 156:
        return hash % 17816761525534927llu;
      case 157:
        return hash % 22447712886978529llu;
      case 158:
        return hash % 28282345988300791llu;
      case 159:
        return hash % 35633523051069991llu;
      case 160:
        return hash % 44895425773957261llu;
      case 161:
        return hash % 56564691976601587llu;
      case 162:
        return hash % 71267046102139967llu;
      case 163:
        return hash % 89790851547914507llu;
      case 164:
        return hash % 113129383953203213llu;
      case 165:
        return hash % 142534092204280003llu;
      case 166:
        return hash % 179581703095829107llu;
      case 167:
        return hash % 226258767906406483llu;
      case 168:
        return hash % 285068184408560057llu;
      case 169:
        return hash % 359163406191658253llu;
      case 170:
        return hash % 452517535812813007llu;
      case 171:
        return hash % 570136368817120201llu;
      case 172:
        return hash % 718326812383316683llu;
      case 173:
        return hash % 905035071625626043llu;
      case 174:
        return hash % 1140272737634240411llu;
      case 175:
        return hash % 1436653624766633509llu;
      case 176:
        return hash % 1810070143251252131llu;
      case 177:
        return hash % 2280545475268481167llu;
      case 178:
        return hash % 2873307249533267101llu;
      case 179:
        return hash % 3620140286502504283llu;
      case 180:
        return hash % 4561090950536962147llu;
      case 181:
        return hash % 5746614499066534157llu;
      case 182:
        return hash % 7240280573005008577llu;
      case 183:
        return hash % 9122181901073924329llu;
      case 184:
        return hash % 11493228998133068689llu;
      case 185:
        return hash % 14480561146010017169llu;
      case 186:
        return hash % 18446744073709551557llu;
      default:
        return hash;
    }
  }
  uint8_t next_size_over(size_t& size) const
  {
    // prime numbers generated by the following method:
    // 1. start with a prime p = 2
    // 2. go to wolfram alpha and get p = NextPrime(2 * p)
    // 3. repeat 2. until you overflow 64 bits
    // you now have large gaps which you would hit if somebody called reserve()
    // with an unlucky number.
    // 4. to fill the gaps for every prime p go to wolfram alpha and get
    // ClosestPrime(p * 2^(1/3)) and ClosestPrime(p * 2^(2/3)) and put those in
    // the gaps
    // 5. get PrevPrime(2^64) and put it at the end
    //
    // clang-format off
        static constexpr const size_t prime_list[] =
        {
            2llu, 3llu, 5llu, 7llu, 11llu, 13llu, 17llu, 23llu, 29llu, 37llu, 47llu,
            59llu, 73llu, 97llu, 127llu, 151llu, 197llu, 251llu, 313llu, 397llu,
            499llu, 631llu, 797llu, 1009llu, 1259llu, 1597llu, 2011llu, 2539llu,
            3203llu, 4027llu, 5087llu, 6421llu, 8089llu, 10193llu, 12853llu, 16193llu,
            20399llu, 25717llu, 32401llu, 40823llu, 51437llu, 64811llu, 81649llu,
            102877llu, 129607llu, 163307llu, 205759llu, 259229llu, 326617llu,
            411527llu, 518509llu, 653267llu, 823117llu, 1037059llu, 1306601llu,
            1646237llu, 2074129llu, 2613229llu, 3292489llu, 4148279llu, 5226491llu,
            6584983llu, 8296553llu, 10453007llu, 13169977llu, 16593127llu, 20906033llu,
            26339969llu, 33186281llu, 41812097llu, 52679969llu, 66372617llu,
            83624237llu, 105359939llu, 132745199llu, 167248483llu, 210719881llu,
            265490441llu, 334496971llu, 421439783llu, 530980861llu, 668993977llu,
            842879579llu, 1061961721llu, 1337987929llu, 1685759167llu, 2123923447llu,
            2675975881llu, 3371518343llu, 4247846927llu, 5351951779llu, 6743036717llu,
            8495693897llu, 10703903591llu, 13486073473llu, 16991387857llu,
            21407807219llu, 26972146961llu, 33982775741llu, 42815614441llu,
            53944293929llu, 67965551447llu, 85631228929llu, 107888587883llu,
            135931102921llu, 171262457903llu, 215777175787llu, 271862205833llu,
            342524915839llu, 431554351609llu, 543724411781llu, 685049831731llu,
            863108703229llu, 1087448823553llu, 1370099663459llu, 1726217406467llu,
            2174897647073llu, 2740199326961llu, 3452434812973llu, 4349795294267llu,
            5480398654009llu, 6904869625999llu, 8699590588571llu, 10960797308051llu,
            13809739252051llu, 17399181177241llu, 21921594616111llu, 27619478504183llu,
            34798362354533llu, 43843189232363llu, 55238957008387llu, 69596724709081llu,
            87686378464759llu, 110477914016779llu, 139193449418173llu,
            175372756929481llu, 220955828033581llu, 278386898836457llu,
            350745513859007llu, 441911656067171llu, 556773797672909llu,
            701491027718027llu, 883823312134381llu, 1113547595345903llu,
            1402982055436147llu, 1767646624268779llu, 2227095190691797llu,
            2805964110872297llu, 3535293248537579llu, 4454190381383713llu,
            5611928221744609llu, 7070586497075177llu, 8908380762767489llu,
            11223856443489329llu, 14141172994150357llu, 17816761525534927llu,
            22447712886978529llu, 28282345988300791llu, 35633523051069991llu,
            44895425773957261llu, 56564691976601587llu, 71267046102139967llu,
            89790851547914507llu, 113129383953203213llu, 142534092204280003llu,
            179581703095829107llu, 226258767906406483llu, 285068184408560057llu,
            359163406191658253llu, 452517535812813007llu, 570136368817120201llu,
            718326812383316683llu, 905035071625626043llu, 1140272737634240411llu,
            1436653624766633509llu, 1810070143251252131llu, 2280545475268481167llu,
            2873307249533267101llu, 3620140286502504283llu, 4561090950536962147llu,
            5746614499066534157llu, 7240280573005008577llu, 9122181901073924329llu,
            11493228998133068689llu, 14480561146010017169llu, 18446744073709551557llu
        };
    // clang-format on
    const size_t* found = std::lower_bound(std::begin(prime_list),
                                           std::end(prime_list) - 1, size);
    size = *found;
    return static_cast<uint8_t>(1 + found - prime_list);
  }

 private:
  uint8_t prime_index = 0;
};
}
}

#endif
