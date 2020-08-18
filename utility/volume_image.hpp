#ifndef JHMI_VOLUME_IMAGE_HPP_NRC20150429
#define JHMI_VOLUME_IMAGE_HPP_NRC20150429

#include "cube.hpp"
#include "range/v3/view.hpp"
#include "range/v3/core.hpp"
#include "range/v3/view_facade.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <fmt/ostream.h>
#include <fstream>

namespace jhmi {
  namespace jhmi_detail {
    template <typename T, typename P, typename V> class by_location_view;
    inline std::string to_dtype(float) { return "float32"; }
    inline std::string to_dtype(double) { return "float64"; }
    inline std::string to_dtype(std::uint8_t) { return "uint8"; }
    inline std::string to_dtype(std::uint32_t) { return "uint32"; }
    inline std::string to_dtype(std::int32_t) { return "int32"; }
    template <typename U, typename T>
    inline std::string to_dtype(boost::units::quantity<U,T>) { return to_dtype(T{}); }
  }

  template <typename T>
  class volume_image {
    using reference = T&;
    using const_reference = T const&;

  public:
    struct physical_pixel_ref {
      physical_pixel_ref(m3 const& pp, int3 const&, reference v) : loc{pp}, value{v} {}
      m3 const loc;
      reference value;
    };
    struct image_pixel_ref {
      image_pixel_ref(m3 const&, int3 const& ip, reference v) : loc{ip}, value{v} {}
      int3 const loc;
      reference value;
    };
    struct pixel_ref {
      pixel_ref(m3 const& pp, int3 const& ip, reference v) : physical_loc{pp}, image_loc{ip}, value{v} {}
      m3 const physical_loc;
      int3 const image_loc;
      reference value;
    };
    struct physical_pixel {
      physical_pixel(m3 const& pp, int3 const&, const_reference v) : loc{pp}, value{v} {}
      m3 const loc;
      const_reference value;
    };
    struct image_pixel {
      image_pixel(m3 const&, int3 const& ip, const_reference v) : loc{ip}, value{v} {}
      int3 const loc;
      const_reference value;
    };
    struct pixel {
      pixel(m3 const& pp, int3 const& ip, const_reference v) : physical_loc{pp}, image_loc{ip}, value{v} {}
      m3 const physical_loc;
      int3 const image_loc;
      const_reference value;
    };

    volume_image() = default;
    volume_image(int3 const& size, cube<m3> const& physical_extents)
      : physical_extents_(physical_extents),
        w_(size.x), h_(size.y), d_(size.z),
        volume_(new T[w_*h_*d_]()),//Note, we default initialize each element of volume_
        half_pixel_sizes_{element_divide(jhmi::dimensions(physical_extents), 2. * size)}
    {}

    explicit volume_image(boost::filesystem::path const& filename) {
      std::ifstream file(filename.string(), std::ios::binary);
      if (!file.is_open()) {
        throw std::runtime_error(fmt::format(
          "Invalid file \"{}\" provided to volume_image", filename.string()));
      }
      boost::iostreams::filtering_istream in;
      in.push(boost::iostreams::zlib_decompressor{});
      in.push(file);
      std::string line;
      getline(in, line);
      std::vector<std::string> e;
      boost::split(e, line, boost::is_any_of(" "), boost::token_compress_on);
      physical_extents_ = cube<m3>{dbl3{stod(e[0]), stod(e[1]), stod(e[2])}*mm,
                                   dbl3{stod(e[3]), stod(e[4]), stod(e[5])}*mm};
      getline(in, line);
      w_ = stoi(line);
      getline(in, line);
      h_ = stoi(line);
      getline(in, line);
      d_ = stoi(line);
      volume_.reset(new T[w_ * h_ * d_]);
      getline(in, line);
      if (line != jhmi_detail::to_dtype(T{})) {
        throw std::invalid_argument(fmt::format(
          "Invalid attempt to construct {} from {}", jhmi_detail::to_dtype(T()), line));
      }
      in.read(reinterpret_cast<char*>(volume_.get()), w_ * h_ * d_ * sizeof(T));
      half_pixel_sizes_ = element_divide(
        jhmi::dimensions(physical_extents_), 2. * int3{w_,h_,d_});
    }

    dbl3 to_index(m3 const& pt) const {
      return dbl3(element_divide(
        element_multiply(pt - physical_extents_.ul() - half_pixel_sizes_, dbl3(w_,h_,d_)),
        jhmi::dimensions(physical_extents_)));
    }

    reference operator()(m3 const& pt) {
      return (*this)(int3(to_index(pt)));
    }
    const_reference operator()(m3 const& pt) const {
      return (*this)(int3(to_index(pt)));
    }

