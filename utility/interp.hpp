#ifndef JHMI_UTILITY_INTERP_HPP_NRC_20160519
#define JHMI_UTILITY_INTERP_HPP_NRC_20160519

#include "utility/volume_image.hpp"

namespace jhmi {

  namespace jhmi_detail {
    template <typename T>
    T lerp_xy(volume_image<T> const& img,
                 int3 const& ul, double px, double py) {
      auto d = img.dimensions();
      auto uld = img(ul);
      auto ur = ul, ll = ul, lr = ul;
      if (ul.x + 1 < d.x) {
        ur += int3{1,0,0};
        lr += int3{1,0,0};
      }
      if (ul.y + 1 < d.y) {
        ll += int3{0,1,0};
        lr += int3{0,1,0};
      }
      auto ud = img(ul) * (1. - px) + img(ur) * px;
      auto ld = img(ll) * (1. - px) + img(lr) * px;
      return ud * (1. - py) + ld * py;
    }
  }
  template <typename T>
  T lerp(volume_image<T> const& img, m3 const& pt) {
    auto d = img.dimensions();
    auto ipt = dbl3{img.to_index(abs(pt))};
    if (ipt.x >= d.x || ipt.y >= d.y || ipt.z >= d.z)
      return T(0);
    if (ipt.x < 0)
      ipt.x = 0;
    if (ipt.y < 0)
      ipt.y = 0;
    if (ipt.z < 0)
      ipt.z = 0;

    auto lul = int3{floor(ipt)};
    auto p = ipt - lul;

    auto ld = jhmi_detail::lerp_xy(img, lul, p.x, p.y);
    if (lul.z + 1 >= img.depth())
      return ld;
    auto ud = jhmi_detail::lerp_xy(img, lul + int3{0,0,1}, p.x, p.y);
    return ld * (1. - p.z) + ud * p.z;
  }
}

#endif
