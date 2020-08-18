#ifndef JHMI_DISTRIBUTION_RADIIZED_HPP_NRC_20161013
#define JHMI_DISTRIBUTION_RADIIZED_HPP_NRC_20161013

#include "distribution/individual.hpp"
#include "utility/gaussian.hpp"

namespace jhmi {
  auto radiized(tract_tree const& tree, std::mt19937& gen,
      int num_spheres, double spheres_per_tract, double straight_ratio,
      m min90, m max90, boost::optional<m> max_diameter, std::atomic<int>& pct_done) {
    return distribute_individual(tree, gen, num_spheres, spheres_per_tract,
      straight_ratio, pct_done,
      truncated_gaussian{gaussian_90_sampler{gen, min90, max90}, 0_mm,
        max_diameter});
  }
}
#endif

