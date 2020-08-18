
#include "liver/tract_tree.hpp"
#include "messages/sphere_locs.pb.h"
#include "utility/options.hpp"
#include "utility/load_protobuf.hpp"
#include "utility/math.hpp"
#include <fmt/format.h>

using namespace jhmi;

auto load_spherefile(boost::filesystem::path const& p) {
  return load_protobuf<jhmi_message::SphereLocs>(p).locations();
}

int main(int argc, char* argv[]) {
  auto opts = options<spherefiles_option, treefile_option>{};
  if (!opts.parse(argc, argv))
    return 1;

  auto tree = tract_tree{opts.treefile().string()};

  auto terminals = std::set<vessel_id>{};
  for (auto&& n : tree.vessel_nodes_postorder()) {
    if (!n.left_child())
      terminals.insert(n.value().id);
  }
  auto sphere_lists = opts.spherefiles() | ranges::view::transform(load_spherefile) | ranges::to_vector;

  std::size_t total = 0;
  std::size_t in_terminal = 0;
  RANGES_FOR(jhmi_message::SphereLoc const& s, sphere_lists | ranges::view::join) {
    ++total;
    if (terminals.find(vessel_id{s.vessel_id()}) != terminals.end())
      ++in_terminal;
  }

  fmt::print("Percent in terminal vessels: {:1.10f}\n", 100. * double(in_terminal) / total);

  return 0;
}

