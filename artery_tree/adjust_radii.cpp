#include "liver/macrocell_tree.hpp"
#include "liver/print_tree_stats.hpp"
#include "utility/math.hpp"
#include "utility/options.hpp"
#include "utility/volume_image.hpp"
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

using namespace jhmi;

int main(int argc, char* argv[]) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  bool save_tree = false;
  auto opts = options<treefile_option, data_directory_option>{};
  opts.description().add_options()
      ("print-tree,p", boost::program_options::bool_switch(&save_tree));
  if (!opts.parse(argc, argv))
    return 1;

  auto liver = voxelized_shape{opts.data_directory() / "liver_extents.datz"};
  auto tree = macrocell_tree{load_tree, opts.treefile(), liver};
  auto terminal_nodes = tree.vessel_tree().terminal_vessel_nodes() | ranges::to_vector;
  auto get_avg_radius = [&] { return mean(terminal_nodes | ranges::view::transform([](auto n) {
        return n.value().radius(); })); };

  auto avg_goal = 11.8_um / 2.;
  auto avg_actual = get_avg_radius();
  fmt::print("Average radius desired: {}\n", avg_goal);
  fmt::print("Average radius actual: {}\n", avg_actual);

  while (abs(avg_actual - avg_goal) > 1_um) {
    tree.scale_terminal_vessels(avg_goal / avg_actual);

    avg_actual = get_avg_radius();
    fmt::print("Average radius actual: {}\n", avg_actual);
  }
  //tree.validate();
  print_tree_stats(tree, liver);

  if (save_tree)
    tree.write(opts.treefile().parent_path() / "vessel_tree.scaled.pbz");

  google::protobuf::ShutdownProtobufLibrary();
}

