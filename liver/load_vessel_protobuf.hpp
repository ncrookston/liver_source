#ifndef JHMI_LIVER_LOAD_VESSEL_PROTOBUF_NRC_2016_02_10
#define JHMI_LIVER_LOAD_VESSEL_PROTOBUF_NRC_2016_02_10

#include "messages/vessel_tree.pb.h"
#include "utility/load_protobuf.hpp"
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <boost/filesystem.hpp>

namespace jhmi {
  auto load_vessel_protobuf(jhmi_message::VesselTree const& vt) {
    vidx_to<jhmi_detail::flat_vessel> vns;
    RANGES_FOR(auto&& vtv, vt.vessels()) {

      physical_vessel v{dbl3{vtv.sx(), vtv.sy(), vtv.sz()}*meters,
                             dbl3{vtv.ex(), vtv.ey(), vtv.ez()}*meters,
                             vtv.radius()*meters,
                             cell_id{vtv.cell()},
                             vtv.flow() * boost::units::pow<3>(meters) / seconds,
                             vtv.exit_pressure() * pascals,
                             vessel_id{vtv.id()}, vtv.is_const()};
      vessel_id parent{vtv.parent()};
      vessel_id left{vtv.left()};
      vessel_id right{vtv.right()};
      vns.insert(std::make_pair(v.id(), jhmi_detail::flat_vessel{v, parent, left, right}));
    }
    return vns;
  }
}//jhmi

#endif
