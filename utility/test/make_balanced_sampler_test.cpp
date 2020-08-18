
#include "utility/make_balanced_sampler.hpp"
#include <fmt/format.h>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

using namespace jhmi;

TEST_CASE( "Balanced sampler test", "[utility]" ) {
  std::mt19937 gen(10);
  auto s = std::vector<int>{0,1,1,1,1,2,2,3,3,3};
  auto dist = make_balanced_sampler(s);
  auto r = std::vector<size_t>(s.size(), 0);
  int n = 10'000'000;
  for (int i = 0; i < n; ++i)
    ++r[dist(gen)];
  double dn = n;
  REQUIRE(std::abs(r[0]/dn - .25/1) < 1e-3);
  REQUIRE(std::abs(r[1]/dn - .25/4) < 1e-3);
  REQUIRE(std::abs(r[2]/dn - .25/4) < 1e-3);
  REQUIRE(std::abs(r[3]/dn - .25/4) < 1e-3);
  REQUIRE(std::abs(r[4]/dn - .25/4) < 1e-3);
  REQUIRE(std::abs(r[5]/dn - .25/2) < 1e-3);
  REQUIRE(std::abs(r[6]/dn - .25/2) < 1e-3);
  REQUIRE(std::abs(r[7]/dn - .25/3) < 1e-3);
  REQUIRE(std::abs(r[8]/dn - .25/3) < 1e-3);
  REQUIRE(std::abs(r[9]/dn - .25/3) < 1e-3);
#if 0
  fmt::print("Counts:");
  for (auto&& rn : r) fmt::print(" {}", rn);
  fmt::print("\nPcts: ");
  for (auto&& rn : r) fmt::print(" {}", rn / dn);
  fmt::print("\n");
#endif
}
