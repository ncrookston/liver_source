#ifndef JHMI_LIVER_PHYSICAL_VESSEL_TREE_HPP_NRC_20150831
#define JHMI_LIVER_PHYSICAL_VESSEL_TREE_HPP_NRC_20150831

#include "liver/build_vessel_map.hpp"
#include "liver/get_split_point.hpp"
#include "liver/load_vessel_protobuf.hpp"
#include "liver/distance_vessel.hpp"
#include "liver/physical_vessel.hpp"
#include "liver/physical_vessel_tree_updater.hpp"
#include "utility/binary_tree.hpp"
#include "utility/line.hpp"
#include "utility/octtree.hpp"
#include <boost/filesystem.hpp>
#include <range/v3/algorithm/equal.hpp>
#include <range/v3/view.hpp>
#include <set>

namespace jhmi {
  class physical_vessel_tree {
    binary_tree<physical_vessel> vessels_;
    vidx_to<binary_node_t<physical_vessel>> to_vessels_;
    id_generator<vessel_tag> get_vessel_id_;
    octtree<distance_vessel> grid_;
    physical_vessel_tree_updater vessel_updater_;
    double gamma_;

    struct forward_distance_squared {
      auto operator()(distance_vessel const& sv, m3 const& pt) -> boost::optional<decltype(pt.x*pt.x)> {
        auto t = dot(pt - sv.l.p1, sv.loff) / sv.ld;
        if (t < 0)
          return boost::none;
        auto lpt = t > 1 ? sv.l.p2 : (sv.l.p1 + t * sv.loff);
        return boost::make_optional(distance_squared(lpt, pt));
      }
    };

    auto get_nearest_vessel(m3 const& loc, std::mt19937& gen) {
      //Randomly select one item of many
      auto items = grid_.find_n_nearest_items(loc, 10, forward_distance_squared());
      assert(!items.empty());
#if 1
      auto dist = make_balanced_sampler(items | ranges::view::transform([&](distance_vessel const& sv) {
        return int(std::lround(sv.l.p2.z / lobule::cell_thickness));
      }) | ranges::to_vector);
#else
#if 1
      auto w = items | ranges::view::transform([&](distance_vessel const& sv) {
        return abs(sv.l.p2.z - loc.z) < lobule::cell_thickness / 2. ? 0. : 1.;
      }) | ranges::to_vector;
      auto dist = std::discrete_distribution<>(w.begin(), w.end());
#else
      std::uniform_int_distribution<> dist(0, items.size() - 1);
#endif
#endif
      return to_vessels_.at(items[dist(gen)].id);
    }

    void record_vessel(binary_node_t<physical_vessel> node) {
      grid_.add_item(node.value());
      to_vessels_.insert(std::make_pair(node.value().id(), node));
    }

    vessel_id split_existing_vessel(binary_node_t<physical_vessel> node, macrocell& cell) {
      auto& v = node.value();

      grid_.remove_item(v);
      auto new_start = get_split_point(v, cell.center, cell.flow, gamma_);
      auto old_start = v.start();
      v.set_start(new_start);
      grid_.add_item(v);

      auto new_parent = node.make_left_child_of(physical_vessel{old_start, new_start,
         1_mm, cell_id::invalid(), v.flow(), v.entry_pressure(),
         get_vessel_id_(), v.is_const()});
      record_vessel(new_parent);
      auto cell_vessel = new_parent.set_right_child(physical_vessel{new_start,
        cell.center, 1_mm, cell.id, cell.flow, cell.pressure, get_vessel_id_()});
      cell.parent_vessel = cell_vessel.value().id();
      record_vessel(cell_vessel);
      return cell.parent_vessel;
    }

