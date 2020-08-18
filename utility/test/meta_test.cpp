
#include "utility/meta.hpp"
#define CATCH_CONFIG_MAIN
#include <catch.hpp>

template <typename T, typename U>
void foo(U lhs, jhmi::identity_t<U> rhs) {
  static_assert(std::is_same<T,U>::value, "");
}

TEST_CASE( "Checking identity", "[meta]" ) {

  foo<double>(1., 3.f);
  foo<int>(1, 3u);
  foo<int>(1, 3.f);
}
