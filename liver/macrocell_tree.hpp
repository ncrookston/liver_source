#ifndef JHMI_LIVER_MACROCELL_TREE_HPP_NRC_20150803
#define JHMI_LIVER_MACROCELL_TREE_HPP_NRC_20150803

#include "liver/cell_list.hpp"
#include "liver/physical_vessel_tree.hpp"
#include "shape/voxelized_shape.hpp"
#include "utility/protobuf_zip_ostream.hpp"
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/rolling_mean.hpp>
#include <boost/filesystem.hpp>
#include <chrono>
#include <vector>

namespace jhmi {

  class macrocell_tree {
    std::mt19937 gen_;
    voxelized_shape const& liver_;
    physical_vessel_tree vessels_;
    cell_list cells_;

    int update_macrocells(float grow_prob, float die_prob) {
      std::uniform_real_distribution<> dist;
      auto rand = std::bind(dist, std::ref(gen_));

      //Determine which cells die
      std::vector<cell_id> dying;
      RANGES_FOR(auto& cell, cells_.list()) {
        if (rand() < die_prob && vessels_.remove(cell.parent_vessel))
          dying.push_back(cell.id);
      }
      //For each mc (in random order) attempt to fill empty spaces;
      std::vector<m3> clone_cells;
      RANGES_FOR(auto& cell, cells_.list()) {
        if (rand() < grow_prob)
          clone_cells.push_back(cell.center);
      }
      int num_died = dying.size();
      RANGES_FOR(auto id, dying) {
        cells_.erase(id);
      }
      ranges::shuffle(clone_cells, gen_);
      int num_created = 0;
      RANGES_FOR(auto center, clone_cells) {
        auto new_id = cells_.add_cell_near(center);
        if (new_id.valid()) {
          if (vessels_.connect_cell(cells_.at(new_id), gen_).valid()) {
            ++num_created;
          }
          else
            cells_.erase(new_id);
        }
      }
#define PER_SUBCYCLE
#ifdef PER_SUBCYCLE
      fmt::print("total: {:8} | attempted: {:6} | created: {:6} | died: {:6} | net: {:6}",
        vessels_.size(), clone_cells.size(), num_created, num_died, num_created - num_died);
#endif
      return num_created - num_died;
    }
    void fill_with_pcts(float grow_prob, float die_prob) {
#if 0
      using namespace boost::accumulators;
      //auto acc = accumulator_set<int, stats<tag::rolling_mean>>{tag::rolling_window::window_size = 20};
      do {
        acc(update_macrocells(grow_prob, die_prob));
# ifdef PER_SUBCYCLE
        fmt::print(" | avg: {}\n", rolling_mean(acc));
# endif
      } while (std::abs(rolling_mean(acc)) > 1);
#else
      while (update_macrocells(grow_prob, die_prob) > 0)
        fmt::print("\n");
      fmt::print("\n");
#endif
    }
    void normalize_all() {
      vessels_.normalize_all();
      //If we're solving for flow, we need to update the cell values afterward
    }

  public:
    static m initial_size() { return 5_mm / .3679; }
    macrocell_tree(build_tree_tag, boost::filesystem::path const& vesselfile,
                   voxelized_shape const& liver,
                   std::random_device::result_type seed,
                   cubic_meters_per_second proper_ha_flow,
                   double gamma, Pa cell_pressure, bool initial_fill = true)
      : gen_{seed}, liver_{liver},
        vessels_{build_tree, vesselfile, extents(liver_), gamma, cell_pressure, proper_ha_flow / 1868346., gen_},
        cells_{build_tree, gen_, liver_, initial_size(), proper_ha_flow, cell_pressure} {
      //We now add macrocells to each terminal vessel in the liver model.
      auto terminal_vessels = vessels_.terminal_vessels() | ranges::to_vector;
      for (auto&& v : terminal_vessels) {
        auto new_id = cells_.add_cell_near(v.start(), boost::optional<m3>{v.end()});
        if (!new_id.valid() || !vessels_.connect_cell_initial(cells_.at(new_id), v.id()).valid())
          throw std::runtime_error("Error adding an initial terminal cell");
      }
      //Finally, fill the liver with macrocells and connect them to this
      // initial map.
      if (initial_fill)
        fill_with_pcts(1.f, 0.f);
      normalize_all();
      if (!validate()) {
        throw std::runtime_error(fmt::format("Invalid tree detected!\n"));
      }
    }
    macrocell_tree(load_tree_tag, boost::filesystem::path const& treefile,
          voxelized_shape const& liver = voxelized_shape{})
      : gen_{0}, liver_{liver}, vessels_{load_tree, treefile, gen_},
        cells_{load_tree, treefile, liver, gen_} {
      //Takes a really long time, not strictly necessary
      //validate();
    }

