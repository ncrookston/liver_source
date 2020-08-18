#ifndef JHMI_UTILITY_BRESENHAM_HPP_NRC_20150829
#define JHMI_UTILITY_BRESENHAM_HPP_NRC_20150829
#include "utility/pt3.hpp"
namespace jhmi {
  namespace jhmi_detail {
    template <typename T = int&>
    struct dec_val {
      using int_ver = dec_val<int>;
      explicit dec_val(T t) : v(t) {}
      T v;
      dec_val<T>& operator++() {
        --v;
        return *this;
      }
      dec_val<T>& operator+=(int i) {
        v += i;
        return *this;
      }
      dec_val<T>& operator-=(int i) {
        v += i;
        return *this;
      }
      bool operator>=(int i) {
        return -v >= i;
      }
      friend dec_val<T> operator*(int l, dec_val<T> const& r) {
        return dec_val<T>{l * r.v};
      }
    };
    template <typename T = int&>
    struct inc_val {
      using int_ver = inc_val<int>;
      explicit inc_val(T t) : v(t) {}
      T v;
      inc_val<T>& operator++() {
        ++v;
        return *this;
      }
      inc_val<T>& operator+=(int i) {
        v += i;
        return *this;
      }
      inc_val<T>& operator-=(int i) {
        v -= i;
        return *this;
      }
      bool operator>=(int i) {
        return v >= i;
      }
      friend inc_val<T> operator*(int l, inc_val<T> const& r) {
        return inc_val<T>{l * r.v};
      }
    };
    template <typename T1, typename T2, typename T3, typename F>
    void bresenham_impl(T1 s1, T2 s2, T3 s3, int e1, int e2, int e3, F f) {
      auto d1 = e1 - s1.v;
      auto d2 = e2 - s2.v;
      auto d3 = e3 - s3.v;
      typename T2::int_ver ep2{0};
      typename T3::int_ver ep3{0};
      for (; s1.v <= e1; ++s1) {
        f();
        ep2 += d2;
        if (2 * ep2 >= d1) {
          ++s2;
          ep2 -= d1;
        }
        ep3 += d3;
        if (2 * ep3 >= d1) {
          ++s3;
          ep3 -= d1;
        }
      }
    }
  }

#define JHMI_BRESENHAM(v1, v2, v3) \
  do \
  { \
    if (p.v1 > e.v1) \
      std::swap(p, e); \
    auto nf = [&] { f(p); }; \
    if (p.v2 < e.v2 && p.v3 < e.v3) \
      bresenham_impl(inc_val<>(p.v1), inc_val<>(p.v2), inc_val<>(p.v3), e.v1, e.v2, e.v3, nf); \
    else if (p.v2 < e.v2) \
      bresenham_impl(inc_val<>(p.v1), inc_val<>(p.v2), dec_val<>(p.v3), e.v1, e.v2, e.v3, nf); \
    else if (p.v3 < e.v3) \
      bresenham_impl(inc_val<>(p.v1), dec_val<>(p.v2), inc_val<>(p.v3), e.v1, e.v2, e.v3, nf); \
    else \
      bresenham_impl(inc_val<>(p.v1), dec_val<>(p.v2), dec_val<>(p.v3), e.v1, e.v2, e.v3, nf); \
  } while(false)

  template <typename F>
  F bresenham(int3 p, int3 e, F f) {
    using namespace jhmi_detail;
    int3 d = abs(e - p);

    if (d.x >= d.y && d.x >= d.z)
      JHMI_BRESENHAM(x,y,z);
    else if (d.y >= d.z)
      JHMI_BRESENHAM(y,x,z);
    else
      JHMI_BRESENHAM(z,x,y);
    return f;
  }

#undef JHMI_BRESENHAM

}
#endif
