#ifndef JHMI_LIVER_GET_SPLIT_POINT_HPP_NRC_20151028
#define JHMI_LIVER_GET_SPLIT_POINT_HPP_NRC_20151028

#include "liver/constants.hpp"
#include "liver/physical_vessel.hpp"
#include "utility/units.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <fmt/ostream.h>
#include <fstream>

namespace jhmi {

  namespace jhmi_detail {
    using circle = std::pair<glm::vec2,double>;

    auto angle_between(m3 const& v1, m3 const& v2) {
      return acos(dot(v1,v2) / (distance(v1)*distance(v2))).value();
    }
    std::pair<glm::vec2, glm::vec2> circle_intersect_pts(circle const& c1, circle const& c2) {
      auto d = glm::length(c1.first - c2.first);
      auto x1 = c1.first.x; auto y1 = c1.first.y;
      auto x2 = c2.first.x; auto y2 = c2.first.y;
      auto r1 = c1.second;  auto r2 = c2.second;
      auto scalar = std::sqrt(((r1+r2)*(r1+r2)-d*d)*(d*d-(r1-r2)*(r1-r2)));
      auto xx1 = (x2+x1)/2 + (x2-x1)*(r1*r1-r2*r2)/(2*d*d) + (y2-y1)/(2*d*d)*scalar;
      auto yy1 = (y2+y1)/2 + (y2-y1)*(r1*r1-r2*r2)/(2*d*d) - (x2-x1)/(2*d*d)*scalar;
      auto xx2 = (x2+x1)/2 + (x2-x1)*(r1*r1-r2*r2)/(2*d*d) - (y2-y1)/(2*d*d)*scalar;
      auto yy2 = (y2+y1)/2 + (y2-y1)*(r1*r1-r2*r2)/(2*d*d) + (x2-x1)/(2*d*d)*scalar;
      return {glm::vec2{xx1,yy1}, glm::vec2{xx2,yy2}};
    }
    bool inside_triangle(glm::vec2 const& pt, glm::vec2 const& t0, glm::vec2 const& t1, glm::vec2 const& t2) {
        auto a = 1/(-t1.y*t2.x+t0.y*(-t1.x+t2.x)+ t0.x*(t1.y-t2.y)+t1.x*t2.y);
        auto s = a*(t2.x*t0.y-t0.x*t2.y+(t2.y-t0.y)*pt.x + (t0.x-t2.x)*pt.y);
        if (s<0)
          return false;
        auto t = a*(t0.x*t1.y-t1.x*t0.y+(t0.y-t1.y)*pt.x + (t1.x-t0.x)*pt.y);
        return t >= 0 && 1 - s - t >= 0;
    }
    boost::optional<glm::vec2> find_true_center(
      double theta, glm::vec2 const& c1, glm::vec2 const& c2,
        glm::vec2 const& p0, glm::vec2 const& p1, glm::vec2 const& p2) {
      if (theta > pi/2) {
        if (!inside_triangle(c1, p0, p1, p2))
          return c1;
        else if (!inside_triangle(c2, p0, p1, p2))
          return c2;
      }
      else {
        if (inside_triangle(c1, p0, p1, p2))
          return c1;
        else if (inside_triangle(c2, p0, p1, p2))
          return c2;
      }
      return boost::none;
    }
    boost::optional<circle> get_circle(double theta, glm::vec2 const& p1,
       glm::vec2 const& p2, glm::vec2 const& p0) {
      auto l12 = glm::length(p1-p2);
      auto r12 = l12 / (2 * std::sin(theta));
      auto pts = circle_intersect_pts(circle{p1,r12}, circle{p2,r12});
      auto ocenter = find_true_center(theta, pts.first, pts.second, p0, p1, p2);
      if (ocenter)
        return circle{*ocenter, r12};
      return boost::none;
    }

