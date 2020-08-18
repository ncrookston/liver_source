#ifndef JHMI_FILL_LIVER_VOLUME_HPP_NRC_20141022
#define JHMI_FILL_LIVER_VOLUME_HPP_NRC_20141022

#include "utility/cube.hpp"
#include "utility/pt3.hpp"
#include "utility/units.hpp"
#include "utility/views.hpp"
#include "shape/voxelized_shape.hpp"
#include <boost/optional.hpp>
#include <range/v3/algorithm.hpp>
#include <cmath>
#include <cstddef>

namespace jhmi {
  namespace lobule {
    //Source: Heterogeneity of microsphere distribution in resected liver and tumor tissue
    // following selective intrahepatic radiotherapy by J. Hogberg et al.
    //A lobule weights 2 mg, 1 portal tract per mg of liver tissue.

    //Source: Targeted radionuclide therapy has the potential to selectively deliver
    // radiation to diseased cells with minimal toxicity to surrounding tissues.
    // M Sathekge et al.
    //SIRT injects radioembolizers via the hepatic artery.
    // Liver metastases are primarily fed via the hepatic artery, whereas normal liver
    // tissue is fed primarily via the portal vein

    //Source: Hepatic Structural Dosimetry in 90Y Microsphere Treatment: A Monte Carlo
    // Modeling Approach Based on Lobular Microanatomy Seza A. Gulec et al.
    constexpr static m const side_length = 0.693_mm;
    constexpr static m const min_radius = 0.6_mm;
    constexpr static m const cell_thickness = 1.5_mm;

    //Source: Portal Hypertension: Pathobiology, Evaluation, and Treatment
    // By Arun J. Sanyal, Vijay H. Shah.
    //static m const centrilobular_vein_radius = 32.5_um;
    //static m const portal_tract_vein_radius = 20_um;//Leads to sinusiods
    //static m const hepatic_arteriole_radius = 7.5_um;//Leads to sinusiods

    //Probably better: Crawford, Lin and Crawford
    // Normal Adult Human Liver Biopsy: A Quantitative Reference Standard
    constexpr static m const centrilobular_vein_radius = 33.45_um;//std 20.15
    constexpr static m const portal_tract_vein_radius = 17.5_um;//std 12.25
    constexpr static m const hepatic_arteriole_radius = 5.9_um;//std 2.35

    //   /\/\/\       y
    //  | | | |       ^
    //   \/\/\/       |-->x
    // z is perpindicular to the x-y plane shown above.
    constexpr static auto const xstep = lobule::min_radius;//Distance between centers of adjacent rows
    constexpr static auto const ystep = lobule::side_length*3.;//Distance between centers in same row
    constexpr static auto const zstep = lobule::cell_thickness;//Thickness of each row of lobules

    constexpr static auto const xoff = xstep / 2.;
    constexpr static auto const yoff0 = 5. * lobule::side_length / 2.;
    constexpr static auto const yoff1 = lobule::side_length;
    constexpr static auto const zoff = zstep / 2.;

    constexpr static auto const dloff = m3{lobule::min_radius, lobule::side_length / 2., 0_mm};
    constexpr static auto const doff = m3{0_mm, lobule::side_length, 0_mm};
  }//end lobule

  //This calls a functor f for each lobule center of a given volume.
  //  Volume must be callable with an m3, returning true if that point is
  //    contained, false otherwise.
  //  F is required to accept an m3 and int3 to its function call operator.
  template <typename Shape, typename F>
  void for_lobule(Shape const& shape, F f) {
    using namespace lobule;
    auto cube = extents(shape);
    int zidx = 0;
    for (auto z = cube.ul().z + zoff; z < cube.lr().z; z += zstep, ++zidx) {
      int xidx = 0;
      for (auto x = cube.ul().x + xoff; x < cube.lr().x; x += xstep, ++xidx) {
        int yidx = 0;
        auto yoff = xidx % 2 == 0 ? yoff0 : yoff1;
        for (auto y = cube.ul().y + yoff; y < cube.lr().y; y += ystep, ++yidx) {
          auto pt = m3{x,y,z};
          if (shape(pt))
            f(pt, int3{xidx,yidx,zidx});
        }//y
      }//x
    }//z
  }
  template <typename Shape>
  auto lobules_in(Shape const& shape) {
    namespace rv = ranges::view;
    auto ex = extents(shape);
    using namespace lobule;
    return rv::for_each(view::step(ex.ul().z + zoff, ex.lr().z, zstep),
      [=](m z) { return rv::for_each(view::step(ex.ul().x + xoff, ex.lr().x, xstep),
      [=, yoff = yoff1](m x) mutable {
        yoff = yoff < yoff0 ? yoff0 : yoff1;
        return rv::for_each(view::step(ex.ul().y + yoff, ex.lr().y, ystep),
          [=](m y) { return ranges::yield(m3{x,y,z}); }); }); })
      | rv::filter([&](m3 const& pt) { return shape(pt); });
  }

