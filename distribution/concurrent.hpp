#ifndef JHMI_DISTRIBUTION_CONCURRENT_HPP_NRC_20161014
#define JHMI_DISTRIBUTION_CONCURRENT_HPP_NRC_20161014

#include "distribute_tree.hpp"
#include "utility/gaussian.hpp"
#include <range/v3/action.hpp>
#include <range/v3/view.hpp>
#include <random>

namespace jhmi {
  struct sphere_info {
    double location = 0.;
    m radius;
    double time_fraction = 1.;
  };
  struct volume_vessel {
    explicit volume_vessel(flow_vessel const& fv)
      : spheres{},
        flow{fv.flow},
        radius{fv.radius},
        cross_section{radius * radius * pi},
        length{distance(fv.start - fv.end)},
        p{1.}
    {}
    std::vector<sphere_info> spheres;
    cubic_meters_per_second flow;
    m radius;
    square_meters cross_section;
    m length;
    double p;
  };
  auto make_sphere_list(distribute_tree<volume_vessel> const& tree) {
    auto spheres = std::vector<std::pair<m3,vessel_id>>{};
    for (auto const& v : tree.vessel_clusters_preorder()) {
      for (auto const& s : v.current.spheres) {
        fmt::print("Location for sphere in {}: {}\n", v.fixed.id, s.location);
        spheres.push_back(std::make_pair(v.fixed.start + (v.fixed.end - v.fixed.start) * s.location, v.fixed.id));
      }
    }
    fmt::print("Number of spheres in list: {}\n", spheres.size());
    auto clusters = std::vector<int>{};
    return std::make_pair(spheres, clusters);
  }

  //static constexpr auto const min_vessel_length = 250_um;
  static constexpr auto const min_vessel_length = 25_um;

  auto concurrent(tract_tree const& ttree, std::mt19937& gen, int num_spheres,
                  double straight_ratio, m min90, m max90, boost::optional<m> max_diameter,
                  std::atomic<int>& pct_done) {
    auto mv = distribute_tree<volume_vessel>{ttree, straight_ratio};
    auto vessels = mv.vessel_clusters_preorder() | ranges::to_vector;
//#define REPORT_DETAILED
#define ALL
#ifdef ALL
    auto diameter_sampler = truncated_gaussian{
      gaussian_90_sampler{gen, min90, max90}, 0_mm, max_diameter};
#else
    auto diameter_sampler = [] { return 25_um; };
#endif
    auto injection_time = 10. * seconds;
    //The time step is related to how quickly blood flows through the arteries.
    // So, step through the tree, calculate the max blood flow speed, and use that.
    auto max_speed = 0. * meters / seconds;
    int max_speed_idx = -1;

    for (int v = 0; v < vessels.size(); ++v) {
      if (vessels[v].current.length > min_vessel_length) {
        max_speed = std::max(max_speed, vessels[v].current.flow / vessels[v].current.cross_section);
        max_speed_idx = v;
      }
    }
    auto step = min_vessel_length / max_speed;
    fmt::print("Maximum speed: {}\nTime step: {}\n", max_speed, step);
    fmt::print("Max vessel properties: length {}, flow {}, xs {:1.10f}, rad {:1.10f}\n",
        vessels[max_speed_idx].current.length,
        vessels[max_speed_idx].current.flow,
        vessels[max_speed_idx].current.cross_section.value(),
        vessels[max_speed_idx].current.radius.value());
    //Assume constant injection rate per step.
    auto sample_num_spheres = std::poisson_distribution<>{double(num_spheres) * step / injection_time};
    std::cout << "Mean: " << (double(num_spheres) * step / injection_time).value() << std::endl;
    auto spheres_left = num_spheres;
    int spheres_moving = 0;
    for (auto time = 0. * seconds; spheres_left > 0 || spheres_moving > 0; time += step) {
      spheres_moving = 0;
#ifdef ALL
      auto current_spheres = sample_num_spheres(gen);
      if (current_spheres > spheres_left)
        current_spheres = spheres_left;
#else
      auto current_spheres = spheres_left;
#endif
      spheres_left -= current_spheres;

      vessels[0].current.spheres |= ranges::action::push_back(ranges::view::generate_n([&] {
        return sphere_info{0., diameter_sampler() / 2., 1.}; }, current_spheres));
      auto flow_distance = [](volume_vessel const& vv, auto flow, s tdelta) {
        return double(flow * tdelta / vv.cross_section / vv.length);
      };
      auto zero_flow = cubic_meters_per_second(0);
      for (auto v : vessels) {
#ifdef REPORT_DETAILED
        if (!v.current.spheres.empty()) {
          fmt::print("Vessel id {} with radius {} flow {} has {} spheres\n", v.fixed.id,
              1000.*v.current.radius, v.current.flow.value(), v.current.spheres.size());
          if (v.parent)
            fmt::print("\tParent has flow {}\n\tHas left? {}\n", v.parent->flow, v.left != nullptr);
        }
#endif
        auto current_flow = v.current.flow;
        if (v.parent && v.parent->flow <= zero_flow)
          v.current.flow = zero_flow;
        if (current_flow <= zero_flow)
          continue;
        if (v.current.spheres.empty())
          continue;
        //Calculate the fraction of the length of this vessel moved in this timedelta (1)
        auto distance_traveled = flow_distance(v.current, current_flow, step);
        auto sph_rem_it = ranges::remove_if(v.current.spheres, [&](sphere_info& c) {
#ifdef REPORT_DETAILED
          fmt::print("Sphere at location {} time fraction {}\n", v.fixed.id, c.location, c.time_fraction);
#endif
          auto sphere_distance_traveled = c.time_fraction * distance_traveled;
          if (c.location + sphere_distance_traveled < 1.) {
            c.location += sphere_distance_traveled;
            c.time_fraction = 1.;
            return false;
          }
          else if ((!v.left || v.left->radius < c.radius) && (!v.right || v.right->radius < c.radius)) {
            // First, if particle too big for either, then stop it there.
            // Set flow in this vessel to 0.
            c.location = 1.;
            v.current.flow = zero_flow;
            return false;
          }
          else {
            auto next = v.left;
            // Second, if it's too big for just one, then assume it goes in the other.
            if (v.left->radius < c.radius && v.right)
              next = v.right;
            // Third, if it fits in either one, use a bernoulli r.v. to choose.
            else if (v.left->radius >= c.radius && v.right && v.right->radius >= c.radius) {
              auto bd = std::bernoulli_distribution{v.left->p};
              if (!bd(gen))
                next = v.right;
            }
            //If either of the last two occurs, move sphere to child vessel.
            if (v.current.length > min_vessel_length) {
              c.time_fraction *= (1 - c.location) / sphere_distance_traveled;
            }

            next->spheres.push_back(sphere_info{0., c.radius, c.time_fraction});
            return true;
          }
        });
        v.current.spheres.erase(sph_rem_it, v.current.spheres.end());
        spheres_moving += v.current.spheres.size();
      }
      //Now, update flow and probabilities.
      //If no embolization, this should be just like the unembolized distribution
      mv.update_flows();

      pct_done = 100 - 100 * (spheres_left + spheres_moving) / num_spheres;
    }

    pct_done = 100;
    return make_sphere_list(mv);
  }
}

#endif
