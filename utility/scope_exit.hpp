#ifndef JHMI_UTILITY_SCOPE_EXIT_HPP_NRC_20160817
#define JHMI_UTILITY_SCOPE_EXIT_HPP_NRC_20160817

namespace jhmi {
  
  template <typename F>
  class scope_exit_impl {
    F f;
  public:
    scope_exit_impl(F f) : f(f) {}

    ~scope_exit_impl() { f(); }
  };

  template <typename F> scope_exit_impl<F> scope_exit(F f) { return {f}; }
}
#endif

