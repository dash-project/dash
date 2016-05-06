#ifdef DASH_ENABLE_HDF5

#include <libdash.h>
#include <gtest/gtest.h>

#include "TestBase.h"
#include "TestLogHelpers.h"
#include "HDF5.h"

#include "limits.h"

typedef int value_t;
typedef dash::Array<value_t, long> array_t;

/**
 * Cantors pairing function to map n-tuple to single number
 */
template <
    typename T,
    size_t ndim >
T cantorpi(std::array<T, ndim> tuple) {
    int cantor = 0;
    for (int i = 0; i < ndim - 1; i++) {
        T x = tuple[i];
        T y = tuple[i + 1];
        cantor += y + 0.5 * (x + y) * (x + y + 1);
    }
    return cantor;
}

/**
 * Fill a n-dimensional dash matrix with a signatures that contains
 * the global coordinates and a secret which can be the unit id
 * for example
 */
template <
    typename T,
    int ndim,
    typename IndexT,
    typename PatternT >
void fill_matrix(dash::Matrix<T, ndim, IndexT, PatternT> & matrix, T secret = 0) {
    std::function< void(const T &, IndexT)>
    f = [&matrix, &secret](T el, IndexT i) {
        auto coords = matrix.pattern().coords(i);
        // hack
        *(matrix.begin()+i) = cantorpi(coords) + secret;
    };
    dash::for_each_with_index(
        matrix.begin(),
        matrix.end(),
        f);
}

/**
 * Counterpart to fill_matrix which checks if the given matrix satisfies
 * the desired signature
 */
template <
    typename T,
    int ndim,
    typename IndexT,
    typename PatternT >
void verify_matrix(dash::Matrix<T, ndim, IndexT, PatternT> & matrix,
                   T secret = 0) {
    std::function< void(const T &, IndexT)>
    f = [&matrix, &secret](T el, IndexT i) {
        auto coords  = matrix.pattern().coords(i);
        auto desired = cantorpi(coords) + secret;
        ASSERT_EQ_U(
            desired,
            el);
    };
    dash::for_each_with_index(
        matrix.begin(),
        matrix.end(),
        f);
}


TEST_F(HDFTest, StoreLargeDashArray) {
    // Pattern for arr2 array
    size_t nunits           = dash::Team::All().size();
    size_t tilesize         = 512 * 512;
    size_t blocks_per_unit  = 32;
    size_t size             = nunits * tilesize * blocks_per_unit;
    long   mbsize_total     = (size * sizeof(value_t)) / (tilesize);
    long   mbsize_unit      = mbsize_total / nunits;

    // Add some randomness to the data
    std::srand(time(NULL));
    int local_secret = std::rand() % 1000;
    int myid         = dash::myid();

    {
        // Create first array
        array_t arr1(size, dash::TILE(tilesize));

        // Fill Array
        for (int i = 0; i < arr1.local.size(); i++) {
            arr1.local[i] = local_secret + myid + i;
        }

        dash::barrier();

        DASH_LOG_DEBUG("Estimated memory per rank: ", mbsize_unit, "MB");
        DASH_LOG_DEBUG("Estimated memory total: ", mbsize_total, "MB");
        DASH_LOG_DEBUG("Array filled, begin hdf5 store");

        dash::io::StoreHDF::write(arr1, "test.hdf5", "data");
        dash::barrier();
    }
    DASH_LOG_DEBUG("Array successfully written ");

    // Create second array
    dash::Array<value_t> arr2;
    dash::barrier();
    dash::io::StoreHDF::read(arr2, "test.hdf5", "data");

    dash::barrier();
    for (int i = 0; i < arr2.local.size(); i++) {
        ASSERT_EQ_U(
            static_cast<int>(arr2.local[i]),
            local_secret + myid + i);
    }
    dash::barrier();
}

