#ifndef JHMI_LIVER_LOCATIONS_GRID_LOCATIONS_HPP_NRC_20160521
#define JHMI_LIVER_LOCATIONS_GRID_LOCATIONS_HPP_NRC_20160521

#include "liver/locations/cell_locations.hpp"
#include "utility/grid.hpp"
#include <fmt/ostream.h>

namespace jhmi {
  class grid_locations : public cell_locations {
    grid<cell_id> grid_;
    m cell_radius_;
    voxelized_shape const& liver_;
  public:
    grid_locations(voxelized_shape const& liver, m cell_radius)
      : grid_{extents(liver), cell_radius / 2.}, cell_radius_{cell_radius}, liver_{liver}
    {}

    virtual boost::optional<std::pair<m3,int3>> find_location(m3 const& loc,
        boost::optional<m3> const& end_loc, std::mt19937& gen) const override {
      std::uniform_real_distribution<> dist{};
      auto rand = std::bind(dist, std::ref(gen));
      if (!liver_(loc)) {
        fmt::print("Near point is outside liver extents: {}\n", loc);
        return boost::none;
      }
      auto near_loc = loc;
      int tries_left = 100;
      while (tries_left > 0) {
        if (end_loc)
          near_loc = *end_loc + (*end_loc - loc) * rand();
        else
          near_loc = (4 * dbl3{rand(),rand(),rand()} - dbl3{2,2,2}) * cell_radius_  + loc;
        if (liver_(near_loc) && grid_(near_loc).empty())
          break;
        --tries_left;
      }
      if (tries_left == 0)
        return boost::none;
      return std::make_pair(near_loc, int3{});
    }

    virtual void add_item(macrocell const& m) override {
      grid_.add_item(m);
    }

    virtual void remove_item(macrocell const& m) override {
      grid_.remove_item(m);
    }

    virtual void reset(m cell_radius, cidx_to<macrocell> const& cells,
                       bool /*fit_to_lobules*/) override {
      cell_radius_ = cell_radius;
      auto new_grid = grid<cell_id>{extents(grid_), cell_radius_ / 2.};
      RANGES_FOR(auto& cell, cells)
        new_grid.add_item(cell.second);
      grid_ = std::move(new_grid);
    }

  };
}

#endif
