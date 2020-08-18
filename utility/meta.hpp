#ifndef JHMI_UTILITY_META_HPP_NRC_20160620
#define JHMI_UTILITY_META_HPP_NRC_20160620

namespace jhmi {
  template <typename T>
  struct identity {
    using type = T;
  };
  template <typename T>
  using identity_t = typename identity<T>::type;
}

#endif
