#include "utility/volume_image.hpp"
#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <boost/units/io.hpp>
#include <range/v3/algorithm.hpp>

using namespace jhmi;

TEST_CASE( "Checking by_location points", "[volume_image]" ) {
  auto hext = dbl3{3,2,1} * mm;
  volume_image<bool> v{int3{3,2,1}, cube<m3>{-hext, hext}};

  auto expected = std::vector<std::pair<int3, m3>>{
    {int3{0,0,0}, dbl3{-2,-1,0}*mm},
    {int3{1,0,0}, dbl3{ 0,-1,0}*mm},
    {int3{2,0,0}, dbl3{+2,-1,0}*mm},
    {int3{0,1,0}, dbl3{-2,+1,0}*mm},
    {int3{1,1,0}, dbl3{ 0,+1,0}*mm},
    {int3{2,1,0}, dbl3{+2,+1,0}*mm}};
  auto vloc = v | view::by_location;
  auto vb = ranges::begin(vloc);
  for (int i = 0; i < expected.size(); ++i) {
    auto actual = *vb++;
    //fmt::print("Points: {} {}\n", actual.image_loc, actual.physical_loc*1e3);
    REQUIRE(expected[i].first == actual.image_loc);
    REQUIRE(distance_squared(expected[i].second - actual.physical_loc).value() < 1e-9);
  }
}
TEST_CASE( "Checking equal_step points", "[volume_image]" ) {
  auto hext = dbl3{2,1.5,1} * mm;
  volume_image<bool> v{int3{4,3,2}, cube<m3>{-hext, hext}};

  auto expected = std::vector<std::pair<int3, m3>>{
    {int3{0,0,0}, dbl3{-1.5,-1,-0.5}*mm},
    {int3{1,0,0}, dbl3{-0.5,-1,-0.5}*mm},
    {int3{2,0,0}, dbl3{+0.5,-1,-0.5}*mm},
    {int3{3,0,0}, dbl3{+1.5,-1,-0.5}*mm},
    {int3{0,1,0}, dbl3{-1.5, 0,-0.5}*mm},
    {int3{1,1,0}, dbl3{-0.5, 0,-0.5}*mm},
    {int3{2,1,0}, dbl3{+0.5, 0,-0.5}*mm},
    {int3{3,1,0}, dbl3{+1.5, 0,-0.5}*mm},
    {int3{0,2,0}, dbl3{-1.5,+1,-0.5}*mm},
    {int3{1,2,0}, dbl3{-0.5,+1,-0.5}*mm},
    {int3{2,2,0}, dbl3{+0.5,+1,-0.5}*mm},
    {int3{3,2,0}, dbl3{+1.5,+1,-0.5}*mm},

    {int3{0,0,1}, dbl3{-1.5,-1,+0.5}*mm},
    {int3{1,0,1}, dbl3{-0.5,-1,+0.5}*mm},
    {int3{2,0,1}, dbl3{+0.5,-1,+0.5}*mm},
    {int3{3,0,1}, dbl3{+1.5,-1,+0.5}*mm},
    {int3{0,1,1}, dbl3{-1.5, 0,+0.5}*mm},
    {int3{1,1,1}, dbl3{-0.5, 0,+0.5}*mm},
    {int3{2,1,1}, dbl3{+0.5, 0,+0.5}*mm},
    {int3{3,1,1}, dbl3{+1.5, 0,+0.5}*mm},
    {int3{0,2,1}, dbl3{-1.5,+1,+0.5}*mm},
    {int3{1,2,1}, dbl3{-0.5,+1,+0.5}*mm},
    {int3{2,2,1}, dbl3{+0.5,+1,+0.5}*mm},
    {int3{3,2,1}, dbl3{+1.5,+1,+0.5}*mm}};
  auto vloc = v | view::equal_step(1_mm);
  auto vb = ranges::begin(vloc);
  for (int i = 0; i < expected.size(); ++i) {
    auto actual = *vb++;
    //fmt::print("Points: {} {}\n", actual.image_loc, actual.physical_loc*1e3);
    REQUIRE(expected[i].first == actual.image_loc);
    REQUIRE(distance_squared(expected[i].second - actual.physical_loc).value() < 1e-9);
  }
}
TEST_CASE( "Checking off-center equal_step points", "[volume_image]" ) {
  volume_image<bool> v{int3{6,6,6}, cube<m3>{dbl3{-2,-2,-2}*mm, dbl3{4,4,4}*mm}};

  auto expected = std::vector<std::pair<int3, m3>>{
    {int3{0,0,0}, dbl3{-1.5,-1.5,-1.5}*mm}};
  auto vloc = v | view::equal_step(1_mm) | ranges::to_vector;
//  for (auto loc : vloc)
//    fmt::print("{} {}\n", loc.image_loc, loc.physical_loc);
  REQUIRE(expected[0].first == vloc[0].image_loc);
  REQUIRE(distance_squared(expected[0].second - vloc[0].physical_loc).value() < 1e-9);
}

TEST_CASE( "Verifying write and read with gzip integrity" ) {
  volume_image<int> v1{int3{6,6,6}, cube<m3>{dbl3{-2,-2,-2}*mm, dbl3{4,4,4}*mm}};
  auto v1it = v1.begin();
  for (int i = 0; i < 216; ++i, ++v1it) {
    *v1it = i;
  }
  v1.write("tmp.datz");

  auto v2 = volume_image<int>{"tmp.datz"};
  auto v2it = v2.begin();
  for (int i = 0; i < 216; ++i, ++v2it) {
    REQUIRE(i == *v2it);
  }
}

TEST_CASE( "Testing Erosion/Dilation" ) {
  auto v1 = volume_image<int>{int3{3,3,3}, cube<m3>{m3{}, dbl3{3,3,3}*mm}};
  v1(1,1,1) = 1;
  v1(1,1,2) = 1;
  auto v2 = dilate(v1);
  for (auto&& p : v2 | view::by_image_location) {
    if (p.loc.z == 0) {
      if (p.loc.x == 1 && p.loc.y == 1)
        REQUIRE(p.value == 1);
      else
        REQUIRE(p.value == 0);
    }
    else {
      if (p.loc.x != 1 && p.loc.y != 1)
        REQUIRE(p.value == 0);
      else
        REQUIRE(p.value == 1);
    }
  }

  auto v3 = erode(v2);
  for (auto&& p : v3 | view::by_image_location) {
    if (p.loc == int3{1,1,1} || p.loc == int3{1,1,2})
      REQUIRE(p.value == 1);
    else
      REQUIRE(p.value == 0);
  }

  auto v4 = erode(v1);
  for (auto&& p : v4)
    REQUIRE(p == 0);
}

