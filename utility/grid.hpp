#ifndef GRID_HPP_NRC_20141031
#define GRID_HPP_NRC_20141031

#include "cube.hpp"
#include "pt3.hpp"
#include "units.hpp"
#include <boost/container/flat_set.hpp>
#include <boost/optional.hpp>
#include <range/v3/to_container.hpp>
#include <range/v3/view/map.hpp>
#include <algorithm>
#include <map>
#include <vector>

namespace jhmi
{

template <typename T>
class grid {
  using t_set = boost::container::flat_set<T>;
public:
  explicit grid(cube<m3> const& extents, m cell_size)
    : extents_(extents), cell_size_(cell_size),
      size_{ceil((extents.lr() - extents.ul()) / cell_size)},  grid_{} {
    grid_.resize(size_.x * size_.y * size_.z);
  }
  template <typename U>
  void remove_item(U&& item) {
    for_sets(item, [&](t_set& s) {
      s.erase(id(item));
    });
  }
  template <typename U>
  void add_item(U&& item) {
    for_sets(item, [&](t_set& s) {
      s.insert(id(item));
    });
  }
  t_set const& operator()(m3 const& pt) const {
    return at(pt_to_box(pt));
  }

  t_set cube_around(m3 const& pt) const {
    auto center = pt_to_box(pt);
    auto ext = intersect(cube<int3>{center - int3{1,1,1}, center + int3{2,2,2}},
                         cube<int3>{int3{0,0,0}, size_});
    t_set ret;
    traverse(ext, 1, [&](int3 const& upt) {
      auto& s = at(upt);
      ret.insert(boost::container::ordered_unique_range, s.begin(), s.end());
    });
    return ret;
  }

  t_set get_all() const {
    t_set ret;
    traverse(cube<int3>{int3{0,0,0}, size_}, 1, [&](int3 const& pt) {
      auto& s = at(pt);
      ret.insert(boost::container::ordered_unique_range, s.begin(), s.end());
    });
    return ret;
  }
private:
  typedef jhmi::pt3<int> int3;
  auto const& at(int3 const& upt) const {
    return grid_[upt.x + upt.y * size_.x + upt.z * size_.x * size_.y];
  }
  auto pt_to_box(m3 const& pt) const
  { return int3{floor((pt - extents_.ul()) / cell_size_)}; }

  template <typename U, typename F>
  auto for_sets(U const& item, F f) {
    bool contained = false;
    traverse(item, [&](m3 const& pt) { return pt_to_box(pt); }, [&](int3 const& pt) {
      if (contains(cube<int3>{int3{}, size_ - int3{1,1,1}}, pt)) {
        contained = true;
        f(grid_[pt.x + pt.y * size_.x + pt.z * size_.x * size_.y]);
      }
    });
    if (!contained) {
      auto e = extents(item);
      fmt::print("Item from\n{} to {} not overlapping grid from\n{} to {}\n",
        e.ul(), e.lr(), extents_.ul(), extents_.lr());
    }
  }

  friend cube<m3> const& extents(grid<T> const& g) {
    return g.extents_;
  }

  cube<m3> extents_;
  m cell_size_;
  int3 size_;
  std::vector<t_set> grid_;
};

}//jhmi

#endif