  public:
    physical_vessel_tree(build_tree_tag, boost::filesystem::path const& filename, cube<m3> const& extents, double gamma, Pa cell_pressure, cubic_meters_per_second cell_flow, std::mt19937& gen)
      : vessels_{}, to_vessels_{}, get_vessel_id_{},
        grid_{extents, 16}, vessel_updater_{vessels_, gamma, cell_pressure, cell_flow, gen}, gamma_{gamma} {
      auto vessel_generator = build_vessel_map(filename);
      get_vessel_id_ = vessel_generator.second;
      auto vessel_map = vessel_generator.first;
      //Organize the map into a binary tree.
      auto parent_id = jhmi_detail::find_parent_vessel(vessel_map);
      vessels_ = binary_tree<physical_vessel>{vessel_map.at(parent_id).v};
      add_children_vessels(vessels_.root(), [&](auto n) {
          record_vessel(n);
        }, vessel_map);
      vessel_updater_.normalize_all();
    }

    physical_vessel_tree(load_tree_tag, boost::filesystem::path const& filename, std::mt19937& gen)
      : vessels_{}, to_vessels_{}, get_vessel_id_{}, grid_{cube<m3>{}},
        vessel_updater_{vessels_, 2.7, Pa{}, cubic_meters_per_second{}, gen}, gamma_{} {
      auto vt = load_protobuf<jhmi_message::VesselTree>(filename);
      gamma_ = vt.gamma();
      if (std::abs(gamma_) < 1e-2)
        gamma_ = 2.7;
      auto vns = load_vessel_protobuf(vt);
      vessel_id max_id{0};
      boost::optional<cube<m3>> ext;
      RANGES_FOR(auto&& v, vns) {
        max_id = std::max(max_id, v.second.v.id());
        ext = ext ? expand(*ext, v.second.v.start())
                  : cube<m3>{v.second.v.start(), v.second.v.start()};
        ext = expand(*ext, v.second.v.end());
      }
      grid_ = octtree<distance_vessel>{*ext};

      auto parent_id = jhmi_detail::find_parent_vessel(vns);
      vessels_ = binary_tree<physical_vessel>{vns.at(parent_id).v};
      add_children_vessels(vessels_.root(), [&](auto n) { record_vessel(n); }, vns);
      get_vessel_id_ = id_generator<vessel_tag>{max_id};
      //No need to use vessel_updater_ here, it should have been saved with
      // desried radii, pressures, etc.

      RANGES_FOR(auto&& n, vessels_ | view::node_post_order) {
        int so = 1;
        if (n.left_child()) {
          so = n.left_child().value().strahler_order;
        }
        if (n.right_child()) {
          int rso = n.right_child().value().strahler_order;
          if (rso == so)
            so++;
          else if (rso > so)
            so = rso;
        }
        n.value().strahler_order = so;
      }
      fmt::print("Highest order: {}\n", vessels_.root().value().strahler_order);
    }

    auto terminal_vessels() const {
      return vessels_ | view::node_in_order
        | ranges::view::filter([](binary_const_node_t<physical_vessel> n) {
           return !n.left_child() && !n.right_child(); })
        | ranges::view::transform([](binary_const_node_t<physical_vessel> n) {
           return n.value(); });
    }

    auto terminal_vessel_nodes() const {
      return vessels_ | view::node_in_order
        | ranges::view::filter([](binary_const_node_t<physical_vessel> n) {
           return !n.left_child() && !n.right_child(); });
    }

    vessel_id connect_cell_initial(macrocell& cell, vessel_id id) {
      auto min_vessel = to_vessels_.at(id);
      auto& mv = min_vessel.value();
      auto v = physical_vessel{mv.end(), cell.center, 1_mm, cell.id, cell.flow, cell.pressure, get_vessel_id_()};
      //Don't adjust any positions, this should only happen on the initial
      // population of microspheres, so min_vessel should be const.
      auto n = min_vessel.set_left_child(v);
      cell.parent_vessel = jhmi::id(v);
      record_vessel(n);
      for (n = n.parent(); n; n = n.parent())
        n.value().flow_ -= cell.flow;
      return cell.parent_vessel;
    }
    vessel_id connect_cell(macrocell& cell, std::mt19937& gen) {
      auto min_vessel = get_nearest_vessel(cell.center, gen);
      assert(min_vessel);
      split_existing_vessel(min_vessel, cell);

      auto n = to_vessels_.at(cell.parent_vessel);
      for (n = n.parent(); n; n = n.parent())
        n.value().flow_ -= cell.flow;

      return cell.parent_vessel;
    }
    auto gamma() const { return gamma_; }
    auto size() const { return to_vessels_.size(); }
    auto vessels() const { return vessels_ | view::pre_order; }
    auto vessel_nodes() const { return vessels_ | view::node_pre_order; }
    auto post_order_vessel_nodes() const { return vessels_ | view::node_post_order; }

