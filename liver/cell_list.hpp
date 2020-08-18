#ifndef JHMI_LIVER_CELL_LIST_HPP_NRC_20150829
#define JHMI_LIVER_CELL_LIST_HPP_NRC_20150829

#include "liver/constants.hpp"
#include "liver/macrocell.hpp"
#include "liver/locations/grid_locations.hpp"
#include "liver/locations/lattice_locations.hpp"
#include "messages/vessel_tree.pb.h"
#include "utility/load_protobuf.hpp"
#include <range/v3/algorithm.hpp>
#include <fmt/ostream.h>
#include <cstdio>
#include <random>
namespace jhmi {
  class cell_list {
    cidx_to<macrocell> cells_;
    id_generator<cell_tag> get_cell_id_;
    std::mt19937& gen_;
    m cell_radius_;
    std::unique_ptr<cell_locations> loc_;
    cube<m3> ext_;
    cubic_meters_per_second proper_ha_flow_;
    Pa cell_pressure_;

    friend auto extents(cell_list const& cl) {
      return cl.ext_;
    }
    static std::unique_ptr<cell_locations> choose_locations(
            voxelized_shape const& liver, m cell_radius, cubic_meters_per_second proper_ha_flow) {
      return std::make_unique<lattice_locations>(liver, cell_radius, proper_ha_flow);
    }
  public:
    cell_list(build_tree_tag, std::mt19937& gen, voxelized_shape const& liver, m cell_radius, cubic_meters_per_second proper_ha_flow, Pa cell_pressure)
      : cells_{}, get_cell_id_{}, gen_(gen),
        cell_radius_{cell_radius},
        loc_{choose_locations(liver, cell_radius, proper_ha_flow)},
        ext_{extents(liver)}, proper_ha_flow_{proper_ha_flow}, cell_pressure_{cell_pressure} {
    }
    cell_list(load_tree_tag, boost::filesystem::path const& filename,
              voxelized_shape const& liver, std::mt19937& gen)
      : cells_{}, get_cell_id_{}, gen_{gen}, ext_{} {
      auto vt = load_protobuf<jhmi_message::VesselTree>(filename);
      cell_pressure_ = vt.cell_pressure() * pascals;
      if (std::abs(vt.cell_pressure()) < 1e-5)
        cell_pressure_ = 25_mmHg;
      if (std::abs(vt.tree_flow()) < 1e-5)
        proper_ha_flow_ = cubic_meters_per_second{400. * mL / minutes};
      else
        proper_ha_flow_ = cubic_meters_per_second{vt.tree_flow() * meters * meters * meters / seconds};

      int max_id = 0;
      boost::optional<cube<m3>> ext;
      RANGES_FOR(auto& vtc, vt.macrocells()) {
        macrocell c{dbl3{vtc.x(), vtc.y(), vtc.z()}*meters,
                    vessel_id{vtc.parent_vessel()},
                    cell_type::normal,
                    vtc.radius()*meters,
                    cell_id{vtc.id()},
                    vtc.flow()*boost::units::pow<3>(meters) / seconds,
                    vtc.pressure() * pascals,
                    int3{vtc.idx_x(), vtc.idx_y(), vtc.idx_z()}};
        cells_.insert(std::make_pair(c.id, c));
        max_id = std::max(max_id, vtc.id());
        ext = ext ? unite(*ext, extents(c)) : extents(c);
      }
      ext_ = *ext;
      if (!cells_.empty()) {
        cell_radius_ = cells_.begin()->second.radius;
        loc_ = choose_locations(liver, cell_radius_, proper_ha_flow_); 
      }
    }

    auto get_cell_flow() const { return loc_->get_flow(nullptr); }
    cell_id add_cell_near(m3 const& loc, boost::optional<m3> end_loc = boost::none) {
      auto near_loc = loc_->find_location(loc, end_loc, gen_);
      if (!near_loc) {
        return cell_id::invalid();
      }
      auto cell_id = get_cell_id_();
      auto new_cell = cells_.insert(std::make_pair(cell_id,
           macrocell{near_loc->first, vessel_id::invalid(), cell_type::normal,
                     cell_radius_, cell_id, loc_->get_flow(&gen_), cell_pressure_, near_loc->second}));
      loc_->add_item(new_cell.first->second);
      return cell_id;
    }
    void reduce_cell_size(double scale, bool fit_to_lobules) {
      cell_radius_ *= scale;
      RANGES_FOR(auto& cell, cells_)
        cell.second.radius *= scale;
      loc_->reset(cell_radius_, cells_, fit_to_lobules);
      fmt::print("# of potential cell sites: {}\n", loc_->number_of_cells());
    }
    void erase(cell_id id) {
      loc_->remove_item(at(id));
      cells_.erase(id);
    }
    macrocell& at(cell_id id) { return cells_.at(id); }
    macrocell const& at(cell_id id) const { return cells_.at(id); }

    auto list() const { return cells_ | ranges::view::values; }

    void store(jhmi_message::VesselTree& vt) const {
      vt.set_tree_flow(proper_ha_flow_.value()); 
      vt.set_cell_pressure(cell_pressure_.value());
      RANGES_FOR(auto& cell, cells_ | ranges::view::values) {
        auto vtc = vt.add_macrocells();
        vtc->set_id(cell.id.value());
        vtc->set_x(cell.center.x.value());
        vtc->set_y(cell.center.y.value());
        vtc->set_z(cell.center.z.value());
        vtc->set_radius(cell.radius.value());
        vtc->set_flow(cell.flow.value());
        vtc->set_pressure(cell.pressure.value());
        vtc->set_parent_vessel(cell.parent_vessel.value());
        vtc->set_idx_x(cell.idx.x);
        vtc->set_idx_y(cell.idx.y);
        vtc->set_idx_z(cell.idx.z);
      }
    }
    bool validate() const {
      bool all_good = true;
      RANGES_FOR(auto& cell, cells_) {
        auto& c = cell.second;
        if (cell.first != c.id) {
          fmt::print("Mismatch in cell id {} references {}\n", cell.first, c.id);
          all_good = false;
        }
        if (!c.parent_vessel.valid()) {
          fmt::print("Unconnected macrocell found in list: {}\n", c.id);
          all_good = false;
        }
        if (c.pressure != cell_pressure_) {
          fmt::print("Cell pressure invalid: {}\n", c.pressure);
          all_good = false;
        }
      }
      return all_good;
    }
  };
  bool operator==(cell_list const& lhs, cell_list const& rhs) {
    RANGES_FOR(auto const& lc, lhs.list()) {
      if (!(rhs.at(lc.id) == lc))
        return false;
    }
    return true;
  }
}
#endif