TEST_F(HDFTest, StoreMultiDimMatrix) {
    typedef dash::TilePattern<2>  pattern_t;
    typedef dash::Matrix <
    value_t,
    2,
    long,
    pattern_t >          matrix_t;

    auto numunits =  dash::Team::All().size();
    dash::TeamSpec<2> team_spec(numunits, 1);
    team_spec.balance_extents();

    auto team_extent_x = team_spec.extent(0);
    auto team_extent_y = team_spec.extent(1);

    auto extend_x = 2 * 2 * team_extent_x;
    auto extend_y = 2 * 5 * team_extent_y;

    pattern_t pattern(
        dash::SizeSpec<2>(
            extend_x,
            extend_y),
        dash::DistributionSpec<2>(
            dash::TILE(2),
            dash::TILE(5)),
        team_spec);

    DASH_LOG_DEBUG("Pattern", pattern);

    int myid = dash::myid();
    {
        matrix_t mat1(pattern);
        dash::barrier();
        LOG_MESSAGE("Matrix created");

        // Fill Array
        for (int x = 0; x < pattern.local_extent(0); x++) {
            for (int y = 0; y < pattern.local_extent(1); y++) {
                mat1.local[x][y] = myid;
            }
        }
        dash::barrier();
        DASH_LOG_DEBUG("BEGIN STORE HDF");
        dash::io::StoreHDF::write(mat1, "test.hdf5", "data");
        DASH_LOG_DEBUG("END STORE HDF");
        dash::barrier();
    }
    dash::barrier();
}

TEST_F(HDFTest, StoreSUMMAMatrix) {
    typedef double  value_t;
    typedef long    index_t;

    auto myid         = dash::myid();
    auto num_units   = dash::Team::All().size();
    auto extent_cols = num_units;
    auto extent_rows = num_units;
    auto team_size_x = num_units;
    auto team_size_y = 1;

    // Adopted from SUMMA test case
    // Automatically deduce pattern type satisfying constraints defined by
    // SUMMA implementation:
    dash::SizeSpec<2> size_spec(extent_cols, extent_rows);
    dash::TeamSpec<2> team_spec(team_size_x, team_size_y);
    team_spec.balance_extents();

    LOG_MESSAGE("Initialize matrix pattern ...");
    auto pattern = dash::make_pattern <
                   dash::summa_pattern_partitioning_constraints,
                   dash::summa_pattern_mapping_constraints,
                   dash::summa_pattern_layout_constraints > (
                       size_spec,
                       team_spec);
    DASH_LOG_DEBUG("Pattern", pattern);

    {
        // Instantiate Matrix
        dash::Matrix<value_t, 2, index_t, decltype(pattern)> matrix_a(pattern);
        dash::barrier();

        fill_matrix(matrix_a, static_cast<double>(myid));
        dash::barrier();

        // Store Matrix
        dash::io::StoreHDF::write(matrix_a, "test.hdf5", "data");
        dash::barrier();

#if 0
        std::ostream cout;
        cout << std::setprecision(4) << std::setw(8)
             << std::left
             << (double)(4.5432)

             dash::HDF5OutputStream os("test.hdf5");

        os << dash::HDF5Table("data")
           << dash::HDF5::transposed
           << matrix_a;
        os.flush();

        matrix_t m;
        os >> m;



        dash::HDF5OutputStream os("test.hdf5");
#endif

        // Read HDF5 Matrix
    }

    dash::Matrix<double, 2> matrix_b;
    dash::io::StoreHDF::read(matrix_b, "test.hdf5", "data");
    dash::barrier();
    verify_matrix(matrix_b, static_cast<double>(myid));
}

TEST_F(HDFTest, Options) {
    {
        auto matrix_a = dash::Matrix<int, 2>(
                            dash::SizeSpec<2>(
                                dash::size(),
                                dash::size()));
        // Fill
        fill_matrix(matrix_a);
        dash::barrier();

        // Set option
        auto fopts = dash::io::StoreHDF::get_default_options();
        fopts.store_pattern = false;

        dash::io::StoreHDF::write(matrix_a, "test.hdf5", "data", fopts);
        dash::barrier();
    }
    dash::Matrix<int, 2> matrix_b;
    //dash::io::StoreHDF::read(matrix_b, "test.hdf5", "data");
    dash::barrier();

    // Verify
    //verify_matrix(matrix_b);
}
#if 0
TEST_F(HDFTest, OutputStream) {
    auto matrix = dash::Matrix<long, 2>(
                      dash::SizeSpec<2>(100, 100));

    auto fopts = dash::HDF5Options::getDefaults();
    auto test  = dash::HDF5OutputStream("test_stream.hdf5");
    test << dash::HDF5Table("data")
         << dash::HDF5Options(fopts)
         << matrix;
}
#endif

#endif // DASH_ENABLE_HDF5


