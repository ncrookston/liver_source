#include "liver/fill_liver_volume.hpp"
#include "liver/physical_vessel.hpp"
#include "liver/physical_vessel_tree_updater.hpp"
#include "messages/vessel_tree.pb.h"
#include "shape/voxelized_shape.hpp"
#include "utility/binary_tree.hpp"
#include "utility/git_hash.hpp"
#include "utility/options.hpp"
#include "utility/protobuf_zip_ostream.hpp"
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <boost/program_options.hpp>
#include <fmt/ostream.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <chrono>
#include <iostream>
#include <random>

using namespace jhmi;
namespace fs = boost::filesystem;
namespace po = boost::program_options;

struct node {
  m3 pt;
  binary_node_t<physical_vessel> n;
};
int main(int argc, char* argv[]) {
  try {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    auto opts = options<data_directory_option, output_path_option>{};
    if (!opts.parse(argc, argv))
      return 1;


    auto liver = voxelized_shape{opts.data_directory() / "liver_extents.datz", adjust::do_open};
    auto full_start = std::chrono::high_resolution_clock::now();
    auto tracts = tracts_in(liver) | ranges::view::transform([](m3 pt) { return node{pt, {}}; }) | ranges::to_vector;
    auto num_levels = std::ceil(std::log2(tracts.size())) + 1;
    auto levels = std::vector<std::vector<node>>(num_levels);
    levels.back() = std::move(tracts);

    auto vend = dbl3{45.447, 85.777, 54.023} * mm;
    for (int lvl = 1; lvl < num_levels; ++lvl) {
      auto& old_lvl = levels[num_levels - lvl];
      auto& new_lvl = levels[num_levels - lvl - 1];
      auto p = double(lvl) / (num_levels - 1);
      new_lvl = old_lvl | ranges::view::chunk(2)
        | ranges::view::transform([=](auto&& pts) {
            auto ctr = ranges::accumulate(pts | ranges::view::transform(&node::pt), m3{}) / double(pts.size());
            return node{ctr * (1. - p) + vend * p, {}};
        }) | ranges::to_vector;
    }
    int idx = 0;
    auto cell_pressure = Pa{25_mmHg};
    auto proper_ha_flow = 400. * mL / minutes;
    auto cell_flow = cubic_meters_per_second{proper_ha_flow / double(tracts.size())};
    auto vessels = binary_tree<physical_vessel>{
      physical_vessel{dbl3{50.143, 78.571, 47.562} * mm, vend, 10_um,
          cell_id::invalid(), cell_flow, cell_pressure, vessel_id{++idx}}};
    levels[0][0].n = vessels.root();
    for (int r = 1; r < num_levels; ++r) {
      for (int c = 0; c < levels[r].size(); ++c) {
        auto& parent = levels[r - 1][c / 2];
        auto& child = levels[r][c];
        auto dv = physical_vessel{parent.n.value().end(), vend, 10_um,
              cell_id::invalid(), cell_flow, cell_pressure, vessel_id{++idx}};
        if (c % 2 == 0)
          child.n = parent.n.set_left_child(dv);
        else
          child.n = parent.n.set_right_child(dv);
      }
    }
    //Now, update flow, pressure, and radii.
    auto vu = physical_vessel_tree_updater{vessels, 2.7, cell_pressure};
    vu.normalize_all();

    //tree is populated now, write to disk.
    auto p = opts.output_path() / "run_50_50";
    fs::create_directories(p);

    protobuf_zip_ostream out_stream{p / "fifty_tree.pbz"};
    jhmi_message::VesselTree vt;
    RANGES_FOR(auto&& n, vessels | view::node_level_order) {
      auto p = n.parent();
      auto r = n.right_child();
      auto l = n.left_child();
      auto vtv = vt.add_vessels();
      auto& v = n.value();
      vtv->set_id(v.id().value());
      vtv->set_parent((p ? p.value().id() : vessel_id::invalid()).value());
      vtv->set_left(  (l ? l.value().id() : vessel_id::invalid()).value());
      vtv->set_right( (r ? r.value().id() : vessel_id::invalid()).value());
      vtv->set_radius(v.radius().value());
      vtv->set_cell(v.cell().value());
      vtv->set_flow(v.flow().value());
      vtv->set_entry_pressure(v.entry_pressure().value());
      vtv->set_exit_pressure(v.exit_pressure().value());
      vtv->set_sx(v.start().x.value());
      vtv->set_sy(v.start().y.value());
      vtv->set_sz(v.start().z.value());
      vtv->set_ex(v.end().x.value());
      vtv->set_ey(v.end().y.value());
      vtv->set_ez(v.end().z.value());
      vtv->set_is_const(false);
    }
    if (!vt.SerializeToZeroCopyStream(out_stream.get()))
      throw std::runtime_error("Failed to write 50/50 tree.");

    auto full_stop = std::chrono::high_resolution_clock::now();
    fmt::print("Done in {} s!\n",
      std::chrono::duration<float>(full_stop - full_start).count());
    google::protobuf::ShutdownProtobufLibrary();
  }
  catch(std::exception const& e) {
    fmt::print("Caught exception: {}\n", e.what());
  }
  return 0;
}

