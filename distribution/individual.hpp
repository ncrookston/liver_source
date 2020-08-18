#ifndef JHMI_DISTRIBUTION_INDIVIDUAL_HPP_NRC_20161013
#define JHMI_DISTRIBUTION_INDIVIDUAL_HPP_NRC_20161013

#include "distribution/distribute_tree.hpp"

namespace jhmi {
  std::vector<int> determine_clusters(distribute_tree<distribute_vessel> const& tree,
                                      m avg_sphere_diameter) {
    auto vessels = std::unordered_map<vessel_id, std::pair<m, int>>{};

    auto clusters = std::vector<int>{};
    for (auto v : tree.vessel_clusters_postorder()) {
      auto it = vessels.find(v.fixed.id);
      auto prev_dist = m{};
      auto curr_sph = 0;
      if (it != vessels.end()) {
        std::tie(prev_dist, curr_sph) = it->second;
      }

      auto curr_dist = distance(v.fixed.start - v.fixed.end);
      if (v.current.spheres.count == 0) {
        curr_dist += prev_dist;
      }
      else {
        curr_dist = curr_dist - double(v.current.spheres.count) * avg_sphere_diameter;
        curr_sph += v.current.spheres.count;
      }

      if (curr_sph > 0) {
        if (curr_dist > 200_um || !v.parent)
          clusters.push_back(curr_sph);
        else
          vessels.insert(std::make_pair(v.parent->spheres.id,
                                        std::make_pair(curr_dist, curr_sph)));
      }
    }
    return clusters;
  }
  template <typename SphereSampler>
  auto distribute_individual(tract_tree const& tree, std::mt19937& gen,
      int num_spheres, double spheres_per_tract, double straight_ratio,
      std::atomic<int>& pct_done, SphereSampler sample_sphere) {
    auto mv = distribute_tree<distribute_vessel>{tree, straight_ratio};
    auto urd = std::uniform_real_distribution<>{};
    auto branch_rand = std::bind(urd, std::ref(gen));
    RANGES_FOR(auto i, ranges::view::ints(0, num_spheres)) {
      auto sphere_radius = sample_sphere();
      mv.traverse_individual([&](distribute_vessel& dv, flow_vessel const& fv,
         distribute_vessel const* left, distribute_vessel const* right) {
        //Next, determine if this is the last sphere location
        if ((!right && !left) || dv.spheres.count > 0 || fv.radius < sphere_radius) {
          dv.spheres.count += 1;
          dv.flow -= fv.flow / double(spheres_per_tract);
          return distribute_tree<distribute_vessel>::traversal::stop;
        }
        else if (right && branch_rand() > left->p)
          return distribute_tree<distribute_vessel>::traversal::right;
        else
          return distribute_tree<distribute_vessel>::traversal::left;
      });
      pct_done = (100*i) / num_spheres;
    }
    pct_done = 100;
    auto sphere_list = distribute_vessel::make_sphere_list(mv, spheres_per_tract);
    auto clusters = determine_clusters(mv, 27.5_um);
    return std::make_pair(sphere_list, clusters);
  }
}
#endif
