#include "messages/sphere_locs.pb.h"
#include "messages/sphere_tracts.pb.h"
#include "utility/options.hpp"
#include "utility/load_protobuf.hpp"
#include "utility/protobuf_zip_ostream.hpp"
#include "utility/units.hpp"
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <fmt/ostream.h>

using namespace jhmi;

int main(int argc, char* argv[]) {
  auto opts = options<spherefiles_option>{};
  if (!opts.parse(argc, argv))
    return 1;

  for (auto&& file : opts.spherefiles()) {
    auto protofile = load_protobuf<jhmi_message::SphereTracts>(file);
    jhmi_message::SphereLocs sl;
    int num_spheres = 0;
    auto sphere_diameter = protofile.sphere_diameter() * meters;
    for (auto&& tract : protofile.tracts()) {
      auto start = dbl3{tract.sx(), tract.sy(), tract.sz()} * meters;
      auto end   = dbl3{tract.ex(), tract.ey(), tract.ez()} * meters;
      auto sstep = sphere_diameter * (start - end) / distance(end - start);
      for (int i = 0; i < tract.num_spheres(); ++i) {
        auto sloc = end + double(i) * sstep;
        auto l = sl.add_locations();
        l->set_x(sloc.x.value());
        l->set_y(sloc.y.value());
        l->set_z(sloc.z.value());
        l->set_vessel_id(tract.vessel_id());
        ++num_spheres;
      }
    }
    auto new_file = file;
    new_file.replace_extension(".1.pbz");
    protobuf_zip_ostream output_stream{new_file};
    if (!sl.SerializeToZeroCopyStream(output_stream.get()))
      throw std::runtime_error("Failed to write spherefiles.");
    fmt::print("{} spheres written from {} to {}\n", num_spheres, file, new_file);
  }
  
  return 0;
}

