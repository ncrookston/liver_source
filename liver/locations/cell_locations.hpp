#ifndef JHMI_LIVER_LOCATIONS_CELL_LOCATIONS_HPP_NRC_20160521
#define JHMI_LIVER_LOCATIONS_CELL_LOCATIONS_HPP_NRC_20160521

#include "shape/voxelized_shape.hpp"
#include "utility/units.hpp"
#include <random>
#include <boost/optional.hpp>

namespace jhmi {
  class cell_locations {
  public:
    virtual ~cell_locations() = default;

    virtual boost::optional<std::pair<m3,int3>> find_location(m3 const& loc,
      boost::optional<m3> const& end_loc,
      std::mt19937& gen) const = 0;

    virtual void add_item(macrocell const& m) = 0;
    virtual void remove_item(macrocell const& m) = 0;
    virtual void reset(m cell_radius, cidx_to<macrocell> const& cells, bool fit_to_lobules) = 0;
    virtual cubic_meters_per_second get_flow(std::mt19937* gen) const = 0;
    virtual int number_of_cells() const = 0;
  };
}
#endif
