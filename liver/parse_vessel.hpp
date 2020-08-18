#ifndef JHMI_LIVER_PARSE_VESSEL_HPP_NRC_20150829
#define JHMI_LIVER_PARSE_VESSEL_HPP_NRC_20150829

#include "liver/physical_vessel.hpp"
#include "liver/constants.hpp"
#include <boost/algorithm/string.hpp>
#include <range/v3/algorithm.hpp>
#include <range/v3/getlines.hpp>
#include <range/v3/range_for.hpp>
#include <fmt/ostream.h>

namespace jhmi {
  namespace jhmi_detail {
    struct flat_vessel {
      physical_vessel v;
      vessel_id parent_id, left_id, right_id;
    };
    flat_vessel parse_vessel(std::vector<std::string> const& m, int idx, float scalar) {
      using boost::units::pow;
      if (m.size() != 13)
        throw std::runtime_error(fmt::format("Invalid v1 vessel file format: {}",m.size()));
      return {physical_vessel{scalar * dbl3{stod(m[0]),stod(m[1]),stod(m[2])}*mm,
                     scalar * dbl3{stod(m[3]),stod(m[4]),stod(m[5])}*mm,
                     scalar * stod(m[6])*mm,
                     cell_id::invalid(),//no connected macrocells, initially
                     cubic_meters_per_second{1 * pow<3>(meters) / seconds},
                     25_mmHg,
                     vessel_id{idx},
                     true},
              //parent, left, right
              vessel_id{stoi(m[7])}, vessel_id{stoi(m[8])},
              vessel_id{stoi(m[9])}};
    }

    auto make_vessel_map(std::istream&& in, float scalar) {
      vidx_to<flat_vessel> vns;
      std::vector<std::string> m;

      int idx = 0;
      RANGES_FOR (auto const& line, ranges::getlines(in)) {
        if (!line.empty() && line[0] != '#') {
          boost::split(m, line, boost::is_any_of(" \t"), boost::token_compress_on);
          auto vn = parse_vessel(m, idx++, scalar);
          vns.insert(std::make_pair(vn.v.id(), vn));
        }
      }
      return vns;
    }
    /**@return the ID of the parent vessel, throws a std::runtime_error if not present*/
    vessel_id find_parent_vessel(vidx_to<flat_vessel> const& vns) {
      auto it = ranges::find_if(vns, [](auto const& v) {
          return !v.second.parent_id.valid() && v.second.v.id().valid();
      });
      if (it == vns.end())
        throw std::runtime_error("No parent vessel found");
      return it->first;
    }

  }//jhmi_detail
}//jhmi

#endif
