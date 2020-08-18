
#include "messages/sphere_locs.pb.h"
#include "shape/voxelized_shape.hpp"
#include "utility/load_protobuf.hpp"
#include "utility/options.hpp"

using namespace jhmi;

struct shape_sampler {
  std::mt19937& eng_;
  voxelized_shape const& shape_;
  m3 dims_;
  std::uniform_real_distribution<> d_;

  shape_sampler(std::mt19937& eng, voxelized_shape const& shape)
    : eng_{eng}, shape_{shape}, dims_{dimensions(extents(shape))} {}

  m3 operator()() {
    while (true) {
      auto pt = extents(shape_).ul() + element_multiply(dims_, dbl3{d_(eng_), d_(eng_), d_(eng_)});
      if (shape_(pt))
        return pt;
    }
  }
};

class dbl3x3 {
  double elem_[9];
public:
  //Row major specification
  dbl3x3(double a, double b, double c, double d, double e, double f, double g, double h, double i) : elem_{a,b,c,d,e,f,g,h,i} {}

  template <typename U>
  auto operator*(pt3<U> const& pt) const {
    return pt3<U>{elem_[0] * pt.x + elem_[1] * pt.y + elem_[2] * pt.z,
                  elem_[3] * pt.x + elem_[4] * pt.y + elem_[5] * pt.z,
                  elem_[6] * pt.x + elem_[7] * pt.y + elem_[8] * pt.z};
  }
};

class cuboid {
  m3 p1_, p2_, p3_, p4_;
  friend cube<m3> extents(cuboid const& c) {
    return expand(expand(cube<m3>{c.p1_, c.p2_}, c.p3_), c.p4_);
  }
  /* untested
  friend bool contains(cuboid const& c, m3 const& x) {
    auto u = c.p1_ - c.p2_;
    auto v = c.p1_ - c.p3_;
    auto w = c.p1_ - c.p4_;
    if (dot(u,x) < dot(u,c.p1_) || dot(u,x) > dot(u,c.p2_))
      return false;
    if (dot(v,x) < dot(v,c.p1_) || dot(v,x) > dot(u,c.p3_))
      return false;
    if (dot(w,x) < dot(w,c.p1_) || dot(w,x) > dot(u,c.p4_))
      return false;
    return true;
  }
  */
  friend std::ostream& operator<<(std::ostream& out, cuboid const& c) {
    out << fmt::format("({}, {}, {}, {})", c.p1_, c.p2_, c.p3_, c.p4_);
    return out;
  }
public:
  cuboid(m3 const& p1, m3 const& p2, m3 const& p3, m3 const& p4) : p1_{p1}, p2_{p2}, p3_{p3}, p4_{p4} {}

};

dbl3x3 random_rotation(std::mt19937& eng) {
  //generate quaternion, then convert it to a rotation matrix m:
  auto dist = std::normal_distribution<>{};
  auto a = dist(eng);
  auto b = dist(eng);
  auto c = dist(eng);
  auto d = dist(eng);
  auto dst = std::sqrt(a*a + b*b + c*c + d*d);
  a /= dst;
  b /= dst;
  c /= dst;
  d /= dst;

  return dbl3x3{a*a+b*b-c*c-d*d, 2*b*c-2*a*d, 2*b*d+2*a*c,
                2*b*c+2*a*d, a*a-b*b+c*c-d*d, 2*c*d-2*a*b,
                2*b*d-2*a*c, 2*c*d+2*a*b, a*a-b*b-c*c+d*d};
}

int main(int argc, char* argv[]) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  auto opts = options<output_path_option, spherefile_option, shapefile_option, seed_option>{};
  if (!opts.parse(argc, argv))
    return 1;

  auto liver = voxelized_shape{opts.shapefile()};
  auto spheres = load_protobuf<jhmi_message::SphereLocs>(opts.spherefile());

  //Sample a random location and orientation in the shape.
  std::mt19937 eng{opts.seed()};
  //Samples a random location within the liver.
  auto center = shape_sampler{eng, liver}();
  auto rot = random_rotation(eng);

  //Go through the list of spheres, determine which spheres are inside the area
  auto dims = m3{4000_um, 16800_um, 9100_um};
  auto exts = cube<m3>{m3{}, dims};
  auto out = std::ofstream{(opts.output_path() / fmt::format("sampled_spheres_{}.csv", opts.seed())).string()};
  int num_spheres = 0;
  for (auto&& l : spheres.locations()) {
    auto pt = m3{rot * (dbl3{l.x(), l.y(), l.z()} * meters - center)};
    if (contains(exts, pt)) {
    //if (contains(sample_ext, pt) && contains(sample_area, pt)) {
      ++num_spheres;
      out << fmt::format("{:3.10f},{:3.10f},{:3.10f}\n",
          1e3*l.x(),1e3*l.y(), 1e3*l.z());
    }
  }
  fmt::print("Selected {} of {} potential spheres\n", num_spheres, spheres.locations().size());
}

