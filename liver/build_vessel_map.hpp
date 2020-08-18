#ifndef JHMI_LIVER_BUILD_VESSEL_MAP_HPP_NRC_2016_02_02
#define JHMI_LIVER_BUILD_VESSEL_MAP_HPP_NRC_2016_02_02

#include "liver/parse_vessel.hpp"
#include <boost/filesystem.hpp>
#include <range/v3/view.hpp>
#include <fstream>

namespace jhmi {
  auto build_vessel_map(boost::filesystem::path const& filename) {
    //Load vessels from the file.
    std::ifstream f{filename.string()};
    if (!f.is_open())
      throw std::runtime_error("Invalid file provided to build_vessel_map");
    auto vessel_map = jhmi_detail::make_vessel_map(std::move(f), 1 / .3679f); //The file is scaled to 1/20 volume, scale it back
    if (vessel_map.empty())
      throw std::invalid_argument("invalid initial vessel map");
    vessel_id max_id{0};
    RANGES_FOR(auto& v_id, vessel_map | ranges::view::keys) {
      max_id = std::max(v_id, max_id);
    }

    return std::make_pair(vessel_map, id_generator<vessel_tag>{max_id});
  }
  template <typename Node, typename AddVessel>
  void add_children_vessels(Node n, AddVessel add_vessel,
     vidx_to<jhmi_detail::flat_vessel> const& vessel_map) {
    add_vessel(n);
    auto current = vessel_map.at(id(n.value()));
    if (current.left_id.valid()) {
      add_children_vessels(n.set_left_child(vessel_map.at(current.left_id).v),
        add_vessel, vessel_map);
    }
    if (current.right_id.valid()) {
      add_children_vessels(n.set_right_child(vessel_map.at(current.right_id).v),
        add_vessel, vessel_map);
    }
    if (n.parent() && current.parent_id != id(n.parent().value()))
      throw std::runtime_error(fmt::format("Node expects {}, actual parent {}",
        current.parent_id, id(n.parent().value())));
  }
}//jhmi

#endif