    reference operator()(int x, int y, int z) {
      assert(x + y * w_ + z * w_ * h_ < w_ * h_ * d_);
      return volume_[x + y * w_ + z * w_ * h_];
    }
    reference operator()(int3 const& pt) { return (*this)(pt.x, pt.y, pt.z); }
    const_reference operator()(int x, int y, int z) const {
      assert(x + y * w_ + z * w_ * h_ < w_ * h_ * d_);
      return volume_[x + y * w_ + z * w_ * h_];
    }
    const_reference operator()(int3 const& pt) const { return (*this)(pt.x, pt.y, pt.z); }

    //TODO: Is this returning a pointer to const?  I don't think so...
    auto begin() const { return volume_.get(); }
    auto begin() { return volume_.get(); }

    auto end() const { return volume_.get() + w_ * h_ * d_; }
    auto end() { return volume_.get() + w_ * h_ * d_; }

    int width() const { return w_; }
    int height() const { return h_; }
    int depth() const { return d_; }

    int3 dimensions() const { return {w_, h_, d_}; }

    void write(boost::filesystem::path const& filename) const {
      std::ofstream file{filename.string(), std::ios::binary};
      boost::iostreams::filtering_ostream out;
      out.push(boost::iostreams::zlib_compressor{});
      out.push(file);
      out << fmt::format("{} {} {} {} {} {}\n{}\n{}\n{}\n{}\n",
        physical_extents_.ul().x.value()*1000,
        physical_extents_.ul().y.value()*1000,
        physical_extents_.ul().z.value()*1000,
        physical_extents_.lr().x.value()*1000,
        physical_extents_.lr().y.value()*1000,
        physical_extents_.lr().z.value()*1000,
        width(), height(), depth(), jhmi_detail::to_dtype(T()));
      write_binary(out);
    }

  private:
    template <typename Stream>
    void write_binary(Stream& out) const {
      out.write(reinterpret_cast<char const*>(volume_.get()),
                w_ * h_ * d_ * sizeof(T));
    }

    template <typename U, typename P, typename V> friend class jhmi_detail::by_location_view;
    friend cube<m3> extents(volume_image<T> const& v) {
      return v.physical_extents_;
    }
    cube<m3> physical_extents_;
    int w_, h_, d_;
    std::unique_ptr<T[]> volume_;
    m3 half_pixel_sizes_;
  };

  namespace jhmi_detail {
  template <typename T, typename P, typename V>
  class by_location_view : public ranges::view_facade<by_location_view<T,P,V>> {
  public:
    friend struct ranges::range_access;
    V* img_;
    m3 step_;
    m3 center_;
    struct cursor {
      V* img_;
      m3 mul_;
      m3 step_;
      int3 iul_;
      int3 ilr_;
      int3 ipt_;

      cursor() = default;

      bool equal(cursor const& other) const { return ipt_ == other.ipt_; }
      auto read() const {
        return P{mul_ + element_multiply(dbl3{ipt_}, step_), ipt_, (*img_)(ipt_)};
      }
      void next() {
        if (++ipt_.x >= ilr_.x) {
          ipt_.x = iul_.x;
          if (++ipt_.y >= ilr_.y) {
            ipt_.y = iul_.y;
            ++ipt_.z;
          }
        }
      }
    };

    cursor make_cursor(bool do_ul) const {
      auto pe = img_->physical_extents_;
      auto d = img_->dimensions();
      int3 iul{0,0,0};
      int3 iend{0,0,d.z};
      auto ipt = do_ul ? iul : iend;

      auto hstep = step_ / 2.;
      auto mul = element_multiply(ceil(element_divide(pe.ul() - center_, hstep)), hstep) + center_;
      return {img_, mul + hstep, step_, iul, d, ipt};
    }

  public:
    cursor begin_cursor() const {
      return make_cursor(true);
    }
    cursor end_cursor() const {
      return make_cursor(false);
    }

    explicit by_location_view(V& img) : img_(&img),
      step_(element_divide(dimensions(extents(img)), dbl3{img.dimensions()})),
      center_{extents(img).ul() + step_ / 2.} {}
    by_location_view(V& img, m3 const& step, m3 const& center)
      : img_(&img), step_(step), center_{center} {}
  };
  }//jhmi_detail

