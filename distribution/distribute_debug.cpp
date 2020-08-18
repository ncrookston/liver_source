
#include "liver/tract_tree.hpp"
#include "messages/sphere_locs.pb.h"
#include "utility/options.hpp"
#include "utility/load_protobuf.hpp"
#include "utility/math.hpp"
#include "range/v3/algorithm.hpp"
#include <fmt/format.h>

using namespace jhmi;

int main(int argc, char* argv[]) {
  auto opts = options<spherefile_option, treefile_option>{};
  if (!opts.parse(argc, argv))
    return 1;

  auto spheres = load_protobuf<jhmi_message::SphereLocs>(opts.spherefile());
  auto tree = tract_tree{opts.treefile().string()};

  //Generate histogram, find the vessel with the most spheres
  auto vessel_counts = std::map<vessel_id, int>{};
  for (auto&& sl : spheres.locations()) {
    ++vessel_counts[vessel_id{sl.vessel_id()}];
  }

  using vi = std::pair<vessel_id,int>;
  auto maxel = ranges::accumulate(vessel_counts, vi(*vessel_counts.begin()), [](vi mx, vi const& tst) {
    return tst.second > mx.second ? tst : mx;
  });

  //Find maxel.first and report path back to the insertion point.
  auto pre_vessels = tree.vessel_nodes_preorder() | ranges::to_vector;
  auto vn = *ranges::find_if(pre_vessels, [&](auto n) { return n.value().id == maxel.first; });

  fmt::print("Most common vessel {} has {} spheres\n", maxel.first, maxel.second);
  for (; vn; vn = vn.parent()) {
    fmt::print(" Sub vessel {} radius {}\n", vn.value().id, vn.value().radius);
    if (vn.left_child())
      fmt::print("   Left {}: radius {} prob {}\n", vn.left_child().value().id, 1000.*vn.left_child().value().radius.value(), vn.left_child().value().flow);
    if (vn.right_child())
      fmt::print("   Right {}: radius {} prob {}\n", vn.right_child().value().id, 1000.*vn.right_child().value().radius.value(), vn.right_child().value().flow);
  }
}

