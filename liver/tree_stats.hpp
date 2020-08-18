#ifndef JHMI_LIVER_PRINT_TREE_STATS_HPP_NRC_20171202
#define JHMI_LIVER_PRINT_TREE_STATS_HPP_NRC_20171202

#include "liver/macrocell_tree.hpp"
#include "utility/math.hpp"
#include "utility/volume_image.hpp"

namespace jhmi {
  struct tree_stats {
    double gamma;
    int num_macrocells;
    int num_locations;

    m pha_radius;
    cubic_meters_per_second pha_flow;
    Pa pha_pressure;

    m ta_radius;
    m ta_radius_std;
    cubic_meters_per_second ta_flow;
    cubic_meters_per_second ta_flow_std;
    Pa ta_pressure;

    m path_radius;
    m path_radius_std;
    cubic_meters_per_second path_flow;
    cubic_meters_per_second path_flow_std;
    m path_length;
    m path_length_std;
    double path_bifurcations;
    double path_bifurcations_std;
    double ta_over_25um;
    double num_trapping_locs;
  };

  tree_stats calc_tree_stats(macrocell_tree const& tree, voxelized_shape const& liver) {
    auto terminal_radii = tree.vessel_tree().terminal_vessels()
      | ranges::view::transform(&physical_vessel::radius) | ranges::to_vector;

    tree_stats ts;
    ts.gamma = tree.vessel_tree().gamma();
    ts.num_macrocells = terminal_radii.size();
    ts.num_locations = ranges::distance(tracts_in(liver));

    ts.pha_radius = ranges::front(tree.vessel_tree().vessels()).radius();
    ts.pha_flow = ranges::front(tree.vessel_tree().vessels()).flow();
    ts.pha_pressure = ranges::front(tree.vessel_tree().vessels()).entry_pressure();

    ts.ta_radius = mean(terminal_radii);
    ts.ta_radius_std = sqrt(variance(terminal_radii));
    auto terminal_flows = tree.vessel_tree().terminal_vessels() | ranges::view::transform(&physical_vessel::flow) | ranges::to_vector;
    ts.ta_flow = mean(terminal_flows);
    ts.ta_flow = sqrt(variance(terminal_flows));
    ts.ta_pressure = ranges::front(tree.vessel_tree().terminal_vessels()).exit_pressure();
    ts.ta_over_25um = ranges::count_if(terminal_radii, [](m r) { return r > 25_um / 2.; }) / double(ts.num_macrocells);

    m sph_rad = 25_um / 2.;
    ts.num_trapping_locs = ranges::count_if(tree.vessel_tree().vessel_nodes(),
      [sph_rad](auto n) {
        auto l = n.left_child();
        auto r = n.right_child();
        if (n.value().radius() > sph_rad) {
          if (!l && !r)
            return true;
          if (l && r && l.value().radius() < sph_rad && r.value().radius() < sph_rad)
            return true;
        }
        return false;
      });
    auto paths_bifurcations = std::vector<double>{};
    auto paths_lengths = std::vector<m>{};
    auto paths_radii = std::vector<m>{};
    auto paths_flows = std::vector<cubic_meters_per_second>{};
    for (auto& cell : tree.macrocells().list()) {
      auto n = tree.vessel_tree().at_node(cell.parent_vessel);
      auto path_bifurcations = 0;
      auto path_length = m{};
      auto path_radius = m{};
      auto path_flow = cubic_meters_per_second{};
      do {
        if (n.left_child() && n.right_child())
          ++path_bifurcations;
        path_radius += n.value().radius() * n.value().distance().value();
        path_flow += n.value().flow() * n.value().distance().value();
        path_length += n.value().distance();
        n = n.parent();
      } while (n);
      paths_bifurcations.push_back(path_bifurcations);
      paths_lengths.push_back(path_length);
      paths_radii.push_back(path_radius / path_length.value());
      paths_flows.push_back(path_flow / path_length.value());
    }
    ts.path_radius = mean(paths_radii);
    ts.path_radius_std = sqrt(variance(paths_radii));
    ts.path_flow = mean(paths_flows);
    ts.path_flow_std = sqrt(variance(paths_flows));
    ts.path_length = mean(paths_lengths);
    ts.path_length_std = sqrt(variance(paths_lengths));

    ts.path_bifurcations = mean(paths_bifurcations);
    ts.path_bifurcations_std = sqrt(variance(paths_bifurcations));
    return ts;
  }