  template <typename T>
  struct by_const_physical_location_view : jhmi_detail::by_location_view<T, typename volume_image<T>::physical_pixel, volume_image<T> const> {
    using jhmi_detail::by_location_view<T, typename volume_image<T>::physical_pixel, volume_image<T> const>::by_location_view;
  };
  template <typename T>
  struct by_physical_location_view : jhmi_detail::by_location_view<T, typename volume_image<T>::physical_pixel_ref, volume_image<T>> {
    using jhmi_detail::by_location_view<T, typename volume_image<T>::physical_pixel_ref, volume_image<T>>::by_location_view;
  };
  template <typename T>
  struct by_const_image_location_view : jhmi_detail::by_location_view<T, typename volume_image<T>::image_pixel, volume_image<T> const> {
    using jhmi_detail::by_location_view<T, typename volume_image<T>::image_pixel, volume_image<T> const>::by_location_view;
  };
  template <typename T>
  struct by_image_location_view : jhmi_detail::by_location_view<T, typename volume_image<T>::image_pixel_ref, volume_image<T>> {
    using jhmi_detail::by_location_view<T, typename volume_image<T>::image_pixel_ref, volume_image<T>>::by_location_view;
  };
  template <typename T>
  struct by_const_location_view : jhmi_detail::by_location_view<T, typename volume_image<T>::pixel, volume_image<T> const> {
    using jhmi_detail::by_location_view<T, typename volume_image<T>::pixel, volume_image<T> const>::by_location_view;
  };
  template <typename T>
  struct by_location_view : jhmi_detail::by_location_view<T, typename volume_image<T>::pixel_ref, volume_image<T>> {
    using jhmi_detail::by_location_view<T, typename volume_image<T>::pixel_ref, volume_image<T>>::by_location_view;
  };

  namespace view {
    struct by_physical_location_fn {
      template <typename T> auto operator()(volume_image<T> const& img) const {
        return by_const_physical_location_view<T>{img};
      }
      template <typename T> auto operator()(volume_image<T>& img) const {
        return by_physical_location_view<T>{img};
      }
    };
    template <typename T> auto operator|(T& img, by_physical_location_fn fn) {
      return fn(img);
    }
    struct by_image_location_fn {
      template <typename T> auto operator()(volume_image<T> const& img) const {
        return by_const_image_location_view<T>{img};
      }
      template <typename T> auto operator()(volume_image<T>& img) const {
        return by_image_location_view<T>{img};
      }
    };
    template <typename T> auto operator|(T& img, by_image_location_fn fn) {
      return fn(img);
    }
    struct by_location_fn {
      template <typename T> auto operator()(volume_image<T> const& img) const {
        return by_const_location_view<T>{img};
      }
      template <typename T> auto operator()(volume_image<T>& img) const {
        return by_location_view<T>{img};
      }
    };
    template <typename T> auto operator|(T& img, by_location_fn fn) {
      return fn(img);
    }
    constexpr by_image_location_fn by_image_location{};
    constexpr by_physical_location_fn by_physical_location{};
    constexpr by_location_fn by_location{};

    struct equal_step {
      m step_;
      m3 center_;
      explicit equal_step(m step, m3 const& center = m3{}) : step_{step}, center_{center} {}
      template <typename T> auto operator()(volume_image<T> const& img) const {
        return by_const_location_view<T>(img, dbl3{1,1,1}*step_, center_);
      }
      template <typename T> auto operator()(volume_image<T>& img) const {
        return by_location_view<T>(img, dbl3{1,1,1}*step_, center_);
      }
    };
    template <typename T> auto operator|(T& img, equal_step fn) {
      return fn(img);
    }

    struct x_slice {
      int x1, x2, y, z;
      x_slice(int x1, int x2, int y, int z) : x1(x1), x2(x2), y(y), z(z) {}

      template <typename T> auto operator()(volume_image<T> const& img) const {
        return ranges::make_iterator_range(&img(x1,y,z), &img(x2,y,z));
      }
      template <typename T> auto operator()(volume_image<T>& img) const {
        return ranges::make_iterator_range(&img(x1,y,z), &img(x2,y,z));
      }
    };
    template <typename T> auto operator|(T& img, x_slice fn) {
      return fn(img);
    }
  }

