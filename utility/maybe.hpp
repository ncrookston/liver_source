#ifndef JHMI_UTILITY_MAYBE_HPP_NRC_20170106
#define JHMI_UTILITY_MAYBE_HPP_NRC_20170106
#include <boost/optional.hpp>

namespace jhmi {
  template <typename F, typename T>
  auto maybe(boost::optional<T> const& ot, F f) {
    using opt = decltype(boost::make_optional(f(*ot)));
    return ot ? boost::make_optional(f(*ot)) : boost::none;
  }
}

#endif