  void write_tree_stats(tree_stats const& ts, boost::filesystem::path const& output_dir) {
    std::ofstream out((output_dir / "tree_stats.txt").string());

    out << fmt::format("tree_stat_tmp.gamma = {};\n", ts.gamma);
    out << fmt::format("tree_stat_tmp.num_macrocells = {};\n", ts.num_macrocells);
    out << fmt::format("tree_stat_tmp.num_locations = {};\n", ts.num_locations);
    out << fmt::format("tree_stat_tmp.pha_radius = {};\n", ts.pha_radius.value());
    out << fmt::format("tree_stat_tmp.pha_flow = {};\n", ts.pha_flow.value());
    out << fmt::format("tree_stat_tmp.pha_pressure = {};\n", ts.pha_pressure.value());
    out << fmt::format("tree_stat_tmp.ta_radius = {};\n", ts.ta_radius.value());
    out << fmt::format("tree_stat_tmp.ta_radius_std = {};\n", ts.ta_radius_std.value());
    out << fmt::format("tree_stat_tmp.ta_flow = {};\n", ts.ta_flow.value());
    out << fmt::format("tree_stat_tmp.ta_pressure = {};\n", ts.ta_pressure.value());
    out << fmt::format("tree_stat_tmp.path_radius = {};\n", ts.path_radius.value());
    out << fmt::format("tree_stat_tmp.path_radius_std = {};\n", ts.path_radius_std.value());
    out << fmt::format("tree_stat_tmp.path_flow = {};\n", ts.path_flow.value());
    out << fmt::format("tree_stat_tmp.path_flow_std = {};\n", ts.path_flow_std.value());
    out << fmt::format("tree_stat_tmp.path_length = {};\n", ts.path_length.value());
    out << fmt::format("tree_stat_tmp.path_length_std = {};\n", ts.path_length_std.value());
    out << fmt::format("tree_stat_tmp.path_bifurcations = {};\n", ts.path_bifurcations);
    out << fmt::format("tree_stat_tmp.path_bifurcations_std = {};\n", ts.path_bifurcations_std);
    out << fmt::format("tree_stat_tmp.ta_over_25um = {};\n", ts.ta_over_25um);
    out << fmt::format("tree_stat_tmp.num_trapping_locs = {};\n", ts.num_trapping_locs);
  }

  void print_tree_stats(tree_stats const& ts) {
    fmt::print("Murray's bifurcation constant used: {}\n", ts.gamma);
    fmt::print("First vessel radius: {} um\n", 1e6*ts.pha_radius.value());
    fmt::print("First vessel flow: {}\n", ts.pha_flow);
    fmt::print("Desired entry pressure: {}\n", input_pressure);
    fmt::print("First vessel entry pressure: {}\n", ts.pha_pressure);
    fmt::print("Macrocell pressure {}\n", ts.ta_pressure);
    fmt::print("Macrocell flow: {} ({})\n", ts.ta_flow, ts.ta_flow_std);
    fmt::print("Terminal radii stats: {} ({}) um\n", 1e6*ts.ta_radius.value(), 1e6*ts.ta_radius_std.value());

    fmt::print("Percent of portal tracts filled: {}\n", 100. * ts.num_macrocells / double(ts.num_locations));
    fmt::print("Number of portal tracts: {}\n", ts.num_macrocells);
    fmt::print("Approximate number of radiized trapping locs: {}\n", ts.num_trapping_locs);
    fmt::print("Average # of bifurcations from source to tract: {} ({})\n", ts.path_bifurcations, ts.path_bifurcations_std);
    fmt::print("Average radius from source to tract: {} ({}) um\n", 1e6*ts.path_radius.value(), 1e6*ts.path_radius_std.value());
    fmt::print("Average length from source to tract: {} ({}) um\n", 1e6*ts.path_length.value(), 1e6*ts.path_length_std.value());
    fmt::print("Percent of terminal arterioles that could potentiall fit a 25 um sphere: {}\n", 100 * ts.ta_over_25um);
  }
}//jhmi

#endif
