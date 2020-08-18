#ifndef JHMI_DISTRIBUTION_EMBOLIZED_HPP_NRC_20161013
#define JHMI_DISTRIBUTION_EMBOLIZED_HPP_NRC_20161013

#include "distribution/individual.hpp"

namespace jhmi {
  auto embolized(tract_tree const& tree, std::mt19937& gen,
        int num_spheres, double spheres_per_tract, double straight_ratio,
        std::atomic<int>& pct_done) {
    return distribute_individual(tree, gen, num_spheres, spheres_per_tract,
      straight_ratio, pct_done, [] { return 0_um; });
  }
}
#endif


