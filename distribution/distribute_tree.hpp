#ifndef JHMI_LIVER_DISTRIBUTE_TREE_HPP_NRC_20161010
#define JHMI_LIVER_DISTRIBUTE_TREE_HPP_NRC_20161010

#include "liver/tract_tree.hpp"
#include "utility/maybe.hpp"

namespace jhmi {
  struct vessel_spheres {
    vessel_id id;
    m3 start;
    m3 end;
    int count = 0;
  };
  template <typename VesselType>
  class distribute_tree {
    tract_tree const& tree_;
    std::unordered_map<vessel_id, VesselType> v_;
    double straight_ratio_;

    auto& at(binary_const_node_t<flow_vessel> n) {
      return v_.at(n.value().id);
    }
    auto const& at(binary_const_node_t<flow_vessel> n) const {
      return v_.at(n.value().id);
    }

    void update_flow(binary_const_node_t<flow_vessel> n) {
      auto& lmv = at(n.left_child());
      auto& rmv = at(n.right_child());
      auto lf = lmv.flow;
      auto rf = rmv.flow;
      auto D_l = lf / (lf+rf);
      auto D_r = rf / (lf+rf);

      lmv.p = D_l * straight_ratio_ / (D_l * straight_ratio_ + D_r * (1 - straight_ratio_));
      rmv.p = 1 - lmv.p;
      at(n).flow = lf + rf;
    }
    auto* from_node(binary_const_node_t<flow_vessel> n) {
      return n ? &v_.at(n.value().id) : nullptr;
    }
    auto const* from_node(binary_const_node_t<flow_vessel> n) const {
      return n ? &v_.at(n.value().id) : nullptr;
    }
  public:
    enum class traversal { left, right, stop, abort };

    distribute_tree(tract_tree const& tree, double straight_ratio)
     : tree_{tree}, v_{}, straight_ratio_{straight_ratio} {
      m min_rad = 100_mm;
      for (auto n : tree_.vessel_nodes_postorder()) {
        auto& v = n.value();
        min_rad = std::min(min_rad, v.radius);
        //auto volume = v.radius * v.radius * pi * distance(v.start - v.end);
        v_.insert(std::make_pair(v.id, VesselType{v}));
        if (n.left_child() && n.right_child())
          update_flow(n);
      }//for
      fmt::print("Min radius in distribute_tree: {:1.10f}\n", min_rad.value());
    }
    void update_flows() {
      for (auto n : tree_.vessel_nodes_postorder()) {
        if (n.left_child() && n.right_child())
          update_flow(n);
        else if (n.left_child())
          at(n).flow = at(n.left_child()).flow;
      }
    }
    template <typename F>
    void traverse_individual(F f) {
      auto call_f = [=](binary_const_node_t<flow_vessel> n) {
        return f(at(n), n.value(), from_node(n.left_child()), from_node(n.right_child()));
      };
      auto node = tree_.root_node();
      auto result = call_f(node);
      while (result != traversal::stop && result != traversal::abort) {
        if (result == traversal::left)
          node = node.left_child();
        else
          node = node.right_child();
        result = call_f(node);
      }
      //User has not modified flow.
      if (result == traversal::abort)
        return;
      while (node.parent()) {
        node = node.parent();
        if (node.left_child() && node.right_child())
          update_flow(node);
        else if (node.left_child())
          at(node).flow = at(node.left_child()).flow;
      }
    }
    struct vessel_cluster { VesselType& current, * left, * right, * parent; flow_vessel const& fixed; };
    auto vessel_clusters_preorder() {
      return tree_.vessel_nodes_preorder() | ranges::view::transform(
        [this](binary_const_node_t<flow_vessel> n) {
          return vessel_cluster{at(n), from_node(n.left_child()),
            from_node(n.right_child()), from_node(n.parent()), n.value()};
        });
    }
    struct const_vessel_cluster { VesselType const& current, * left, * right, * parent; flow_vessel const& fixed; };
    auto vessel_clusters_preorder() const {
      return tree_.vessel_nodes_preorder() | ranges::view::transform(
        [this](binary_const_node_t<flow_vessel> n) {
          return const_vessel_cluster{at(n), from_node(n.left_child()),
            from_node(n.right_child()), from_node(n.parent()), n.value()};
        });
    }
    auto vessel_clusters_postorder() const {
      return tree_.vessel_nodes_postorder() | ranges::view::transform(
        [this](binary_const_node_t<flow_vessel> n) {
          return const_vessel_cluster{at(n), from_node(n.left_child()),
            from_node(n.right_child()), from_node(n.parent()), n.value()};
        });
    }
  };

  struct distribute_vessel {
    explicit distribute_vessel(flow_vessel const& fv)
      : spheres{fv.id, fv.start, fv.end, 0}, flow{fv.flow}, p{1.} {}
    vessel_spheres spheres;
    cubic_meters_per_second flow;
    double p;

    static auto make_sphere_list(distribute_tree<distribute_vessel>& tree,
                                 double spheres_per_tract) {
      //Go through all vessels, and any that are too short for num_spheres
      // should have those spheres distributed
      if (isinf(spheres_per_tract))
        spheres_per_tract = lobule::cell_thickness / 27.5_um;
      auto spheres = std::vector<std::pair<m3,vessel_id>>{};
      auto spheres_per_m = spheres_per_tract / lobule::cell_thickness;

      for (auto v : tree.vessel_clusters_preorder()) {
        if (v.current.spheres.count == 0)
          continue;
        auto vdiff = v.current.spheres.end - v.current.spheres.start;
        auto max_here = std::max(int(std::ceil(distance(vdiff) * spheres_per_m)), 1);
        if (v.current.spheres.count > max_here) {
          auto num_left = v.current.spheres.count - max_here;
          if (v.right) {
            auto num_left_left = int(std::ceil(v.left->p * num_left));
            v.left->spheres.count += num_left_left;
            v.right->spheres.count += num_left - num_left_left;
            v.current.spheres.count = max_here;
          }
          else if (v.left) {
            v.left->spheres.count += num_left;
            v.current.spheres.count = max_here;
          }
          //else leave the spheres in this portal tract
        }
        auto vstep = vdiff / distance(vdiff) / spheres_per_m;
        for (int i : ranges::view::ints(0, v.current.spheres.count))
          spheres.push_back(std::make_pair(v.current.spheres.end - double(i) * vstep, v.fixed.id));
      }
      return spheres;
    }
  };
}

#endif
