#include "utility/binary_tree.hpp"
#include <fmt/ostream.h>
#include <range/v3/algorithm.hpp>
#include <range/v3/numeric.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/transform.hpp>
#include <tuple>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

using namespace std::string_literals;
static jhmi::binary_tree<char> make_test_tree()
{
  return jhmi::binary_tree<char>{
                        {'f'},
                      {'b', 'g'},
          {'a',          'd',        nullptr,        'i'},
    {nullptr, nullptr, 'c', 'e', nullptr, nullptr, 'h'}};
}

TEST_CASE( "Does tree traversal work?", "[tree]" )
{
  jhmi::binary_tree<int> empty_tree1;
  REQUIRE(empty_tree1.empty());
  int sum = 0;
  RANGES_FOR(int i, empty_tree1 | jhmi::view::in_order)
  { sum += i; }
  REQUIRE(sum == 0);
#ifdef NDEBUG//Linux "error: debug information for auto is not yet supported"
  auto node = jhmi::binary_tree<int>::node{};
  auto const_node = jhmi::binary_tree<int>::const_node{};
  REQUIRE(!node);
  REQUIRE(!const_node);
#endif

  jhmi::binary_tree<int> empty_tree2{};
  REQUIRE(empty_tree2.empty());

  auto tree = make_test_tree();
  REQUIRE(!tree.empty());
  auto vals1 = ranges::accumulate(jhmi::in_order_view<char>(tree), ""s);
  REQUIRE(vals1 == "abcdefghi");
  auto vals2 = ranges::accumulate(tree | jhmi::view::pre_order, ""s);
  REQUIRE(vals2 == "fbadcegih");
  auto vals3 = ranges::accumulate(tree | jhmi::view::post_order, ""s);
  REQUIRE(vals3 == "acedbhigf");
  auto vals4 = ranges::accumulate(tree | jhmi::view::level_order, ""s);
  REQUIRE(vals4 == "fbgadiceh");
  auto vals5 = ranges::accumulate(tree | jhmi::view::pre_order
      | ranges::view::transform([](char c) -> char { return c + 1; }), ""s);
  REQUIRE(vals5 == "gcbedfhji");

  auto const& ctree = tree;
  auto vals6 = ranges::accumulate(ctree | jhmi::view::in_order, ""s);
  REQUIRE(vals6 == "abcdefghi");

  auto vals1b = ranges::accumulate(tree.root().left_child() | jhmi::view::in_order, ""s);
  REQUIRE(vals1b == "abcde");
  auto vals2b = ranges::accumulate(tree.root().left_child() | jhmi::view::pre_order, ""s);
  REQUIRE(vals2b == "badce");
  auto vals3b = ranges::accumulate(tree.root().left_child() | jhmi::view::post_order, ""s);
  REQUIRE(vals3b == "acedb");
  auto vals4b = ranges::accumulate(tree.root().left_child() | jhmi::view::level_order, ""s);
  REQUIRE(vals4b == "badce");

  tree.root().make_left_child_of('j');
  auto vals7 = ranges::accumulate(ctree | jhmi::view::in_order, ""s);
  REQUIRE(vals7 == "abcdefghij");

  tree.root().left_child().make_right_child_of('k');
  auto vals8 = ranges::accumulate(ctree | jhmi::view::in_order, ""s);
  REQUIRE(vals8 == "kabcdefghij");
}
