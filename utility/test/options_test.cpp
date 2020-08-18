#include "utility/options.hpp"
#include "utility/units.hpp"
#define CATCH_CONFIG_MAIN
#include <boost/optional/optional_io.hpp>
#include <catch.hpp>
#include <fmt/ostream.h>

using namespace jhmi;

TEST_CASE( "Do options work?", "[options]" ) {
  auto opts = options<output_path_option, shapefile_option>{};

  char const* argv1[] = {"options_test", "--output-path", "Hello", "--shapefile", "World!"};
  REQUIRE(opts.parse(5, argv1));
  REQUIRE(opts.output_path() == "Hello");
  REQUIRE(opts.shapefile() == "World!");
  int foo = 0;
  opts.description().add_options()("foo",
       boost::program_options::value<int>(&foo)->required(),
       "For foos everywhere");
  char const* argv2[] = {"options_test", "-o", "Hello", "-s", "World!", "--foo", "100"};
  REQUIRE(opts.parse(7, argv2));
  REQUIRE(opts.output_path() == "Hello");
  REQUIRE(opts.shapefile() == "World!");
  REQUIRE(foo == 100);

  m bar = 0;
  opts.description().add_options()("bar",
       unit_value(&bar, 1e-3)->default_value(5),
       "For bars everywhere");
  REQUIRE(opts.parse(7, argv2));
  REQUIRE(opts.output_path() == "Hello");
  REQUIRE(opts.shapefile() == "World!");
  REQUIRE(foo == 100);
  REQUIRE(abs(bar - 5_mm).value() < 1e-12);

  char const* argv3[] = {"options_test", "-o", "Hello", "-s", "World!", "--foo", "100", "--bar", "6"};
  REQUIRE(opts.parse(9, argv3));
  REQUIRE(opts.output_path() == "Hello");
  REQUIRE(opts.shapefile() == "World!");
  REQUIRE(foo == 100);
  REQUIRE(abs(bar - 6_mm).value() < 1e-12);
}

TEST_CASE( "Do optional options work?", "[options]") {
  auto opts = options<>{};
  auto foo = boost::optional<m>{};
  auto bar = boost::optional<int>{};
  opts.description().add_options()
    ("foo", option_unit_value(&foo, 1e-3)->default_value(4))
    ("bar", option_value(&bar));
  char const* argv[] = {"options_test", "--foo", "5", "--bar", "100"};
  REQUIRE(opts.parse(5, argv));
  REQUIRE(foo);
  REQUIRE(abs(*foo - 5_mm).value() < 1e-12);
  REQUIRE(bar);
  REQUIRE(*bar == 100);
}
TEST_CASE( "Do unsupplied optional options work as expected?", "[options]") {
  auto opts = options<>{};
  auto foo = boost::optional<m>{};
  auto bar = boost::optional<int>{};
  opts.description().add_options()
    ("foo", option_unit_value(&foo, 1e-3)->default_value(4))
    ("bar", option_value(&bar));
  char const* argv[] = {"options_test"};
  REQUIRE(opts.parse(1, argv));
  REQUIRE(foo);
  REQUIRE(abs(*foo - 4_mm).value() < 1e-12);
  REQUIRE(!bar);
}
