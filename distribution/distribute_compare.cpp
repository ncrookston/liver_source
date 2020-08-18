#include "distribution/distribute_tree.hpp"
#include "messages/sphere_tracts.pb.h"
#include "utility/gaussian.hpp"
#include "utility/options.hpp"
#include "utility/volume_image.hpp"
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <fmt/ostream.h>
#include <random>
#include <unordered_map>

namespace fs = boost::filesystem;
namespace po = boost::program_options;
using namespace jhmi;

auto compare_distributions(tract_tree const& tree, double straight_ratio,
    std::mt19937& gen, int num_comparisons,
    m min90_0, m max90_0, m max0, m min90_1, m max90_1, m max1) {
  auto mv = distribute_tree<distribute_vessel>{tree, straight_ratio};
  auto nd0 = truncated_gaussian{gaussian_90_sampler{gen, min90_0, max90_0}, 0_mm, max0};
  auto nd1 = truncated_gaussian{gaussian_90_sampler{gen, min90_1, max90_1}, 0_mm, max1};

  auto seeds  = ranges::view::generate_n([&] { return gen(); }, num_comparisons) | ranges::to_vector;
  auto radii0 = ranges::view::generate_n([&] { return nd0(); }, num_comparisons) | ranges::to_vector;
  auto radii1 = ranges::view::generate_n([&] { return nd1(); }, num_comparisons) | ranges::to_vector;

  std::atomic<int> idx{0};
  return tbb::parallel_reduce(tbb::blocked_range<size_t>(0, num_comparisons), m3{},
    [&](tbb::blocked_range<size_t> const& prange, m3 diffs) {
      for(auto i = prange.begin(); i != prange.end(); ++i) {
        std::mt19937 tgen{seeds[i]};
        auto r0 = radii0[i], r1 = radii1[i];

        auto urd = std::uniform_real_distribution<>{};
        auto branch_rand = std::bind(urd, std::ref(tgen));

        boost::optional<m3> pt0, pt1;
        m3 pt;
        mv.traverse_individual([&](distribute_vessel& dv, flow_vessel const& fv,
              distribute_vessel const* left, distribute_vessel const* right) {
          pt = fv.start;
          if (!pt0 && (!left || fv.radius < r0))
            pt0 = fv.start;
          if (!pt1 && (!left || fv.radius < r1))
            pt1 = fv.start;

          if (right && branch_rand() > left->p)
            return distribute_tree<distribute_vessel>::traversal::right;
          else if (left)
            return distribute_tree<distribute_vessel>::traversal::left;
          assert(pt0 && pt1);
          return distribute_tree<distribute_vessel>::traversal::abort;
        });
        assert(pt0 && pt1);
        diffs.x += distance(pt - *pt0);
        diffs.y += distance(pt - *pt1);
        diffs.z += distance(*pt0 - *pt1);
        auto curridx = ++idx;
        if (curridx % 10000 == 0)
          fmt::print("{} %\n", (100. * curridx) / num_comparisons);
      }
      return diffs;
    }, std::plus<>{}) / double(num_comparisons);
}

int main(int argc, char* argv[]) {
  try {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    auto opts = options<seed_option, treefile_option>{};
    int comparisons;
    m min90_0, max90_0, max_0, min90_1, max90_1, max_1;
    double straight_ratio;
    opts.description().add_options()
      ("comparisons,c", po::value(&comparisons)->default_value(10'000'000),
         "number of particle distributions to compare.")
      ("percentage", po::value(&straight_ratio)->default_value(.6),
         "Percentage of microspheres entering the straight vessel given equal flow")
      ("min90_0", unit_value(&min90_0, 1e-6)->required(),
         "diameter in um of 5th percentile of a normal sphere distribution")
      ("max90_0", unit_value(&max90_0, 1e-6)->required(),
         "diameter in um of 95th percentile of a normal sphere distribution")
      ("max_0", unit_value(&max_0, 1e-6)->required(),
         "maximum diameter in um of a normal sphere distribution")
      ("min90_1", unit_value(&min90_1, 1e-6)->required(),
         "diameter in um of 5th percentile of a normal sphere distribution")
      ("max90_1", unit_value(&max90_1, 1e-6)->required(),
         "diameter in um of 95th percentile of a normal sphere distribution")
      ("max_1", unit_value(&max_1, 1e-6)->required(),
         "maximum diameter in um of a normal sphere distribution");

    if (!opts.parse(argc, argv))
      return 1;

    auto tree = tract_tree{opts.treefile().string()};
    auto gen = std::mt19937{opts.seed()};
    fmt::print("Beginning distribution\n");
    auto start = std::chrono::high_resolution_clock::now();
    auto means = compare_distributions(tree, straight_ratio, gen, comparisons,
      min90_0/2., max90_0/2., max_0/2., min90_1/2., max90_1/2., max_1/2.);
    auto end = std::chrono::high_resolution_clock::now();
    fmt::print("Done in {} s!\n", std::chrono::duration<float>(end - start).count());

    fmt::print("\nAverage distances between:\n");
    fmt::print("  {}-{} um to end: {} mm\n",
      1e6*min90_0.value(), 1e6*max90_0.value(), 1e3*means.x.value());
    fmt::print("  {}-{} um to end: {} mm\n",
      1e6*min90_1.value(), 1e6*max90_1.value(), 1e3*means.y.value());
    fmt::print("  {}-{} um to {}-{} um: {} mm\n",
      1e6*min90_0.value(), 1e6*max90_0.value(),
      1e6*min90_1.value(), 1e6*max90_1.value(), 1e3*means.z.value());
  }
  catch(std::exception const& e) {
    fmt::print("Error running program: {}\n", e.what());
    return 1;
  }

  google::protobuf::ShutdownProtobufLibrary();
  return 0;
}
