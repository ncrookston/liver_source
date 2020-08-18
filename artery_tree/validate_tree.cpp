
#include "liver/macrocell_tree.hpp"
#include "utility/options.hpp"
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

namespace po = boost::program_options;
using namespace jhmi;

int main(int argc, char* argv[]) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  try {
    auto opts = options<treefile_option>{};
    if (!opts.parse(argc, argv))
      return 1;
    auto tree = macrocell_tree{load_tree, opts.treefile().string()};
    tree.validate();

    google::protobuf::ShutdownProtobufLibrary();
  }
  catch (std::exception const& e) {
    fmt::print("Failed with exception: {}\n", e.what());
  }
  return 0;
}

