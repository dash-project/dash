#include <libdash.h>
#include <gtest/gtest.h>
#include <functional>

#include "TestBase.h"
#include "ForEachTest.h"

TEST_F(ForEachTest, TestArrayAllInvoked) {
    // Shared variable for total number of invoked callbacks:
    dash::SharedCounter<size_t> count_invokes;
    // Create for_each callback from member function:
    std::function<void(index_t)> invoke =
        std::bind(&ForEachTest::count_invoke, this, std::placeholders::_1);
    // Ensure value global counter is published to all units
    dash::Team::All().barrier();
    // Initialize global array:
    Array_t array(_num_elem);
    // Run for_each on complete array
    dash::for_each(array.begin(), array.end(), invoke);
    // Should have been invoked on every local index in the array:
    LOG_MESSAGE("Local number of inspected indices: %d",
                _invoked_indices.size());
    EXPECT_EQ(array.lsize(), _invoked_indices.size());
    // Count number of local invokes
    count_invokes.inc(_invoked_indices.size());
    // Wait for all units
    array.barrier();
    // Test number of total invokes
    size_t num_invoked_indices_all = count_invokes.get();
    LOG_MESSAGE("Total number of inspected indices: %d",
                num_invoked_indices_all);
    EXPECT_EQ(_num_elem, num_invoked_indices_all);
}

TEST_F(ForEachTest, ForEachWithIndex) {
    std::function<void(const Element_t &, index_t)> dummy_fct =
    [](Element_t el, index_t idx) {
				EXPECT_EQ_U(
					el,
					static_cast<Element_t>(dash::myid())
				);
    };

    // Test Array
    Array_t array(_num_elem);
		dash::fill(
						array.begin(),
						array.end(),
						static_cast<Element_t>(dash::myid()));
    dash::for_each_with_index(
        array.begin(),
        array.end(),
        dummy_fct
    );

    // Test Matrix
    auto matrix = dash::Matrix<Element_t, 2>(
                      dash::SizeSpec<2>(
                          dash::size(),
                          dash::size()));
		dash::fill(
						matrix.begin(),
						matrix.end(),
						static_cast<Element_t>(dash::myid()));

    dash::for_each_with_index(
        matrix.begin(),
        matrix.end(),
        dummy_fct
    );
}
