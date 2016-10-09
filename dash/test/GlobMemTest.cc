#include <libdash.h>
#include <gtest/gtest.h>

#include "TestBase.h"
#include "GlobMemTest.h"

TEST_F(GlobMemTest, PersistentMemory)
{
  {
    using value_t = int;
    using alloc_t =
      dash::allocator::CollectivePersistentAllocator<value_t>;
    using globmem_t = dash::GlobMem<value_t, alloc_t>;


    auto poolID = dash::util::random_str(8) + ".pmem";
    dash::default_size_t nlocal = 100;
    {
      alloc_t persistent_alloc = alloc_t{dash::Team::All(), poolID};
      globmem_t globmem{nlocal, persistent_alloc};

      auto lmem = globmem.lbegin();

      for (auto idx = 0; idx < nlocal; ++idx) {
        auto value = 1000 * (_dash_id + 1) + idx;

        DASH_LOG_TRACE("GlobMemTest.PersistentMemory",
                       "writing local offset", idx, "at unit",
                       "value:", value);
        lmem[idx] = value;
      }
    }

    {
      alloc_t persistent_alloc = alloc_t{dash::Team::All(), poolID};
      globmem_t globmem{nlocal, persistent_alloc};

      auto lmem = globmem.lbegin();

      for (auto idx = 0; idx < nlocal; ++idx) {
        auto expected = 1000 * (_dash_id + 1) + idx;
        auto actual = lmem[idx];

        DASH_LOG_TRACE("GlobMemTest.PersistentMemory",
                       "reading local offset", idx, "at unit",
                       "value:", actual);

        EXPECT_EQ_U(expected, actual);
      }
    }
  }
}
