#ifndef JHMI_UTILITY_GAUSSIAN_HPP_NRC_20161013
#define JHMI_UTILITY_GAUSSIAN_HPP_NRC_20161013

#include <random>

namespace jhmi {
  struct gaussian_90_sampler {
    std::mt19937& gen_;
    std::normal_distribution<> dist_;
    gaussian_90_sampler(std::mt19937& gen, m min_90_radius, m max_90_radius)
      : gen_{gen},
        dist_{((max_90_radius + min_90_radius) / 2.).value(),
              ((max_90_radius - (max_90_radius + min_90_radius) / 2.) / 1.65).value()}
    {}
    auto operator()() { return dist_(gen_) * meters; }
  };
  struct truncated_gaussian {
    gaussian_90_sampler sampler_;
    m min_radius_;
    boost::optional<m> max_radius_;
    truncated_gaussian(gaussian_90_sampler const& sampler, m min_radius, boost::optional<m> max_radius)
      : sampler_{sampler}, min_radius_{min_radius}, max_radius_{max_radius} {}
    auto operator()() {
      auto lb = std::max(sampler_(), min_radius_);
      return max_radius_ ? std::min(lb, *max_radius_) : lb;
    }
  };
}

#endif

