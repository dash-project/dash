#ifndef TEST_H_INCLUDED
#define TEST_H_INCLUDED

#include <assert.h>

#define EXPECT_TRUE( arg_ )  assert(arg_);
#define EXPECT_FALSE( arg_ ) assert( !(arg_) );

#define EXPECT_EQ( arg1_, arg2_ ) assert( (arg1_)==(arg2_) );
#define EXPECT_NE( arg1_, arg2_ ) assert( (arg1_)!=(arg2_) );
#define EXPECT_LT( arg1_, arg2_ ) assert( (arg1_)<(arg2_) );
#define EXPECT_LE( arg1_, arg2_ ) assert( (arg1_)<=(arg2_) );
#define EXPECT_GT( arg1_, arg2_ ) assert( (arg1_)>(arg2_) );
#define EXPECT_GE( arg1_, arg2_ ) assert( (arg1_)>=(arg2_) );

/*
EXPECT_STREQ
EXPECT_STRNE
EXPECT_STRCASEEQ
EXPECT_STRCASENE
*/

#endif /* TEST_H_INCLUDED */
