#include "distribution/concurrent.hpp"
#include "distribution/embolized.hpp"
#include "distribution/radiized.hpp"
#include "distribution/unembolized.hpp"
#include "messages/sphere_locs.pb.h"
#include "utility/git_hash.hpp"
#include "utility/options.hpp"
#include "utility/protobuf_zip_ostream.hpp"
#include "utility/volume_image.hpp"
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <fmt/ostream.h>
#include <range/v3/view.hpp>
#include <atomic>
#include <chrono>
#include <future>
#include <random>
#include <thread>
#include <unordered_map>

namespace fs = boost::filesystem;
namespace po = boost::program_options;
using namespace jhmi;

namespace {
  using distributor = std::function<std::pair<std::vector<std::pair<m3,vessel_id>>,std::vector<int>>(std::mt19937&&, std::atomic<int>&)>;

  auto write_sphere_clusters_bin(std::vector<int> const& clusters,
                                 boost::filesystem::path const& p) {
    auto file = (p / "clusters.bin").string();
    std::ofstream out{file, std::ios::binary};
    out.write(reinterpret_cast<char const*>(clusters.data()), clusters.size() * sizeof(int));
    return file;
  }
  auto write_sphere_locs_bin(std::vector<std::pair<m3,vessel_id>> const& spheres,
                             boost::filesystem::path const& p) {

    auto output = std::vector<double>(spheres.size() * 4);
    for (auto i : ranges::view::ints(size_t(0), spheres.size())) {
      auto& s = spheres[i];
      output[i * 4 + 0] = s.first.x.value();
      output[i * 4 + 1] = s.first.y.value();
      output[i * 4 + 2] = s.first.z.value();
      output[i * 4 + 3] = s.second.value();
    }

    auto file = (p / "sphere_locs.bin").string();
    std::ofstream out{file, std::ios::binary};
    out.write(reinterpret_cast<char*>(output.data()), output.size() * sizeof(double));
    return file;
  }
  auto write_sphere_locs_protobuf(std::vector<std::pair<m3,vessel_id>> const& spheres,
                                  boost::filesystem::path const& p) {
    jhmi_message::SphereLocs sl;
    for (auto& s : spheres) {
      auto l = sl.add_locations();
      l->set_x(s.first.x.value());
      l->set_y(s.first.y.value());
      l->set_z(s.first.z.value());
      l->set_vessel_id(s.second.value());
    }
    auto filename = std::string{"sphere_locs.1.pbz"};
    protobuf_zip_ostream output_stream{p / filename};
    if (!sl.SerializeToZeroCopyStream(output_stream.get()))
      throw std::runtime_error("Failed to write spherefiles.");
    fmt::print("Number of spheres written to {}: {}\n", filename, spheres.size());
    return filename;
  }
  void print_status(int idx, std::unique_ptr<std::atomic<int>[]> const& task_pcts, int num_tasks) {
    fmt::print("Report {}: ", idx);
    for (int j = 0; j < num_tasks; ++j)
      fmt::print("{} | ", int(task_pcts[j]));
    std::cout << std::endl;
  }
}//namespace
int main(int argc, char* argv[]) {
  try {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    auto opts = options<seed_option, treefile_option>{};

    std::string output_prefix, method;
    int usphere_count, tries, spheres_per_tract;
    double straight_ratio;
    std::vector<double> values;
    opts.description().add_options()
      ("num-microspheres,n",
         po::value(&usphere_count)->default_value(1'000'000),
         "number of microspheres to distribute via the arterial tree")
      ("straight_ratio", po::value(&straight_ratio)->default_value(.6),
         "Percentage of microspheres entering the straight vessel given equal flow")
      ("output-prefix", po::value(&output_prefix)->default_value(""),
         "The directory to store these particular results in")
      ("distribution-tries", po::value(&tries)->default_value(1),
         "allows running several distributions of a single tree, since loading"
         " time dominates the runtime")
      ("spheres-per-tract", po::value(&spheres_per_tract)->default_value(50),
         "number of spheres that fit into a single portal tract (unused for unembolized and concurrent)")
      ("method", po::value(&method), "Type of distribution to perform, either\n"
         " unembolized: previous spheres have no effect on later spheres, all arrive in portal tracts\n"
         " embolized: flow is adjusted per-sphere, all arrive in portal tracts\n"
         " radiized: flow is adjusted per-sphere, spheres may stop before arriving in portal tracts\n"
         " unradiized: flow is constant, but spheres may stop before arriving in portal tracts\n"
         " concurrent: spheres stop flow, but many are dispatched concurrently, spheres may stop before arriving in portal tracts")
      ("values", po::value(&values)->multitoken(), "Values depend on method type:\n"
         " unembolized: the average diameter of the particles in microns\n"
         " embolized: the average diameter of the particles in microns\n"
         " radiized: the 5th percentile, 95th percentile, and maximum particle size sampling from a truncated Gaussian in microns\n"
         " unradiized: the 5th percentile, 95th percentile, and maximum particle size sampling from a truncated Gaussian in microns\n"
         " concurrent: the 5th percentile, 95th percentile, and maximum particle size sampling from a truncated Gaussian in microns");

    if (!opts.parse(argc, argv))
      return 1;
    if (values.size() != 1 && values.size() != 3) {
      throw std::invalid_argument("Invalid set of values given: expected 1 or 3 arguments");
    }
    ranges::sort(values);
    auto min90 = values.size() == 1 ? values[0] * um : values[0] * um;
    auto max90 = values.size() == 1 ? values[0] * um : values[1] * um;
    auto max_diameter = values.size() > 2 ? boost::optional<m>{values[2] * um} : boost::optional<m>{boost::none};
    auto tree = tract_tree{opts.treefile().string()};
    auto dist = distributor{};

    if (method == "unembolized") {
      dist = [=,&tree](std::mt19937&& rng, std::atomic<int>& pct_done) {
        return unembolized(tree, rng, usphere_count, straight_ratio, pct_done);
      };
    }
    else if (method == "embolized") {
      dist = [=,&tree](std::mt19937&& rng, std::atomic<int>& pct_done) {
        return embolized(tree, rng, usphere_count, spheres_per_tract, straight_ratio, pct_done);
      };
    }
    else if (method == "radiized") {
      dist = [=,&tree](std::mt19937&& rng, std::atomic<int>& pct_done) {
        return radiized(tree, rng, usphere_count, spheres_per_tract, straight_ratio,
                        min90, max90, max_diameter, pct_done);
      };
    }
    else if (method == "unradiized") {
      dist = [=,&tree](std::mt19937&& rng, std::atomic<int>& pct_done) {
        return radiized(tree, rng, usphere_count,
                        std::numeric_limits<double>::infinity(), straight_ratio,
                        min90, max90, max_diameter, pct_done);
      };
    }
    else if (method == "concurrent") {
      dist = [=,&tree](std::mt19937&& rng, std::atomic<int>& pct_done) {
        return concurrent(tree, rng, usphere_count, straight_ratio,
                          min90, max90, max_diameter, pct_done);
      };
    }
    else {
      throw std::invalid_argument(fmt::format("Unknown method requested: {}", method));
    }

    auto seed = opts.seed();
    std::random_device rd;
    std::vector<std::future<void>> tasks;
    std::unique_ptr<std::atomic<int>[]> task_pcts(new std::atomic<int>[tries]());

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < tries; ++i) {
      auto output_directory = opts.treefile().parent_path() / output_prefix
        / fmt::format("usphere_{}_{}_{}", usphere_count, seed, method);
      fs::create_directories(output_directory);
      std::ofstream out{(output_directory / "parameters.txt").string()};
      out << fmt::format("Git hash: {}\n", git_hash());
      out << fmt::format("Random seed: {}\n", seed);
      auto& pct_done = task_pcts[i];
      tasks.push_back(std::async(std::launch::async, [=, &pct_done] {
        auto [sphere_locs,clusters] = dist(std::mt19937{seed}, pct_done);
        write_sphere_locs_bin(sphere_locs, output_directory);
        write_sphere_clusters_bin(clusters, output_directory);
        //write_sphere_locs_protobuf(sphere_locs, output_directory);
      }));
      seed = rd();
    }
    int idx = 0;
    for (int i = 0; i < tries; ++i) {
      while (tasks[i].wait_for(std::chrono::seconds(30)) != std::future_status::ready)
        print_status(idx++, task_pcts, tries);
      tasks[i].get();
    }
    print_status(idx, task_pcts, tries);
    auto end = std::chrono::high_resolution_clock::now();
    fmt::print("Done in {} s!\n", std::chrono::duration<float>(end - start).count());
  }
  catch(std::exception const& e) {
    fmt::print("Error running program: {}\n", e.what());
    return 1;
  }

  google::protobuf::ShutdownProtobufLibrary();
  return 0;
}
