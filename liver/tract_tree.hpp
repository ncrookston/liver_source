#ifndef JHMI_LIVER_WALRAND_TRACT_TREE_HPP_NRC_20160206
#define JHMI_LIVER_WALRAND_TRACT_TREE_HPP_NRC_20160206

#include "liver/build_vessel_map.hpp"
#include "liver/fill_liver_volume.hpp"
#include "liver/load_vessel_protobuf.hpp"
#include "utility/binary_tree.hpp"
#include "utility/volume_image.hpp"
#include <tbb/tbb.h>
#include <fstream>

namespace jhmi {
  struct flow_vessel {
    flow_vessel(physical_vessel const& v)
      : id{v.id()}, start{v.start()}, end{v.end()}, radius{v.radius()}, flow{v.flow()} {}
    flow_vessel(vessel_id id, m3 start, m3 end, m radius, cubic_meters_per_second flow)
      : id(id), start(start), end(end), radius(radius), flow(flow) {}
    vessel_id id;
    m3 start;
    m3 end;
    m radius;
    cubic_meters_per_second flow;
  };
  auto id(flow_vessel const& f) { return f.id; }

  class tract_tree {
    binary_tree<flow_vessel> vessels_;
  public:
    explicit tract_tree(binary_tree<flow_vessel>&& vessels) : vessels_{std::move(vessels)} {}

    explicit tract_tree(boost::filesystem::path const& filename) : vessels_{} {

      auto vt = load_protobuf<jhmi_message::VesselTree>(filename);
      auto vns = load_vessel_protobuf(vt);
      auto parent_id = jhmi_detail::find_parent_vessel(vns);
      vessels_ = binary_tree<flow_vessel>{vns.at(parent_id).v};
      add_children_vessels(vessels_.root(), [&](auto) {}, vns);
      //Finally, swap left and right such that the straight vessel is on the left.
      auto max_id = vessel_id::invalid();
      m min_rad = 100_mm;
      for (auto n : vessels_ | view::node_post_order) {
        min_rad = std::min(min_rad, n.value().radius);
        max_id = std::max(max_id, n.value().id);
        if (n.left_child() && n.right_child()) {
          auto v1 = normalize(n.value().start - n.value().end);
          auto v2 = normalize(n.left_child().value().end - n.value().end);
          auto v3 = normalize(n.right_child().value().end - n.value().end);
          bool left_is_straight = dot(v2, v1) >= dot(v3, v1);
          if (!left_is_straight)
            n.swap_children();
        }
        else if (n.right_child())
          n.swap_children();
      }
      fmt::print("Min radius: {:1.10f} m\n", min_rad.value());
      auto id_gen = make_generator(max_id);
      int num_tracts = 0, num_zero_length = 0, num_ternary = 0;
      for (auto n : vessels_ | view::node_post_order) {
        if (!n.left_child()) {
          n.set_left_child(flow_vessel{id_gen(), n.value().end,
            n.value().end + m3{0_mm,0_mm,lobule::cell_thickness},
            n.value().radius, n.value().flow});
        }
        ++num_tracts;
        if (distance(n.value().end - n.value().start).value() < 1e-16) {
          ++num_zero_length;
          if (n.right_child() && n.parent().right_child())
            ++num_ternary;
        }
      }
      fmt::print("# zero length: {}, # total {}, #ternary {}\n", 100 * num_zero_length, num_tracts, num_ternary);
    }
    auto vessel_nodes_preorder() const {
      return vessels_ | view::node_pre_order;
    }
    auto vessel_nodes_postorder() const {
      return vessels_ | view::node_post_order;
    }

    auto vessels_postorder() const {
      return vessels_ | view::post_order;
    }
    auto root_node() const { return vessels_.root(); }
  };
}//jhmi
#endif
