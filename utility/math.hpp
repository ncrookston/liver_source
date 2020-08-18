#ifndef JHMI_UTILITY_MATH_HPP_NRC_20160611
#define JHMI_UTILITY_MATH_HPP_NRC_20160611

#include "utility/meta.hpp"
#include <range/v3/all.hpp>

namespace jhmi {
  namespace jhmi_detail {
    template <typename Range, typename M>
    auto variance(Range const& r, M m) {
      return ranges::accumulate(r, decltype(m*m){}, [&](auto s, auto const& v) {
        return s + (v - m) * (v - m);
      }) / double(ranges::distance(r));
    }
  }//detail
  template <typename Range>
  auto mean(Range const& r) {
    return ranges::accumulate(r, ranges::range_value_t<Range>{}) / double(ranges::distance(r));
  }
  template <typename Range>
  auto variance(Range const& r) {
    auto m = mean(r);
    return jhmi_detail::variance(r, m);
  }

  template <typename Range>
  auto mean_std(Range const& r) {
    auto m = mean(r);
    using std::sqrt;
    return std::make_pair(m, sqrt(jhmi_detail::variance(r, m)));
  }

  template <typename Range>
  auto median(Range const& r) {
    auto rc = r | ranges::to_vector;
    auto n = rc.size() / 2;
    ranges::nth_element(rc, rc.begin() + n);
    return rc[n];
  }
  template <typename T> T clamp(T val, identity_t<T> low, identity_t<T> high) {
    return std::min<T>(std::max<T>(val, low), high);
  }
}

#endif
