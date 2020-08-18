#include "liver/macrocell_tree.hpp"
#include "liver/tree_stats.hpp"
#include "utility/options.hpp"
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

using namespace jhmi;

binary_const_node_t<physical_vessel> find_porta_hepatis_vessel(macrocell_tree const& tree) {
  auto n = ranges::front(tree.vessel_tree().vessel_nodes());
  auto& liver = tree.liver_shape();
  while (n) {
    if (!liver(n.value().start()) && liver(n.value().end()))
      break;
    if (!n.left_child())
      n = n.right_child();
    else if (!n.right_child())
      n = n.left_child();
    else if (n.left_child() && n.right_child()) {
      if (n.left_child().value().flow() >= n.right_child().value().flow())
        n = n.left_child();
      else
        n = n.right_child();
    }
  }
  return n;
}
m3 find_porta_hepatis_location(physical_vessel const& v, voxelized_shape const& liver) {
  auto left = v.start();
  auto right = v.end();
  auto test = (left + right) / 2.;
  while (distance(test-right) < 100_um) {
    if (liver(test)) {
      right = test;
    }
    else {
      left = test;
    }
    test = (left + right) / 2.;
  }
  return test;
}
m find_outside_length(binary_const_node_t<physical_vessel> n, m3 const& loc) {
  auto length = distance(n.value().start() - loc);
  n = n.parent();
  while (n) {
    length += n.value().distance();
  }
  return length;
}
int main(int argc, char* argv[]) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  auto opts = options<treefiles_option, data_directory_option>{};
  if (!opts.parse(argc, argv))
    return 1;
  auto liver = voxelized_shape{opts.data_directory() / "liver_extents.datz", adjust::do_open};
  auto liver_extents = extents(liver);
  fmt::print("Liver location (mm): {} {}\n", dbl3{1000. * liver_extents.ul()/meters},
                                             dbl3{1000. * liver_extents.lr()/meters});
  fmt::print("Liver volume: {}\n", liver.volume());
  auto lobule_volume = lobule::cell_thickness * lobule::side_length * lobule::min_radius * 3.;
  fmt::print("Lobule volume: {}\n", lobule_volume);
  fmt::print("Expected no. of lobules: {}\n", double(liver.volume() / lobule_volume));
  for (auto treefile : opts.treefiles()) {
    fmt::print("Tree at {}\n", treefile.string());
    auto tree = macrocell_tree{load_tree, treefile, liver};
#if 0
    fmt::print("here1\n");
    auto porta_hepatis = find_porta_hepatis_vessel(tree);
    if (!porta_hepatis) {
      fmt::print("Nothing in the liver?\n");
      return 1;
    }
    fmt::print("here2\n");
    auto start = find_porta_hepatis_location(porta_hepatis.value(), tree.liver_shape());
    fmt::print("here3\n");
    auto outside_length = find_outside_length(porta_hepatis, start);
    fmt::print("here4\n");
    auto intra_distances = std::vector<m>{};
    for (auto& cell : tree.macrocells().list()) {
      intra_distances.push_back(distance(cell.center - start));
    }
    auto mean_dist = mean(intra_distances);
    fmt::print("Avg distance PH to macrocells: {} ({})\nTotal: {}\n",
               mean_dist, sqrt(variance(intra_distances)), mean_dist + outside_length);
#endif
    auto ts = calc_tree_stats(tree, liver);
    print_tree_stats(ts);
    write_tree_stats(ts, treefile.parent_path());
  }
  google::protobuf::ShutdownProtobufLibrary();
}

