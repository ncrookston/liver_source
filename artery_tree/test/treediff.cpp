#include "liver/macrocell_tree.hpp"
#include "utility/volume_image.hpp"
#include <boost/filesystem.hpp>
#include <fmt/ostream.h>

using namespace jhmi;

int main(int argc, char* argv[]) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  if (argc != 3) {
    fmt::print("Usage: {} <path_to_1st_tree> <path_to_2nd_tree>\n", argv[0]);
    return 1;
  }
  auto tree1 = macrocell_tree{load_tree, argv[1]};
  auto tree2 = macrocell_tree{load_tree, argv[2]};
  google::protobuf::ShutdownProtobufLibrary();
  return tree1 == tree2 ? 0 : 1;
}
