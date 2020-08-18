#ifndef JHMI_UTILITY_TAGGED_INT_HPP_NRC_20150826
#define JHMI_UTILITY_TAGGED_INT_HPP_NRC_20150826
#include <functional>
#include <ostream>

namespace jhmi {
  template <typename Tag> class tagged_int {
    int i_;
    friend bool operator==(tagged_int<Tag> const& l, tagged_int<Tag> const& r) {
      return l.i_ == r.i_;
    }
    friend bool operator!=(tagged_int<Tag> const& l, tagged_int<Tag> const& r) {
      return l.i_ != r.i_;
    }
    friend bool operator<(tagged_int<Tag> const& l, tagged_int<Tag> const& r) {
      return l.i_ < r.i_;
    }
    friend std::ostream& operator<<(std::ostream& out, tagged_int<Tag> const& t) {
      out << t.i_;
      return out;
    }
    template <typename U> friend class id_generator;
  public:
    explicit tagged_int(int i) : i_(i) {}
    bool valid() const { return i_ != -1; }
    static auto invalid() { return tagged_int<Tag>{-1}; }
    int value() const { return i_; }
  };

  template <typename Tag> class id_generator {
    int i_;
  public:
    id_generator() : i_(0) {}
    explicit id_generator(int i) : i_(i+1) {}
    explicit id_generator(tagged_int<Tag> const& t) : i_(t.i_ + 1) {}

    auto operator()() { return tagged_int<Tag>{i_++}; }
  };

  template <typename Tag>
  auto make_generator(tagged_int<Tag> const& ti) { return id_generator<Tag>{ti}; }
}

namespace std {
  template <typename T> struct hash<jhmi::tagged_int<T>> {
    size_t operator()(jhmi::tagged_int<T> const& ti) const {
      return std::hash<int>()(ti.value());
    }
  };
}
#endif
