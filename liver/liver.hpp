#ifndef JHMI_LIVER_HPP_NRC_20141124
#define JHMI_LIVER_HPP_NRC_20141124

#include "liver/fill_liver_volume.hpp"
#include "utility/cube.hpp"
#include "utility/grid.hpp"
#include <boost/tokenizer.hpp>
#include <range/v3/view/const.hpp>
#include <fstream>

namespace jhmi {

class liver {
  using pt_list = std::vector<m3>;

  liver() : pt_grid_{nullptr}, extents_{} {}
public:
  template <typename Shape>
  static liver create(Shape const& volume) {
    liver lv;
    std::size_t idx = 0;
    lv.extents_ = extents(volume);
    fill_liver_volume(lv.extents_,
        [&](m3 const& pt) { return contains(volume, pt); },
        [&, last_x=std::numeric_limits<std::size_t>::max()]
        (m3 const& pt, int3 const& idxs) mutable
    {
      if (last_x != idxs.x)
      {
        lv.x_starts_.push_back(idx);
        last_x = idxs.x;
      }
      idx += 2;

      using namespace lobule;
      lv.portal_tracts_.push_back(pt + m3{0_mm, -side_length, 0_mm});
      lv.portal_tracts_.push_back(pt + m3{min_radius, -side_length / 2., 0_mm});

      lv.centrilobular_veins_.push_back(pt);
    });
    lv.x_starts_.push_back(idx);

    lv.pt_grid_.reset(new grid<pt_list>{lv.portal_tracts_,lobule::side_length});

    return lv;
  }
  static liver load(std::string const& filename) {
    liver lv;
    std::ifstream fin{filename};
    std::string line;
    std::getline(fin, line);//Ignore first line

    std::size_t idx = 0;
    std::string last_x;
    boost::optional<cube<m3>> oextents = boost::none;
    while (fin) {
      std::getline(fin, line);
      if (line.empty())
        continue;
      line = line.substr(1, line.size() - 2);
      boost::tokenizer<boost::char_separator<char>> tok{line, boost::char_separator<char>{","}};
      auto it = tok.begin();
      if (last_x != *it) {
        lv.x_starts_.push_back(idx);
        last_x = *it;
      }
      idx += 2;
      m x = std::stod(*it++) * meters;
      m y = std::stod(*it++) * meters;
      m z = std::stod(*it++) * meters;
      assert(it == tok.end());
      m3 pt{x,y,z};
      oextents = oextents ? expand(*oextents, pt) : cube<m3>{pt,pt};

      using namespace lobule;
      lv.portal_tracts_.push_back(pt + m3{0_mm, -side_length, 0_mm});
      lv.portal_tracts_.push_back(pt + m3{min_radius, -side_length / 2., 0_mm});
      lv.centrilobular_veins_.push_back(pt);
    }
    lv.pt_grid_.reset(new grid<pt_list>{lv.portal_tracts_,lobule::side_length});
    lv.extents_ = *oextents;

    return lv;
  }

  auto portal_tracts() const
  { return ranges::view::const_(portal_tracts_); }

  auto centrilobular_veins() const
  { return ranges::view::const_(centrilobular_veins_); }

  template <typename F>
  void visit_portal_tracts(std::size_t x_step, std::size_t y_step, F f) const
  {
    for (std::size_t i = 0, i_end = x_starts_.size()-1; i < i_end; i += x_step)
    {
      for (std::size_t j = x_starts_[i], j_end = x_starts_[i+1]; j < j_end; j += y_step)
      {
        f(j);
      }
    }
  }

  template <typename F>
  void for_lobules_in_radius(m3 const& pt, m radius, F f) const
  { pt_grid_->for_items_in_radius(pt, radius, f); }

private:
  std::vector<m3> portal_tracts_;
  std::vector<m3> centrilobular_veins_;
  std::vector<std::size_t> x_starts_;

  std::unique_ptr<grid<pt_list>> pt_grid_;
  cube<m3> extents_;
};

liver get_liver_cube(m size) {
  auto shape = cube<m3>{{0_mm,0_mm,0_mm},{size,size,size}};
  return liver::create(shape);
}

}//jhmi

#endif
