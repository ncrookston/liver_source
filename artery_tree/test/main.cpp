#include "liver/macrocell_tree.hpp"
#include "utility/volume_image.hpp"
#include <boost/filesystem.hpp>
#define CATCH_CONFIG_MAIN
#include <catch.hpp>

using namespace jhmi;

TEST_CASE( "Storing and loading", "[macrocell_tree]" ) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    std::random_device::result_type seed = 100;
    auto initial_vessels = "../data/vtree_cycle0.txt";
    auto liver = voxelized_shape{"../data/liver_extents.datz"};
    const int cycles = 15;
    auto final_radius = 7_mm;
    auto tree = macrocell_tree{build_tree, initial_vessels, liver, seed, cubic_meters_per_second{400. * mL / minutes}, 2.7, 25_mmHg, true /*use_lattice*/};
    tree.build(cycles, final_radius);
    REQUIRE(tree.validate());
    auto saved_file = boost::filesystem::current_path() / "vessel_tree.pbz";
    tree.write(saved_file);

    auto tree2 = macrocell_tree{load_tree, saved_file};
    REQUIRE(tree == tree2);
    google::protobuf::ShutdownProtobufLibrary();
}