    physical_vessel const& at(vessel_id id) const {
      return to_vessels_.at(id).value();
    }
    binary_node_t<physical_vessel> at_node(vessel_id id) {
      return to_vessels_.at(id);
    }
    auto at_node(vessel_id id) const {
      return binary_const_node_t<physical_vessel>{to_vessels_.at(id)};
    }

    void normalize_all() {
      vessel_updater_.normalize_all();
    }

    bool remove(vessel_id id) {
      auto node = to_vessels_.at(id);
      assert(node.value().cell().valid());
      auto flow = node.value().flow();
      bool remove_left = true;
      std::vector<binary_node_t<physical_vessel>> remove_vessels;
      while (node) {
        if (node.left_child() && node.right_child()) {
          assert(!node.value().cell().valid());
          break;
        }
        if (node.value().is_const())
          return false;
        remove_vessels.push_back(node);
        remove_left = node.is_left_child();
        node = node.parent();
      }
      //node now holds what should continue to exist, and id et al. may be
      // removed.
      RANGES_FOR(auto const& vessel, remove_vessels) {
        grid_.remove_item(vessel.value());
        to_vessels_.erase(vessel.value().id());
      }

      if (remove_left)
        node.set_left_child(nullptr);
      else
        node.set_right_child(nullptr);

      if (!node.value().is_const()) {
        auto remove_node = node;
        node = node.left_child() ? node.left_child() : node.right_child();
        grid_.remove_item(remove_node.value());
        grid_.remove_item(node.value());
        to_vessels_.erase(remove_node.value().id());
        node.value().set_start(remove_node.value().start());
        node.replace_parent();
        grid_.add_item(node.value());
      }
      for (; node; node = node.parent())
        node.value().flow_ -= flow;
      return true;
    }

    void store(jhmi_message::VesselTree& vt) const {
      vt.set_gamma(gamma_);
      RANGES_FOR(auto&& v, vessels_ | view::node_level_order) {
        auto p = v.parent();
        auto r = v.right_child();
        auto l = v.left_child();
        auto vtv = vt.add_vessels();
        vtv->set_id(v.value().id().value());
        vtv->set_parent((p ? p.value().id() : vessel_id::invalid()).value());
        vtv->set_left(  (l ? l.value().id() : vessel_id::invalid()).value());
        vtv->set_right( (r ? r.value().id() : vessel_id::invalid()).value());
        vtv->set_radius(v.value().radius().value());
        vtv->set_cell(v.value().cell().value());
        vtv->set_flow(v.value().flow().value());
        vtv->set_entry_pressure(v.value().entry_pressure().value());
        vtv->set_exit_pressure(v.value().exit_pressure().value());
        vtv->set_sx(v.value().start().x.value());
        vtv->set_sy(v.value().start().y.value());
        vtv->set_sz(v.value().start().z.value());
        vtv->set_ex(v.value().end().x.value());
        vtv->set_ey(v.value().end().y.value());
        vtv->set_ez(v.value().end().z.value());
        vtv->set_is_const(v.value().is_const());
      }
    }