    glm::vec3 to_glm(m3 const& p) {
      return {p.x.value(), p.y.value(), p.z.value()};
    }
    glm::mat3 make_rotation(glm::vec3 const& p0, glm::vec3 const& p1, glm::vec3 const& p2) {
      auto off = glm::normalize(glm::cross(p1-p0, p2-p1));
      auto angle = glm::angle(glm::vec3{0,0,1}, off);
      auto axis = glm::normalize(glm::cross(glm::vec3{0,0,1}, off));

      auto sina = std::sin(angle);
      auto cosa = std::cos(angle);
      return glm::mat3{cosa} + glm::outerProduct(axis, axis) * (1 - cosa)
        + glm::mat3{0, -axis.z, axis.y,  axis.z, 0, -axis.x,  -axis.y, axis.x, 0} * sina;
    }
    boost::optional<glm::vec2> find_xy_point(glm::vec2 const& p0,
       glm::vec2 const& p1, glm::vec2 const& p2, double th1, double th2) {
      auto c12 = get_circle(th1+th2, p1, p2, p0);
      auto c01 = get_circle(pi-th1, p0, p1, p2);

      if (!c12 || !c01)
        return boost::none;
      auto bs = circle_intersect_pts(*c12, *c01);
      if (glm::length(bs.first - p1) > glm::length(bs.second - p1))
        return bs.first;
      return bs.second;
    }
    m3 find_bifurcation_point(m3 const& p0, m3 const& p1, m3 const& p2,
                      cubic_meters_per_second f1, cubic_meters_per_second f2, double gamma) {
      auto togmo = 2. / gamma - 1.;
      auto omtog = -togmo;
      auto tmfog = 2. * omtog;

      assert(f1 >= f2);
      auto r = f1 / (f1+f2);
      auto th1 = std::acos(.5*(std::pow(r, togmo)+std::pow(r, omtog)-std::pow(r, togmo)*std::pow(1-r, tmfog)));
      auto th2 = std::acos(.5*(std::pow(r, togmo)-std::pow(r, omtog)+std::pow(r, togmo)*std::pow(1-r, tmfog)));

      auto g0 = to_glm(p0); auto g1 = to_glm(p1); auto g2 = to_glm(p2);
      auto rm = make_rotation(g0, g1, g2);

      auto xy0 = rm*g0; auto xy1 = rm*g1; auto xy2 = rm*g2;
      auto xy_pb = find_xy_point(glm::vec2{xy0}, glm::vec2{xy1}, glm::vec2{xy2}, th1, th2);
      if (!xy_pb) {
        return (p0+p1+p2) / 3.;
      }
      auto gpb = glm::transpose(rm) * glm::vec3{*xy_pb, xy0.z};
      auto pb = dbl3{gpb.x, gpb.y, gpb.z} * meters;

      auto ath1 = angle_between(p1-pb,pb-p0);
      auto ath2 = angle_between(p2-pb,pb-p0);

      if (isnan(ath1) || isnan(ath2) || std::abs(ath1 - th1) > 1e-2 || std::abs(ath2 - th2) > 1e-2) {
//        fmt::print("Unable to find valid bifurcation point\n{:1.15e},{:1.15e},{:1.15e}\n{:1.15e},{:1.15e},{:1.15e}\n{:1.15e},{:1.15e},{:1.15e}\n",
//            p0.x.value(), p0.y.value(), p0.z.value(),
//            p1.x.value(), p1.y.value(), p1.z.value(),
//            p2.x.value(), p2.y.value(), p2.z.value());
//        exit(0);
        return (p0+p1+p2) / 3.;
      }
      return pb;
    }
  }//jhmi_detail

  m3 find_bifurcation_point(m3 const& p0, m3 const& p1, m3 const& p2,
                      cubic_meters_per_second q1, cubic_meters_per_second q2, double gamma) {
    if (q1 > q2)
      return jhmi_detail::find_bifurcation_point(p0, p1, p2, q1, q2, gamma);
    return jhmi_detail::find_bifurcation_point(p0, p2, p1, q2, q1, gamma);
  }

  m3 get_split_point(physical_vessel const& v, m3 const& p, cubic_meters_per_second flow, double gamma) {
    if (v.is_const()) {
      auto unclipped_t = double(nearest_t(v.line(), p));
      auto t = std::min(1., std::max(0., unclipped_t));
      auto split_pt = v.start() + t * (v.end()-v.start());
      return split_pt;
    }

    if (v.flow() > flow)
      return jhmi_detail::find_bifurcation_point(v.start(), v.end(), p, v.flow(), flow, gamma);
    return jhmi_detail::find_bifurcation_point(v.start(), p, v.end(), flow, v.flow(), gamma);
  }
}
#endif