  template <typename T>
  auto gaussian_filter(volume_image<T>& in, double sigma, int num_dev = 3) {
    volume_image<double> out{in.dimensions(), extents(in)};
    volume_image<double> tmp{in.dimensions(), extents(in)};
    volume_image<double> tmp1{in.dimensions(), extents(in)};
    int range = static_cast<int>(std::ceil(num_dev * sigma));
    auto filter = ranges::view::closed_indices(-range, range)
      | ranges::view::transform([&](int x) {
           return /*1. / (sigma * 2.506628274) */ std::exp(-x*x/(2*sigma*sigma)); })
      | ranges::to_vector;

    auto ds = inflate(cube<int3>{int3{}, in.dimensions()}, -range*int3{1,1,1});
    traverse(ds, 1, [&](int3 const& pt) {
      for (int i = 0; i < filter.size(); ++i) {
        auto new_pt = pt + int3{i-range,0,0};
        tmp1(pt) += in(new_pt) * filter[i];
      }
    });
    traverse(ds, 1, [&](int3 const& pt) {
      for (int i = 0; i < filter.size(); ++i) {
        auto new_pt = pt + int3{0,i-range,0};
        tmp(pt) += tmp1(new_pt) * filter[i];
      }
    });
    traverse(ds, 1, [&](int3 const& pt) {
      for (int i = 0; i < filter.size(); ++i) {
        auto new_pt = pt + int3{0,0,i-range};
        out(pt) += tmp(new_pt) * filter[i];
      }
    });
    return out;
  }
  template <typename T, typename U> auto convolve(
      volume_image<T> const& lhs, volume_image<U> const& rhs) {
    auto half_size = int3(ceil(rhs.dimensions() / 2.));
    cube<int3> ds{int3{}, lhs.dimensions() - rhs.dimensions()};
    using Out = decltype(T{} * U{});
    volume_image<Out> out_img{lhs.dimensions(), extents(lhs)};
    //Reverse rhs once, simplify calculations later.
    auto kernel = rhs;
    traverse(cube<int3>{int3{}, rhs.dimensions()}, 1, [&](int3 const& pt) {
      kernel(rhs.dimensions() - int3{1,1,1} - pt) = rhs(pt);
    });
    traverse(ds, 1, [&](int3 const& ul) {
      Out out;
      traverse(cube<int3>{int3{}, kernel.dimensions()}, 1, [&](int3 const& pt) {
        out += lhs(ul + pt) * kernel(pt);
      });
      out_img(ul+half_size) = out;
    });
    return out_img;
  }
  namespace jhmi_detail {
    struct prev_type{}; static const constexpr prev_type prev{};
    struct next_type{}; static const constexpr next_type next{};
    struct Top {
      int operator()() const { return 0; }
      int operator()(prev_type) const { return 0; }
      int operator()(next_type) const { return 1; }
    };
    struct Mid {
      int v;
      explicit Mid(int v) : v(v) {}
      int operator()() const { return v; }
      int operator()(prev_type) const { return v-1; }
      int operator()(next_type) const { return v+1; }
    };
    struct Bottom {
      int v;
      explicit Bottom(int v) : v(v) {}
      int operator()() const { return v; }
      int operator()(prev_type) const { return v-1; }
      int operator()(next_type) const { return v; }
    };
    template <typename T, typename U1, typename U2, typename F>
    void traverse_x(F f, U1 y, U2 z, volume_image<T> const& in, volume_image<T>& out) {
      out(0, y(), z()) = f(in(0, y(), z()), in(1, y(), z()),
                           in(0, y(prev), z()), in(0, y(next), z()),
                           in(0, y(), z(prev)), in(0, y(), z(next)));
      auto l = in.width()-1;
      for (int x = 1; x < l; ++x)
        out(x, y(), z()) = f(in(x, y(), z()), in(x-1, y(), z()), in(x+1, y(), z()),
                             in(x, y(prev), z()), in(x, y(next), z()),
                             in(x, y(), z(prev)), in(x, y(), z(next)));
      out(l, y(), z()) = f(in(l, y(), z()), in(l-1, y(), z()),
                           in(l, y(prev), z()), in(l, y(next), z()),
                           in(l, y(), z(prev)), in(l, y(), z(next)));
    }
    template <typename T, typename U1, typename F>
    void traverse_y(F f, U1 z, volume_image<T> const& in, volume_image<T>& out) {
      traverse_x(f, Top(), z, in, out);
      for (int y = 1; y < in.height()-1; ++y)
        traverse_x(f, Mid(y), z, in, out);
      traverse_x(f, Bottom(in.height()-1), z, in, out);
    }
    template <typename T, typename F>
    void traverse_z(F f, volume_image<T> const& in, volume_image<T>& out) {
      traverse_y(f, Top(), in, out);
      for (int z = 1; z < in.depth()-1; ++z)
        traverse_y(f, Mid(z), in, out);
      traverse_y(f, Bottom(in.depth()-1), in, out);
    }
  }//jhmi_detail
  template <typename T> auto erode(volume_image<T> const& img) {
    auto out = volume_image<T>{img.dimensions(), extents(img)};
    jhmi_detail::traverse_z([](auto ...vals) { return std::min({vals...}); }, img, out);
    return out;
  }
  template <typename T> auto dilate(volume_image<T> const& img) {
    auto out = volume_image<T>{img.dimensions(), extents(img)};
    jhmi_detail::traverse_z([](auto ...vals) { return std::max({vals...}); }, img, out);
    return out;
  }
}

#endif
