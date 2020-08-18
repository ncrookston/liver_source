#ifndef JHMI_LIVER_PHYSICAL_VESSEL_HPP_NRC_20150820
#define JHMI_LIVER_PHYSICAL_VESSEL_HPP_NRC_20150820

#include "liver/constants.hpp"
#include "liver/utility.hpp"
#include "utility/cube.hpp"
#include "utility/line.hpp"
#include <random>

namespace jhmi {

  class physical_vessel {
    m3 start_, end_;
    m distance_;
    cell_id cell_;
    vessel_id id_;
    bool is_const_;
  public:
    m radius_;
    cubic_meters_per_second flow_;
    Pa entry_pressure_, exit_pressure_;

    friend bool operator==(physical_vessel const& lhs, physical_vessel const& rhs) {
      return nearly_equal(lhs.start_, rhs.start_)
        && nearly_equal(lhs.end_, rhs.end_)
        && nearly_equal(lhs.distance_, rhs.distance_)
        && nearly_equal(lhs.radius_, rhs.radius_)
        && lhs.cell_ == rhs.cell_
        && nearly_equal(lhs.flow_, rhs.flow_)
        && nearly_equal(lhs.entry_pressure_, rhs.entry_pressure_)
        && nearly_equal(lhs.exit_pressure_, rhs.exit_pressure_)
        && lhs.id_ == rhs.id_
        && lhs.is_const_ == rhs.is_const_;
    }

    Pa delta_pressure() const {
      return flow_ * 8. * blood_viscosity * distance() / (pi * boost::units::pow<4>(radius_));
    }
  public:
    physical_vessel(m3 start, m3 end, m radius, cell_id cell,
           cubic_meters_per_second flow, Pa exit_pressure, vessel_id id,
           bool is_const = false)
      : start_{start}, end_{end}, distance_{jhmi::distance(start - end)},
        radius_{radius}, cell_{cell}, flow_{flow}, entry_pressure_{},
        exit_pressure_{exit_pressure}, id_{id}, is_const_{is_const}
    {
      entry_pressure_ = exit_pressure_ + delta_pressure();
    }
    m3 const& start() const { return start_; }
    m3 const& end() const { return end_; }
    m radius() const { return radius_; }
    cell_id cell() const { return cell_; }
    cubic_meters_per_second flow() const { return flow_; }
    Pa entry_pressure() const { return entry_pressure_; }
    Pa exit_pressure() const { return exit_pressure_; }
    vessel_id id() const { return id_; }
    bool is_const() const { return is_const_; }

    m distance() const { return distance_; }
    jhmi::line<m3> line() const { return {start_, end_}; }
    bool validate() const {
      return check_close(entry_pressure_, exit_pressure_ + delta_pressure(), 1e-3, id_,
        "entry pressure", "exit pressure");
    }

    void set_exit_values(m l, m r, cubic_meters_per_second flow, Pa exit_pressure, double gamma) {
      using boost::units::pow;
      radius_ = std::pow(std::pow(l.value(), gamma) + std::pow(r.value(), gamma), 1./gamma) * meters;
      flow_ = flow;
      exit_pressure_ = exit_pressure;
      entry_pressure_ = exit_pressure + delta_pressure();
    }
    void set_start(m3 start) {
      start_ = start;
      distance_ = jhmi::distance(start_ - end_);
      entry_pressure_ = exit_pressure_ + delta_pressure();
    }
    void set_end(m3 end) {
      end_ = end;
      distance_ = jhmi::distance(start_ - end_);
      entry_pressure_ = exit_pressure_ + delta_pressure();
    }
    void scale_radius(double scalar, Pa entry_pressure) {
      radius_ *= scalar;
      entry_pressure_ = entry_pressure;
      exit_pressure_ = entry_pressure - delta_pressure();
    }
    void set_radius(m radius) {
      if (0_um >= radius)
        throw std::runtime_error(fmt::format("Invalid radius set: {}\n", radius));
      radius_ = radius;
      entry_pressure_ = exit_pressure_ + delta_pressure();
    }
    void scale_flow(double scalar) {
      flow_ *= scalar;
      entry_pressure_ = exit_pressure_ + delta_pressure();
    }
    int strahler_order = 1;
  };

  vessel_id id(physical_vessel const& v) {
    return v.id();
  }
  template <typename Convert, typename F>
  inline F traverse(physical_vessel const& v, Convert c, F f) {
    return bresenham(c(v.start()), c(v.end()), f);
  }
  inline cube<m3> extents(physical_vessel const& v) {
    return {v.start(), v.end()};
  }
}
#endif