    auto const& macrocells() const { return cells_; }
    physical_vessel_tree const& vessel_tree() const { return vessels_; }

    void write(boost::filesystem::path const& filename) const {
      namespace io = google::protobuf::io;
      jhmi_message::VesselTree vt;
      vessels_.store(vt);
      cells_.store(vt);
      protobuf_zip_ostream out_stream{filename};
      if (!vt.SerializeToZeroCopyStream(out_stream.get()))
        throw std::runtime_error("Failed to write macrocell tree.");
    }
    auto const& liver_shape() const { return liver_; }

    void reduce_cell_size(double scale, bool final) {
      assert((scale > 0 && scale < 1) || final);
      cells_.reduce_cell_size(scale, final);
    }
    void build(int cycles, m final_radius,
               boost::filesystem::path const& p = boost::filesystem::path{},
               std::string const& filestem = "") {
      float m1 = 1.f, m2 = 9.8f, n1 = .3f, n2 = 9.f;
      auto t = (final_radius - macrocell_tree::initial_size()) / double(cycles);
      RANGES_FOR(int cycle, ranges::view::ints(0, cycles)) {
        auto grow_prob = m1 * expf(-cycle / m2);
        auto die_prob = n1 * expf(-cycle / n2);
        bool fit_to_lobules = cycle == cycles - 1 && final_radius < 2_mm;
        reduce_cell_size(
          1. + t / (macrocell_tree::initial_size() + double(cycle) * t), fit_to_lobules);
        auto start_num_cells = ranges::size(macrocells().list());
        auto start = std::chrono::high_resolution_clock::now();

        fill_with_pcts(grow_prob, die_prob);
        vessels_.normalize_all();
        auto stop = std::chrono::high_resolution_clock::now();
        auto end_num_cells = ranges::size(macrocells().list());
        if (!validate()) {
          fmt::print("Invalid tree detected!\n");
        }
#if 1
        fmt::print("Cycle {:2} | Mitosis: {:10} | Necrosis: {:10} | Change: {:6} | Time: {} s\n",
          cycle + 1, grow_prob, die_prob, int(end_num_cells) - int(start_num_cells),
          std::chrono::duration<float>(stop - start).count());
#endif
        if (!filestem.empty() && cycle < cycles - 1) {
          write(p / fmt::format(filestem, cycle));
        }
      }

      auto start = std::chrono::high_resolution_clock::now();
      //How could this be necessary?
      vessels_.normalize_all();
      auto stop = std::chrono::high_resolution_clock::now();
      fmt::print("Time to normalize: {} s\n", std::chrono::duration<float>(stop - start).count());
    }

    bool validate() const {
      bool no_errors = vessels_.validate(ranges::empty(cells_.list())) && cells_.validate();
      //Verify each cell matches the exit pressure and flow of the parent_vessel
      RANGES_FOR(auto& cell, cells_.list()) {
        auto& v = vessels_.at(cell.parent_vessel);
//        no_errors &= check_close(v.flow(), cell.flow, 1e-8, v.id(),
//          "vessel flow", "cell flow");
        no_errors &= check_close(v.exit_pressure(), cell.pressure, 1e-4, v.id(),
          "vessel exit pressure", "cell pressure");
      }
      RANGES_FOR(auto& vessel, vessels_.vessels()) {
        try {
          if (vessel.cell().valid())
            cells_.at(vessel.cell());
        }
        catch(std::exception& e) {
          fmt::print("Vessel references non-existent cell\n");
          no_errors = false;
        }
      }
      if (!no_errors)
        fmt::print("Errors found in tree\n");
      return no_errors;
    }
  };

  bool operator==(macrocell_tree const& lhs, macrocell_tree const& rhs) {
    return lhs.macrocells() == rhs.macrocells() &&
           lhs.vessel_tree() == rhs.vessel_tree();
  }
}//jhmi
#endif
