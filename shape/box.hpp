#ifndef JHMI_SHAPE_BOX_HPP_NRC_20160516
#define JHMI_SHAPE_BOX_HPP_NRC_20160516

#include "utility/cube.hpp"

namespace jhmi {

  class box {
    cube<m3> ext_;
    friend cube<m3> extents(box const& b) { return b.ext_; }
  public:
    explicit box(m3 const& ul, m3 const& lr) : ext_{ul, lr} {}

    bool operator()(m3 const& pt) const { return contains(ext_, pt); }
  };
}

#endif