  namespace jhmi_detail {
    int to_idx(m p, m step, m offset) {
      return int(std::round((p - offset) / step));
    }
    int3 to_idx(m3 const& pt) {
      using namespace lobule;
      int x = to_idx(pt.x, xstep, xoff);
      int z = to_idx(pt.z, zstep, zoff);
      int y = to_idx(pt.y, ystep, x % 2 == 0 ? yoff0 : yoff1);
      return int3{x,y,z};
    }
    m3 from_idx(int3 const& ipt) {
      using namespace lobule;
      return element_multiply(dbl3{ipt}, m3{xstep,ystep,zstep}) +
        m3{xoff, ipt.x % 2 == 0 ? yoff0 : yoff1, zoff};
    }
    int3 adjust_index(int3 const& pt, int xoffset) {
      return int3{pt.x + xoffset,
                  pt.y + (xoffset % 2 != 0 && pt.x % 2 == 0 ? 1 : 0),
                  pt.z};
    }
  }

  using opt_pts = boost::optional<std::pair<m3, int3>>;
  template <typename Shape>
  opt_pts find_near_lobule(Shape const& shape, m3 const& pt) {
    auto ext = extents(shape);
    auto ipt = jhmi_detail::to_idx(pt - ext.ul());
    auto mpt = jhmi_detail::from_idx(ipt) + ext.ul();
    if (!shape(mpt))
      return boost::none;
    return std::make_pair(mpt, ipt);
  }
  template <typename Shape>
  opt_pts find_near_tract(Shape const& shape, m3 const& pt) {
    using namespace lobule;
    auto ext = extents(shape);
    auto lpt = find_near_lobule(shape, pt);
    if (!lpt)
      return boost::none;

    auto get_closer_pt = [&](int xoff0, int xoff1) -> opt_pts {
      using namespace jhmi_detail;
      auto idx0 = adjust_index(lpt->second, xoff0);
      auto idx1 = adjust_index(lpt->second, xoff1);

      auto pt0 = from_idx(idx0) - doff + ext.ul();
      auto pt1 = from_idx(idx1) - dloff + ext.ul();
      if (distance_squared(pt - pt0) < distance_squared(pt - pt1))
        return shape(pt0) ? std::make_pair(pt0, int3{idx0.x,2*idx0.y,idx0.z}) : opt_pts{boost::none};
      else
        return shape(pt1) ? std::make_pair(pt1, int3{idx1.x,2*idx1.y+1,idx1.z}) : opt_pts{boost::none};
    };
    if (pt.x < lpt->first.x && pt.y < lpt->first.y)
      return get_closer_pt(0,0);
    else if (pt.x >= lpt->first.x && pt.y < lpt->first.y)
      return get_closer_pt(0,2);
    else if (pt.x < lpt->first.x && pt.y >= lpt->first.y)
      return get_closer_pt(-1,1);
    else
      return get_closer_pt(1,1);
  }

  template <typename Shape, typename F>
  void for_portal_tract(Shape const& shape, F f) {
    for_lobule(shape, [&](m3 const& mpt, int3 spt) {
      auto mpt1 = mpt - lobule::doff;
      spt.y *= 2;
      if (shape(mpt1))
        f(mpt1, spt);

      auto mpt2 = mpt - lobule::dloff;
      spt.y += 1;
      if (shape(mpt2))
        f(mpt2, spt);
    });
  }
#if 1
  template <typename Shape>
  auto tracts_in(Shape const& shape) {
    namespace rv = ranges::view;
    using type = decltype(lobules_in(shape));
    return lobules_in(shape) | rv::for_each([](m3 const& pt) {
      return rv::concat(rv::single(pt - lobule::doff),
                        rv::single(pt - lobule::dloff));
    }) | rv::filter([&](m3 const& pt) { return shape(pt); });
  }
#endif

  auto lobule_mask(m step) {
    auto sides = m3{lobule::min_radius, lobule::side_length, lobule::cell_thickness/2.};
    //Center the mask at the origin
    auto mask_ext = cube<m3>{-sides, sides};

    auto lobule_dims = int3{floor(2.*(mask_ext.lr()+dbl3{1,1,0}*step) / step)};
    auto mask = volume_image<bool>{lobule_dims, mask_ext};
    auto mid_y = lobule::side_length / 2.;
    RANGES_FOR(auto pix, mask | view::equal_step(step)) {
      //Operate only in one octant
      auto p = abs(pix.physical_loc);
      if (p.z > lobule::cell_thickness/2.)
        pix.value = false;
      else if (p.x > lobule::min_radius)
        pix.value = false;
      else {
        p.y - mid_y;
        pix.value = (p.y - mid_y < mid_y - p.x * mid_y / lobule::min_radius);
      }
    }
    return mask;
  }
}//end jhmi

#endif