    bool validate(bool ignore_unconnected_vessels) const {
      bool no_errors = true;
      auto check_errors = [&](binary_const_node_t<physical_vessel> cn, physical_vessel const& v) {
        if (cn) {
          auto const& c = cn.value();
          if (distance(c.start() - v.end()) >= 1e-8_mm) {
            fmt::print("Invalid spatial distance: {} to {}\n", v.id(), c.id());
            no_errors = false;
          }
        }
      };
      RANGES_FOR(auto n, vessels_ | view::node_in_order) {
        check_errors(n.left_child(), n.value());
        check_errors(n.right_child(), n.value());
      }
      std::set<vessel_id> all_vessels;
      RANGES_FOR(auto const& v, vessels_ | view::in_order) {
        all_vessels.insert(v.id());
      }
      auto grid_vessels = grid_.get_all();
      std::set<vessel_id> map_vessels;
      RANGES_FOR(auto& v, to_vessels_ | ranges::view::keys) {
        map_vessels.insert(v);
      }
      if (!ranges::equal(all_vessels, grid_vessels)) {
        fmt::print("Some vessels not found in the grid:\n");
        std::set_difference(all_vessels.begin(), all_vessels.end(), grid_vessels.begin(), grid_vessels.end(), std::ostream_iterator<vessel_id>{std::cout, " "});
        no_errors = false;
      }
      if (!ranges::includes(map_vessels, all_vessels, std::less<>{})) {
        fmt::print("Some vessels not found in the map\n");
        no_errors = false;
      }
      no_errors &= check_close(vessels_.root().value().entry_pressure(), input_pressure,
        1e-3, vessels_.root().value().id(), "root pressure", "model pressure");

      //Now we need to verify that radii and pressures match our expectations.
      RANGES_FOR(auto n, vessels_ | view::node_level_order) {
        auto& v = n.value();
        if (v.cell().valid() && (n.left_child() || n.right_child())) {
          fmt::print("Vessel {} connects macrocell {} and has child vessels\n",
            v.id(), v.cell());
          no_errors = false;
        }
        if (n.left_child()) {
          no_errors &= check_close(v.exit_pressure(),
            n.left_child().value().entry_pressure(), 1e-3,
            v.id(), "node exit pressure", "left_child entry pressure");
        }
        if (n.right_child()) {
          no_errors &= check_close(v.exit_pressure(),
            n.right_child().value().entry_pressure(), 1e-3,
            v.id(), "node exit pressure", "right_child entry pressure");
        }

        if (n.right_child() && n.left_child()) {
          no_errors &= check_close(v.radius(),
            std::pow(std::pow(n.left_child().value().radius().value(), gamma_)
                   + std::pow(n.right_child().value().radius().value(), gamma_), 1./gamma_) * meters,
            1e-5, v.id(), "node radius", "children-derived radius");
          no_errors &= check_close(v.flow(),
            n.left_child().value().flow() + n.right_child().value().flow(),
            1e-5, v.id(), "children flows", "node flow");
        }
        else if (n.right_child()) {
          no_errors &= check_close(n.right_child().value().radius(), v.radius(),
            1e-5, v.id(), "right child radius", "node radius");
          no_errors &= check_close(v.flow(), n.right_child().value().flow(),
            1e-4, v.id(), "right child flow", "node flow");
        }
        else if (n.left_child()) {
          no_errors &= check_close(v.radius(), n.left_child().value().radius(),
            1e-5, v.id(), "node radius", "left child radius");
          no_errors &= check_close(v.flow(), n.left_child().value().flow(),
            1e-4, v.id(), "node flow", "left child flow");
        }

        no_errors &= v.validate();
      }

      if (!ignore_unconnected_vessels) {
        auto unconnected_vessels = vessels_ | view::node_in_order
          | ranges::view::filter([](binary_const_node_t<physical_vessel> n) {
             return !n.left_child() && !n.right_child() && !n.value().cell().valid(); })
          | ranges::to_vector;
        if (!unconnected_vessels.empty()) {
          no_errors = false;
          fmt::print("Unconnected vessels found: ");
          RANGES_FOR(auto n, unconnected_vessels) {
            fmt::print("{} ", n.value().id());
          }
          fmt::print("\n");
        }
      }
      return no_errors;
    }
  };
  bool operator==(physical_vessel_tree const& lhs, physical_vessel_tree const& rhs) {
    return ranges::equal(lhs.vessels(), rhs.vessels(), std::equal_to<physical_vessel>{});
  }
}
#endif
