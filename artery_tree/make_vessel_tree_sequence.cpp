#include "gl/visualizer.hpp"
#include "liver/macrocell_tree.hpp"
#include "utility/options.hpp"
#include "utility/volume_image.hpp"
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

namespace po = boost::program_options;
using namespace jhmi;

int main(int argc, char* argv[]) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  try {
    auto opts = options<treefiles_option>{};
    bool show_macrocells = false;
    opts.description().add_options()
      ("show-macrocells,m", po::bool_switch(&show_macrocells),
       "indicates that macrocells should be shown as well as vessels");
    if (!opts.parse(argc, argv))
      return 1;
    int idx = 0;
    auto ctr = flt3{0.0992586,0.230315,0.165132};
    {
      auto tree = macrocell_tree{build_tree, "../../data/vtree_cycle0.txt",
        voxelized_shape{"../../data/liver_extents.datz"}, 1, cubic_meters_per_second{0}, 2.7, 25_mmHg, false};
      auto g = gl::make_gui(tree, show_macrocells, 1024, 768, ctr);
      auto img = g.save_screen();
      img.saveToFile(fmt::format("tree_img_{:02}.bmp", idx++));
    }
    for (auto file : opts.treefiles()) {
      auto tree = macrocell_tree{load_tree, file};
      auto mcl = tree.macrocells().list();
      auto radius = ranges::begin(mcl)->radius;
      fmt::print("Cell radius for {}: {}\n", file, radius);
      //auto ctr = flt3{center(extents(tree.macrocells())) / meters};
      //fmt::print("Center: {}\n", ctr);
      auto g = gl::make_gui(tree, show_macrocells, 1024, 768, ctr);
      auto img = g.save_screen();
      img.saveToFile(fmt::format("tree_img_{:02}.bmp", idx++));
    }
    google::protobuf::ShutdownProtobufLibrary();
  }
  catch (std::exception const& e) {
    fmt::print("Failed with exception: {}\n", e.what());
  }
  return 0;
}

