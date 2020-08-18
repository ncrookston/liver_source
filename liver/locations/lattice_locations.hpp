#ifndef JHMI_LIVER_LOCATIONS_LATTICE_LOCATIONS_HPP_NRC_20160521
#define JHMI_LIVER_LOCATIONS_LATTICE_LOCATIONS_HPP_NRC_20160521

#include "liver/fill_liver_volume.hpp"
#include "utility/make_balanced_sampler.hpp"
#include "utility/octtree.hpp"
#include <range/v3/action/remove_if.hpp>
#include <unordered_set>

namespace jhmi {
  namespace jhmi_detail {
    struct potential_loc {
      m3 loc;
      int3 idx;
    };
    auto extents(potential_loc const& l) { return cube<m3>{l.loc, l.loc}; }
    bool operator<(potential_loc const& lhs, potential_loc const& rhs) {
      return lhs.idx < rhs.idx;
    }
    auto hash_value(potential_loc const& l) {
      return boost::hash<int3>{}(l.idx);
    }
    auto distance_squared(potential_loc const& pl, m3 const& pt) {
      return distance_squared(pl.loc, pt);
    }
  }

  class lattice_locations : public cell_locations {
    cube<m3> ext_;
    m cell_radius_;
    voxelized_shape const& liver_;
    octtree<jhmi_detail::potential_loc> oct_;
    std::unordered_set<int3, boost::hash<int3>> occupied_;
    double curr_num_acini_;
    double max_num_acini_;
    cubic_meters_per_second proper_ha_flow_;

    void select_locations(bool fit_to_lobules) {
      auto new_oct = octtree<jhmi_detail::potential_loc>{extents(liver_)};
      auto ext = inflate(extents(liver_), -cell_radius_*dbl3{1,1,1});
      curr_num_acini_ = 0;
      if (!fit_to_lobules) {
        traverse(ext, 2. * cell_radius_, [&](m3 const& pt) {
          auto nearest = find_near_tract(liver_, pt);
          if (nearest) {
            new_oct.add_item(jhmi_detail::potential_loc{nearest->first, nearest->second});
            ++curr_num_acini_;
          }
        });
      }
      else {
        for_portal_tract(liver_, [&](m3 const& pt, int3 const& ipt) {
          new_oct.add_item(jhmi_detail::potential_loc{pt, ipt});
          ++curr_num_acini_;
        });
      }
      oct_ = std::move(new_oct);
    }
  public:
    lattice_locations(voxelized_shape const& liver, m cell_radius, cubic_meters_per_second proper_ha_flow)
      : ext_{extents(liver)}, cell_radius_{cell_radius}, liver_{liver},
        oct_{ext_}, occupied_{}, max_num_acini_{0}, proper_ha_flow_{proper_ha_flow} {
      select_locations(false);
      int num_acini = 0;
      for_portal_tract(liver_, [&](m3 const&, int3 const&) {
        ++num_acini;
      });
      max_num_acini_ = num_acini;
    }

    virtual boost::optional<std::pair<m3,int3>> find_location(m3 const& loc,
          boost::optional<m3> const& end_loc,
          std::mt19937& gen) const override {

      if (end_loc) {
        auto fnt = find_near_tract(liver_, *end_loc);
        if (!fnt || occupied_.find(fnt->second) != occupied_.end())
          return boost::none;
        return *fnt;
      }

      auto items = oct_.find_nearest_items(loc, 6. * cell_radius_)
        | ranges::action::remove_if([&](jhmi_detail::potential_loc const& pl) {
           return occupied_.find(pl.idx) != occupied_.end(); });
      if (items.empty())
        return boost::none;
      auto dist = make_balanced_sampler(items | ranges::view::transform([](auto&& pl) {
        return int(std::lround(pl.loc.z / lobule::cell_thickness));
      }) | ranges::to_vector);
      auto& item = items[dist(gen)];
      return std::make_pair(item.loc, item.idx);
    }

    virtual void add_item(macrocell const& m) override {
      occupied_.insert(m.idx);
    }
    virtual void remove_item(macrocell const& m) override {
      occupied_.erase(m.idx);
    }
    virtual void reset(m cell_radius, cidx_to<macrocell> const&, bool fit_to_lobules) override {
      cell_radius_ = cell_radius;
      select_locations(fit_to_lobules);
    }
    virtual cubic_meters_per_second get_flow(std::mt19937*) const override {
      return cubic_meters_per_second{proper_ha_flow_ / max_num_acini_};
    }
    virtual int number_of_cells() const override {
      return curr_num_acini_;
    }
  };
}
#endif
