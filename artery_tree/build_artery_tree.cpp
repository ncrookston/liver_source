#include "liver/macrocell_tree.hpp"
#include "liver/tree_stats.hpp"
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
    auto opts = options<data_directory_option, seed_option, output_path_option>{};
    int cycles = 0;
    m final_radius;
    double flow_ml_min, gamma, cell_pressure_mmHg;
    opts.description().add_options()
      ("cycles", po::value(&cycles)->default_value(15), "Number of growth/death cycles")
      ("final-radius", unit_value(&final_radius, 1e-3)->default_value(.5), "Final radius in millimeters")
      ("proper-flow", po::value(&flow_ml_min)->default_value(400), "Flow through the proper hepatic artery in mL / min")
      ("gamma", po::value(&gamma)->default_value(2.7), "Murray's bifurcation constant (2-3)")
      ("cell-pressure", po::value(&cell_pressure_mmHg)->default_value(25), "Pressure at macrocells");
    if (!opts.parse(argc, argv))
      return 1;

    auto vesselfile = opts.data_directory() / "vtree_cycle0.txt";
    auto liver = voxelized_shape{opts.data_directory() / "liver_extents.datz", adjust::do_open};
    auto full_start = std::chrono::high_resolution_clock::now();
    auto tree = macrocell_tree{build_tree, vesselfile, liver, opts.seed(), cubic_meters_per_second{flow_ml_min * mL / minutes}, gamma, Pa{cell_pressure_mmHg * mmHg}, true /*initial_fill*/};

    auto p = opts.output_path() / fmt::format("run_{}", opts.seed());
    fs::create_directories(p);
    tree.build(cycles, final_radius, p, "vessel_tree.{:02}.pbz");

    tree.write(p / "vessel_tree.pbz");
    auto ts = calc_tree_stats(tree, liver);
    write_tree_stats(ts, p);

    auto f = std::ofstream{(p / "build_parameters.txt").string()};
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
