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
    auto opts = options<treefile_option>{};
    bool show_macrocells = false;
    opts.description().add_options()
      ("show-macrocells,m", po::bool_switch(&show_macrocells),
       "indicates that macrocells should be shown as well as vessels");
    if (!opts.parse(argc, argv))
      return 1;
    auto tree = macrocell_tree{load_tree, opts.treefile()};
    auto ctr = flt3{0.0992586,0.230315,0.165132};
    auto g = gl::make_gui(tree, show_macrocells, 1024, 768, ctr);
    while (g.process_events() != gl::gui::request_close)
      g.draw();
    google::protobuf::ShutdownProtobufLibrary();
  }
  catch (std::exception const& e) {
    fmt::print("Failed with exception: {}\n", e.what());
  }
  return 0;
}
