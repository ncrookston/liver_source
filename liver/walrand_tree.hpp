#ifndef JHMI_LIVER_WALRAND_TREE_HPP_NRC_2016_02_02
#define JHMI_LIVER_WALRAND_TREE_HPP_NRC_2016_02_02

#include "liver/build_vessel_map.hpp"
#include "liver/distance_vessel.hpp"
#include "messages/vessel_tree.pb.h"
#include "shape/voxelized_shape.hpp"
#include "utility/binary_tree.hpp"
#include "utility/octtree.hpp"
#include "utility/protobuf_zip_ostream.hpp"
#include <boost/filesystem.hpp>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

namespace jhmi {
  struct closer_to_root{
    m_sq root_dist_;
    m3 root_;
    closer_to_root(m3 const& root, m_sq root_dist) : root_dist_{root_dist}, root_{root} {}

    boost::optional<m_sq> operator()(distance_vessel const& vessel, m3 const& pt) {
      auto cpt = lerp(vessel.l, nearest_t(vessel.l, pt));
      auto dist = distance_squared(cpt - root_);
      if (dist <= root_dist_)
        return dist;
      return boost::none;
    }
  };
  struct closer_to_p1{
    boost::optional<m_sq> operator()(distance_vessel const& vessel, m3 const& pt) {
      auto ncpt = (vessel.l.p1 + vessel.l.p2 + pt) / 3.;
      auto d1 = distance_squared(vessel.l.p1 - ncpt);
      auto d2 = distance_squared(ncpt - pt);
      if (d1 < d2)
        return d2;
      return boost::none;
    }
  };

  class walrand_tree {
    using distance_node = binary_node_t<distance_vessel>;

    binary_tree<distance_vessel> vessels_;
    vidx_to<distance_node> to_vessels_;
    octtree<distance_vessel> grid_;
    voxelized_shape const& liver_;
    id_generator<vessel_tag> get_vessel_id_;
    id_generator<cell_tag> get_cell_id_;
    m3 root_;

    void add_vessel(distance_node n) {
      grid_.add_item(n.value());
      to_vessels_.insert(std::make_pair(n.value().id, n));
    }
  public:
    walrand_tree(build_tree_tag, std::string const& vessel_file,
        voxelized_shape const& liver)
      : vessels_{}, grid_{extents(liver)}, liver_{liver} {

      auto vg = build_vessel_map(vessel_file);
      get_vessel_id_ = vg.second;
      //Organize the map into a binary tree.
      auto parent_id = jhmi_detail::find_parent_vessel(vg.first);
      vessels_ = binary_tree<distance_vessel>{vg.first.at(parent_id).v};
      add_children_vessels(vessels_.root(), [&](auto n) { add_vessel(n); }, vg.first);

      auto node = vessels_.root();
      while (!node.right_child())
        node = node.left_child();
      root_ = node.value().l.p2;
    }

    void connect_cell(m3 const& pt) {
#if 0
      auto nearest = grid_.find_nearest_item(pt, closer_to_root(root_, distance_squared(pt - root_)));
#else
      auto nearest = grid_.find_nearest_item(pt, closer_to_p1{});
#endif
      if (!nearest)
        throw std::runtime_error("Unexpected unconnctable item");
      assert(nearest);
      auto n = to_vessels_.at(nearest->id);
      auto& v = n.value();

      grid_.remove_item(v);
      auto new_start = (v.l.p1 + v.l.p2 + pt) / 3.;
      auto old_start = v.l.p1;
      v.l.p1 = new_start;
      grid_.add_item(v);

      auto new_parent = n.make_left_child_of(
        distance_vessel{get_vessel_id_(), line<m3>{old_start, new_start}});
      add_vessel(new_parent);
      auto cell_vessel = new_parent.set_right_child(
        distance_vessel{get_vessel_id_(), line<m3>{new_start, pt}});
      cell_vessel.value().cell = get_cell_id_();
      add_vessel(cell_vessel);
    }

    std::string write(boost::filesystem::path const& p) const {
      jhmi_message::VesselTree vt;
      RANGES_FOR(auto&& v, vessels_ | view::node_level_order) {
        auto p = v.parent();
        auto r = v.right_child();
        auto l = v.left_child();
        auto vtv = vt.add_vessels();
        vtv->set_id(v.value().id.value());
        vtv->set_parent((p ? p.value().id : vessel_id::invalid()).value());
        vtv->set_left(  (l ? l.value().id : vessel_id::invalid()).value());
        vtv->set_right( (r ? r.value().id : vessel_id::invalid()).value());
        vtv->set_radius(30e-6);
        vtv->set_cell(v.value().cell.value());
        vtv->set_flow(.1);
        vtv->set_entry_pressure(10);
        vtv->set_exit_pressure(10);
        vtv->set_sx(v.value().l.p1.x.value());
        vtv->set_sy(v.value().l.p1.y.value());
        vtv->set_sz(v.value().l.p1.z.value());
        vtv->set_ex(v.value().l.p2.x.value());
        vtv->set_ey(v.value().l.p2.y.value());
        vtv->set_ez(v.value().l.p2.z.value());
        vtv->set_is_const(false);
      }
      std::string filename = "vessel_tree.pbz";
      protobuf_zip_ostream out_stream{p / filename};
      if (!vt.SerializeToZeroCopyStream(out_stream.get()))
        throw std::runtime_error("Failed to write simple vessel tree.");
      return filename;
    }
  };
}//jhmi
#endif
