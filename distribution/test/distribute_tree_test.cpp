#include "distribution/concurrent.hpp"
#define CATCH_CONFIG_MAIN
#include <catch.hpp>

using namespace jhmi;

TEST_CASE( "Testing simple tree", "[distribute]" ) {
  auto flow = 1. * meters * meters * meters / seconds;
  auto tt = tract_tree{binary_tree<flow_vessel>{
    {flow_vessel(vessel_id{1}, m3{}, dbl3{1,0,0}*mm, 1.5_mm, 1.*flow)},
    {flow_vessel(vessel_id{2}, dbl3{1,0,0}*mm, dbl3{2,0,0}*mm, 1499_um, .99*flow),
      flow_vessel(vessel_id{3}, dbl3{1,0,0}*mm, dbl3{2,1,0}*mm, 1_um, .01*flow)},
    {flow_vessel(vessel_id{4}, dbl3{2,0,0}*mm, dbl3{3,0,0}*mm, 700_um, .44*flow),
      flow_vessel(vessel_id{5}, dbl3{2,0,0}*mm, dbl3{3,1,0}*mm, 799_um, .55*flow),
      nullptr,
      nullptr},
    {flow_vessel(vessel_id{6}, dbl3{3,0,0}*mm, dbl3{4,0,0}*mm, 600_um, .40*flow),
      flow_vessel(vessel_id{7}, dbl3{3,0,0}*mm, dbl3{4,1,0}*mm, 30_um, .04*flow),
      flow_vessel(vessel_id{8}, dbl3{3,1,0}*mm, dbl3{4,2,0}*mm, 5_um, .22*flow),
      flow_vessel(vessel_id{9}, dbl3{3,1,0}*mm, dbl3{4,3,0}*mm, 10_um, .33*flow)}
    }};
  std::mt19937 eng;
  std::atomic<int> pct_done;
  auto [spheres,clusters] = concurrent(tt, eng, 2, .6, 15_um, 35_um, boost::none, pct_done);
  for (auto&& s : spheres) {
    fmt::print("Sphere at {} in vessel {}\n", dbl3{1000.*s.first/meters}, s.second);
  }
}
/*
  Sphere at [0.003 m,0 m,0 m] in vessel 7
  Sphere at [0.003 m,0 m,0 m] in vessel 7
  Sphere at [0.003 m,0 m,0 m] in vessel 7
  Sphere at [0.003 m,0.001 m,0 m] in vessel 5
  Sphere at [0.003 m,0.001 m,0 m] in vessel 5
  Sphere at [0.003 m,0.001 m,0 m] in vessel 5
  Sphere at [0.003 m,0.001 m,0 m] in vessel 5
  Sphere at [0.003 m,0.001 m,0 m] in vessel 5
  Sphere at [0.003 m,0.001 m,0 m] in vessel 9
  Sphere at [0.003 m,0.001 m,0 m] in vessel 9
*/
