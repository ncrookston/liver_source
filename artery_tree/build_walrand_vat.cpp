
#include "liver/fill_liver_volume.hpp"
#include "liver/walrand_tree.hpp"
#include "utility/git_hash.hpp"
#include "utility/options.hpp"
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <boost/program_options.hpp>
#include <fmt/ostream.h>
#include <chrono>
#include <iostream>
#include <random>

using namespace jhmi;
namespace fs = boost::filesystem;
namespace po = boost::program_options;

int main(int argc, char* argv[]) {
  try {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    auto opts = options<shapefile_option, vesselfile_option, seed_option, output_path_option>{};
    if (!opts.parse(argc, argv))
      return 1;
    auto eng = std::mt19937{opts.seed()};

    auto full_start = std::chrono::high_resolution_clock::now();
    //First, generate the locations of the portal tracts.
    auto liver = voxelized_shape{opts.shapefile(), adjust::do_open};
    auto pts = tracts_in(liver) | ranges::to_vector;
    //Next, load the initial vessel tree
    auto vtree = walrand_tree{build_tree, opts.vesselfile().string(), liver};
    //Shuffle the locations randomly
    ranges::shuffle(pts, eng);
    //Then connect them one at a time by proceeding down the list.
    fmt::print("Connecting cells:\n0 ");
    int pct = 0;
    float val = 0;
    for (auto&& pt : pts) {
      val += 1;
      auto cpct = static_cast<int>(100*val / pts.size());
      if (cpct > pct) {
        pct = cpct;
        std::cout.flush();
        fmt::print("{} ", pct);
      }
      vtree.connect_cell(pt);
    }

    auto p = opts.output_path() / fmt::format("walrand_run_{}", opts.seed());
    fs::create_directories(p);
    vtree.write(p);
    std::ofstream f{(p / "build_parameters.txt").string()};
    if (!f.is_open())
      throw std::runtime_error("Can't write build parameter file.");
    f << "Git hash: " << git_hash() << std::endl;
    f << "Seed: " << opts.seed() << std::endl;
    auto full_stop = std::chrono::high_resolution_clock::now();
    fmt::print("Done in {} s!\n",
      std::chrono::duration<float>(full_stop - full_start).count());
    google::protobuf::ShutdownProtobufLibrary();
  }
  catch(std::exception const& e) {
    fmt::print("Caught exception: {}\n", e.what());
  }
  return 0;
}
