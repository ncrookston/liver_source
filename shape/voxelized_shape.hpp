#ifndef JHMI_VOXELIZED_SHAPE_HPP_NRC20150504
#define JHMI_VOXELIZED_SHAPE_HPP_NRC20150504

#include "utility/volume_image.hpp"
#include <range/v3/algorithm/count_if.hpp>

namespace jhmi {

  enum class adjust { do_nothing, do_open };
  class voxelized_shape {
    volume_image<std::uint8_t> img_;
    friend cube<m3> extents(voxelized_shape const& s) { return extents(s.img_); }
  public:
    voxelized_shape() : img_{int3{}, cube<m3>{}} {}
    explicit voxelized_shape(boost::filesystem::path const& filename, adjust adj = adjust::do_nothing)
      : img_{filename} {
      if (adj == adjust::do_open)
        img_ = dilate(erode(img_));
    }

    bool operator()(m3 const& pt) const { return contains(extents(img_), pt) && img_(pt) != 0; }

    cubic_meters volume() const {
      if (img_.width() == 0 || img_.height() == 0 || img_.depth() == 0)
        return cubic_meters{0};
      auto pixel_sizes = element_divide(dimensions(extents(img_)), dbl3{img_.dimensions()});
      auto pixel_volume = pixel_sizes.x * pixel_sizes.y * pixel_sizes.z;
      return pixel_volume * double(ranges::count_if(img_, [](auto pix) { return pix > 0; }));
    }
  };
}

#endif
