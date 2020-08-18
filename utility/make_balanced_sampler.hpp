#ifndef JHMI_UTILITY_MAKE_BALANCED_SAMPLER_HPP_NRC_20160805
#define JHMI_UTILITY_MAKE_BALANCED_SAMPLER_HPP_NRC_20160805

#include <boost/container/flat_map.hpp>
#include <range/v3/algorithm.hpp>
#include <range/v3/view.hpp>

namespace jhmi {

  auto make_balanced_sampler(std::vector<int> const& values) {
    auto hist = boost::container::flat_map<int,int>{};
    for (auto&& v : values)
      ++hist[v];
    auto desired_pct = 1. / hist.size();
    auto all_sum = ranges::accumulate(hist | ranges::view::values, 0);
    auto weight_table = boost::container::flat_map<int, double>{};
    for (auto&& h : hist)
      weight_table.insert(std::make_pair(h.first, desired_pct * all_sum / h.second));
    auto weights = values | ranges::view::transform([&](int i) { return weight_table[i]; });

    return std::discrete_distribution<>(weights.begin(), weights.end());
    //auto ones = std::vector<double>(hist.size(), 1.);
    //return std::discrete_distribution<>(ones.begin(), ones.end());
  }
}

#endif
