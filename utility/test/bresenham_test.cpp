#include "utility/bresenham.hpp"
#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <fmt/ostream.h>
#include <range/v3/algorithm.hpp>
#include <set>

using namespace jhmi;

TEST_CASE( "Bresenham for a few lines", "[bresenham]" ) {
  std::set<int3> q1, q2, q3, q4;
  bresenham(int3{0,0,0}, int3{5,4,2}, [&](int3 const& pt) { q1.insert(pt); });
  bresenham(int3{5,4,2}, int3{0,0,0}, [&](int3 const& pt) { q2.insert(pt); });
  auto result = std::vector<int3>{
    int3{0,0,0}, int3{1,1,0}, int3{2,2,1},
    int3{3,2,1}, int3{4,3,2}, int3{5,4,2}};
  REQUIRE( ranges::equal(q1, result) );
  REQUIRE( ranges::equal(q2, result) );
  bresenham(int3{0,0,0}, int3{-5,-4,-2}, [&](int3 const& pt) { q3.insert(pt); });
  auto result3 = std::vector<int3>{
    int3{-5,-4,-2}, int3{-4,-3,-2}, int3{-3,-2,-1},
    int3{-2,-2,-1}, int3{-1,-1,0}, int3{0,0,0}};
  REQUIRE( ranges::equal(q3, result3) );

  bresenham(int3{0,0,0}, int3{-5,4,2}, [&](int3 const& pt) { q4.insert(pt); });
  auto result4 = std::vector<int3>{
    int3{-5,4,2}, int3{-4,3,2}, int3{-3,2,1},
    int3{-2,2,1}, int3{-1,1,0}, int3{0,0,0}};
  REQUIRE( ranges::equal(q4, result4) );
}
