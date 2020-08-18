#include "utility/interp.hpp"

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

using namespace jhmi;

TEST_CASE( "Interpolation test?", "[volume_image]" ) {
  auto img = volume_image<double>{int3{2,2,2}, cube<m3>{m3{}, dbl3{2,2,2}*mm}};
  ranges::copy(ranges::view::ints(0,8), img.begin());

  REQUIRE(std::abs(lerp(img, dbl3{.7,.5,.5}*mm) - .2) < 1e-12);
  REQUIRE(std::abs(lerp(img, dbl3{.7,1.5,.5}*mm) - 2.2) < 1e-12);
  REQUIRE(std::abs(lerp(img, dbl3{.7,1.1,.5}*mm) - 1.4) < 1e-12);
  REQUIRE(std::abs(lerp(img, dbl3{.7,.5,1.5}*mm) - 4.2) < 1e-12);
  REQUIRE(std::abs(lerp(img, dbl3{.7,1.5,1.5}*mm) - 6.2) < 1e-12);
  REQUIRE(std::abs(lerp(img, dbl3{.7,1.1,1.5}*mm) - 5.4) < 1e-12);
  REQUIRE(std::abs(lerp(img, dbl3{.7,1.1,.9}*mm) - 3) < 1e-12);
}
