#include <gtest/gtest.h>

#include "filterclass.h"

TEST(filter_t, constructor)
{
  std::vector<double> A(1,1);
  std::vector<double> B(1,1);
  TASCAR::filter_t filter(A,B);
  EXPECT_EQ(1u,filter.get_len_A());
  EXPECT_EQ(1u,filter.get_len_B());
  EXPECT_EQ(1.0,filter.A[0]);
  EXPECT_EQ(1.0,filter.B[0]);
}

TEST(filter_t, copyconstructor)
{
  std::vector<double> A(1,1);
  std::vector<double> B(1,1);
  TASCAR::filter_t filter(A,B);
  EXPECT_EQ(1u,filter.get_len_A());
  EXPECT_EQ(1u,filter.get_len_B());
  EXPECT_EQ(1.0,filter.A[0]);
  EXPECT_EQ(1.0,filter.B[0]);
  TASCAR::filter_t filter2(filter);
  EXPECT_EQ(1u,filter2.get_len_A());
  EXPECT_EQ(1u,filter2.get_len_B());
  EXPECT_EQ(1.0,filter2.A[0]);
  EXPECT_EQ(1.0,filter2.B[0]);
}

TEST(filter_t, filter)
{
  std::vector<double> A(1,1);
  std::vector<double> B(1,1);
  TASCAR::filter_t filter(A,B);
  TASCAR::wave_t delta(1000);
  TASCAR::wave_t res(1000);
  delta[0] = 1;
  filter.filter(&res,&delta);
  EXPECT_EQ(1,delta[0]);
  EXPECT_EQ(0,delta[1]);
  EXPECT_EQ(1,res[0]);
  EXPECT_EQ(0,res[1]);
  B.push_back(1);
  TASCAR::filter_t filter2(A,B);
  filter2.filter(&res,&delta);
  EXPECT_EQ(1,delta[0]);
  EXPECT_EQ(0,delta[1]);
  EXPECT_EQ(1,res[0]);
  EXPECT_EQ(1,res[1]);
  EXPECT_EQ(0,res[2]);
}

// Local Variables:
// compile-command: "make -C ../.. unit-tests"
// coding: utf-8-unix
// c-basic-offset: 2
// indent-tabs-mode: nil
// End:
