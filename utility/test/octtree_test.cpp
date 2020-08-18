#include "utility/line.hpp"
#include "utility/octtree.hpp"
#include "utility/tagged_int.hpp"
#include <boost/optional/optional_io.hpp>
#include <fmt/ostream.h>
#include <range/v3/all.hpp>
#include <deque>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

using namespace jhmi;

struct test_tag {};
using test_id = jhmi::tagged_int<test_tag>;
struct ln {
  test_id id;
  line<m3> line;
};
std::ostream& operator<<(std::ostream& out, ln const& v) {
  out << v.id;
  return out;
}
cube<m3> extents(ln const& l) {
  return {l.line.p1, l.line.p2};
}
bool operator<(ln const& lhs, ln const& rhs) {
  return lhs.id < rhs.id;
}
auto distance_squared(ln const& l, m3 const& pt) {
  return distance_squared(l.line, pt);
}

TEST_CASE( "Can I add things to the tree?", "[octtree]" ) {
  octtree<ln> tree{cube<m3>{m3{},dbl3{5,5,5}*mm}, 2};
  std::deque<ln> l{
    ln{test_id{1}, line<m3>{dbl3{0,0,0}*mm, dbl3{1,1,1}*mm}},
    ln{test_id{2}, line<m3>{dbl3{3,3,3}*mm, dbl3{4,4,4}*mm}},
    ln{test_id{3}, line<m3>{dbl3{0,4,0}*mm, dbl3{4,0,4}*mm}}};
  auto find_nearest_id = [&](m3 const& pt) {
    auto dist = distance_squared(l[0].line, pt);
    auto id = l[0].id;
    for (auto&& lni : l) {
      auto nd = distance_squared(lni.line, pt);
      if (nd < dist) {
        nd = dist;
        id = lni.id;
      }
    }
    return id;
  };
  REQUIRE(tree.root().objs.empty());
  REQUIRE(!tree.root().sub_nodes);
  tree.add_item(l[0]);
  REQUIRE(tree.root().objs.size() == 1);
  REQUIRE(!tree.root().sub_nodes);
  tree.add_item(l[1]);
  REQUIRE(tree.root().objs.size() == 2);
  REQUIRE(!tree.root().sub_nodes);
  tree.add_item(l[2]);
  REQUIRE(tree.root().objs.size() == 1);
  REQUIRE(tree.root().sub_nodes);
  auto npt = dbl3{1,1,1.05}*mm;
  auto n1 = tree.find_nearest_item(npt);
  REQUIRE(n1);
  REQUIRE(n1->id == find_nearest_id(npt));
  tree.remove_item(l[0]);
  REQUIRE(tree.root().objs.begin()->id == l[2].id);
  REQUIRE(tree.root().sub_nodes[6].objs.begin()->id == l[1].id);
  l.pop_front();
  auto n2 = tree.find_nearest_item(npt);
  REQUIRE(n2);
  REQUIRE(n2->id == find_nearest_id(npt));
}

namespace jhmi {
  auto extents(m3 const& pt) { return cube<m3>{pt, pt}; }
}
TEST_CASE( "Do multiple items work?", "[octtree]" ) {
  namespace rv = ranges::view;
  namespace ra = ranges::action;
  auto tree = octtree<m3>{cube<m3>{m3{}, dbl3{5,5,5} * mm}};
  std::random_device rd;
  auto gen = std::mt19937{rd()};
  auto dist = std::uniform_real_distribution<>{};
  auto rand = [&] { return 5_mm * dist(gen); };
  auto pts = rv::generate_n([&] {
     return m3{rand(), rand(), rand()}; }, 100) | ranges::to_vector;

  for (auto&& p : pts)
    tree.add_item(p);

  auto cpt = dbl3{4,4,4} * mm;
  auto in_tree = tree.find_nearest_items(cpt, 1_mm) | ra::sort(std::less<>{});
  auto in_list = pts | rv::filter([&](m3 const& p) {
     return distance(p - cpt) < 1_mm; }) | ranges::to_vector | ra::sort(std::less<>{});
  REQUIRE(ranges::equal(in_tree, in_list));
}
