#ifndef JHMI_DISTRIBUTION_UNEMBOLIZED_HPP_NRC_20161010
#define JHMI_DISTRIBUTION_UNEMBOLIZED_HPP_NRC_20161010

#include "distribution/distribute_tree.hpp"

namespace jhmi {
  auto unembolized(tract_tree const& ttree, std::mt19937& gen,
                   int num_spheres, double straight_ratio, std::atomic<int>& pct) {
    auto mv = distribute_tree<distribute_vessel>{ttree, straight_ratio};
    auto vessels = mv.vessel_clusters_preorder() | ranges::to_vector;
    vessels[0].current.spheres.count = num_spheres;
    int idx = 0;
    for (auto v : vessels) {
      if (v.left && v.right) {
        auto d = std::binomial_distribution<>{v.current.spheres.count, v.left->p};
        auto num_left = d(gen);
        v.left->spheres.count = num_left;
        v.right->spheres.count = v.current.spheres.count - num_left;
        v.current.spheres.count = 0;
      }
      else if (v.left) {
        v.left->spheres.count = v.current.spheres.count;
        v.current.spheres.count = 0;
      }
      pct = 100 * idx++ / vessels.size();
    }

    pct = 100;
    auto spheres = std::vector<std::pair<m3,vessel_id>>{};
    auto clusters = std::vector<int>{};
    for (auto v : vessels) {
      if (v.current.spheres.count > 0)
        clusters.push_back(v.current.spheres.count);

      for (int i : ranges::view::ints(0, v.current.spheres.count))
        spheres.push_back(std::make_pair(v.current.spheres.end, v.fixed.id));
    }
    return std::make_pair(spheres, clusters);
  }
}//jhmi

#endif
