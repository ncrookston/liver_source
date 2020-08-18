
#include "liver/fill_liver_volume.hpp"
#include "shape/box.hpp"
#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <fmt/ostream.h>
#include <range/v3/all.hpp>

using namespace jhmi;

TEST_CASE( "Lobules two ways", "[fill_liver_volume]" ) {
  auto shape = box{m3{}, dbl3{1,1,1} * 18_mm};

  auto lob1 = std::vector<m3>{};
  for_lobule(shape, [&](m3 const& pt, int3 const&) {
    lob1.push_back(pt);
  });
  auto lob2 = lobules_in(shape);

  RANGES_FOR(auto z, ranges::view::zip(lob1, lob2))
    REQUIRE(distance(z.first - z.second).value() < 1e-16);
}

#if 0
TEST_CASE( "Portal tracts two ways", "[fill_liver_volume]" ) {
  auto shape = box{m3{}, dbl3{1,1,1} * 18_mm};
  auto tract1 = std::vector<m3>{};
  for_portal_tract(shape, [&](m3 const& pt, int3 const&) {
    tract1.push_back(pt);
  });
  auto tract2 = tracts_in(shape);

  RANGES_FOR(auto z, ranges::view::zip(tract1, tract2))
    REQUIRE(distance(z.first - z.second).value() < 1e-16);
}
#endif

TEST_CASE( "Lobule near", "[fill_liver_volume]") {
  auto shape = box{m3{}, dbl3{1,1,1} * 18_mm};
  auto lobules = std::map<int3,m3>{};
  for_lobule(shape, [&](m3 const& pt, int3 const& ipt) {
    lobules[ipt] = pt;
  });

  auto pts = ranges::view::ints(2,35);
  RANGES_FOR(double i, pts) {
    auto near = find_near_lobule(shape, i * dbl3{1,1,1} * .5_mm);
    REQUIRE(near);
    REQUIRE(distance(near->first - lobules[near->second]).value() < 1e-16);
  }
}

TEST_CASE( "Tract near", "[fill_liver_volume]") {
  auto shape = box{m3{}, dbl3{1,1,1} * 18_mm};
  auto tracts = std::map<int3,m3>{};
  for_portal_tract(shape, [&](m3 const& pt, int3 const& ipt) {
    tracts[ipt] = pt;
  });

  auto pts = ranges::view::ints(2,35);
  RANGES_FOR(double i, pts) {
    auto near = find_near_tract(shape, i * dbl3{1,1,1} * .5_mm);
    REQUIRE(near);
    REQUIRE(distance(near->first - tracts[near->second]).value() < 1e-16);
  }
}
