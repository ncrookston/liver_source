#ifndef JHMI_UTILITY_OCTTREE_HPP_NRC_20160123
#define JHMI_UTILITY_OCTTREE_HPP_NRC_20160123

#include "utility/cube.hpp"
#include "utility/units.hpp"
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/optional.hpp>
#include <range/v3/to_container.hpp>
#include <range/v3/view.hpp>
#include <tbb/tbb.h>
#include <unordered_set>
#include <deque>

namespace jhmi {
  struct normal_distance_squared {
    template <typename T, typename U> auto operator()(T&& lhs, U&& rhs) {
      auto ds = distance_squared(std::forward<T>(lhs), std::forward<U>(rhs));
      return boost::optional<decltype(ds)>{ds};
    }
  };
  template <typename T>
  class octtree {
    using m_sq = decltype(1_mm * 1_mm);
    using container_t = boost::container::flat_set<T>;
    //using container_t = std::unordered_set<T>;
    struct node {
      friend class octtree;
      //Required since flat_set has an explicit default constructor?!
      node() : objs{}, sub_nodes{nullptr}, ext{} {}
      container_t objs;
      std::unique_ptr<node[]> sub_nodes;
      cube<m3> ext;
    };

    node node_;
    int preferred_;

    void split_node(node& n) {
      assert(!n.sub_nodes);
      n.sub_nodes.reset(new node[8]);
      auto ul = n.ext.ul(), nc = center(n.ext);
      m3 x{nc.x - ul.x, 0_mm, 0_mm};
      m3 y{0_mm, nc.y - ul.y, 0_mm};
      m3 z{0_mm, 0_mm, nc.z - ul.z};
      n.sub_nodes[0].ext = {ul, nc};
      n.sub_nodes[1].ext = {ul+x, nc+x};
      n.sub_nodes[2].ext = {ul+x+y, nc+x+y};
      n.sub_nodes[3].ext = {ul+y, nc+y};
      n.sub_nodes[4].ext = {ul+z, nc+z};
      n.sub_nodes[5].ext = {ul+z+x, nc+z+x};
      n.sub_nodes[6].ext = {nc, n.ext.lr()};
      n.sub_nodes[7].ext = {ul+z+y, nc+z+y};
    }
    static node* find_containing_sub_node(cube<m3> const& ext, node* sub_nodes) {
      if (!sub_nodes)
        return nullptr;
      for (std::int8_t idx = 0; idx < 8; ++idx) {
        if (contains(sub_nodes[idx].ext, ext))
          return &sub_nodes[idx];
      }
      return nullptr;
    }
    struct option_pt {
      boost::optional<std::pair<T,m_sq>> val;
      void consider(T const& t, m_sq d) {
        if (!val || d < val->second)
          val = std::make_pair(t, d);
      }
      bool valid() const { return bool(val); }
      auto extents(m3 const& pt) const {
        return inflate(cube<m3>{pt,pt}, dbl3{1,1,1} * m{sqrt(val->second)});
      }
      option_pt merge(option_pt rhs) const {
        if (val)
          rhs.consider(val->first, val->second);
        return rhs;
      }
    };
    struct best_vector_pts {
      boost::container::flat_map<m_sq, T> objs_;
      int n_;
      explicit best_vector_pts(int n) : n_(n) {}
      void consider(T const& t, m_sq d) {
        objs_.emplace(d,t);
        if (objs_.size() > n_)
          objs_.erase(objs_.begin() + n_, objs_.end());
      }
      bool valid() const { return !objs_.empty(); }
      auto extents(m3 const& pt) const {
        auto last = --objs_.end();
        auto maxext = last->first;
        return inflate(cube<m3>{pt,pt}, dbl3{1,1,1} * m{sqrt(maxext)});
      }
      best_vector_pts merge(best_vector_pts rhs) const {
        rhs.objs.insert(rhs.objs_.end(), objs_.begin(), objs_.end());
        if (rhs.objs_.size() > n_)
          objs_.erase(objs_.begin() + n_, objs_.end());
        return rhs;
      }
    };
    struct vector_pts {
      std::vector<T> objs;
      m_sq dsq;
      explicit vector_pts(m d) : dsq(d*d) {}
      void consider(T const& t, m_sq d) {
        if (d < dsq)
          objs.push_back(t);
      }
      bool valid() const { return true; }
      auto extents(m3 const& pt) const {
        return inflate(cube<m3>{pt,pt}, dbl3{1,1,1} * m{sqrt(dsq)});
      }
      vector_pts merge(vector_pts rhs) const {
        rhs.objs.insert(rhs.objs.end(), objs.begin(), objs.end());
        return rhs;
      }
    };
    template <typename F, typename ValHolder>
    static void find_at_level(m3 const& pt, node const& n, ValHolder& ov, F dist_sq) {
      for (auto&& obj : n.objs) {
        auto new_dist = dist_sq(obj, pt);
        if (new_dist)
          ov.consider(obj, *new_dist);
      }
      if (n.sub_nodes) {
        //We expect the containing node will have the closest match, do this
        // first so we might prune more ocatants in the future.
        auto c = find_containing_sub_node(cube<m3>{pt,pt}, n.sub_nodes.get());
        if (c)
          find_at_level(pt, *c, ov, dist_sq);
        for (int i = 0; i < 8; ++i)
          if (&n.sub_nodes[i] != c)
            if (!ov.valid() || overlaps(n.sub_nodes[i].ext, ov.extents(pt)))
              find_at_level(pt, n.sub_nodes[i], ov, dist_sq);
      }
    }
  public:
    explicit octtree(cube<m3> const& ext, int preferred = 16)
        : node_{}, preferred_{preferred} {
      node_.ext = ext;
    }

    void remove_item(T const& item) {
      auto ext = extents(item);
      auto node = &node_;
      for (auto c = &node_; c; c = find_containing_sub_node(ext, c->sub_nodes.get()))
        node = c;
      assert(node);
      auto it = node->objs.find(item);
      if (it == node->objs.end())
        throw std::runtime_error("removing non-existent octtree item");
      node->objs.erase(it);
    }

    void add_item(T const& item) {
      auto ext = extents(item);
      auto best = &node_;
      if (!contains(best->ext, ext))
        return;
//        throw std::runtime_error("Adding item outside octree bounds");
      while (best->sub_nodes) {
        auto new_best = find_containing_sub_node(ext, best->sub_nodes.get());
        if (new_best)
          best = new_best;
        else
          break;
      }
      best->objs.insert(item);
      if (!best->sub_nodes && best->objs.size() > preferred_) {
        //Split objs into the different octants, unless they overlap.
        split_node(*best);
        container_t ncont;
        for (auto&& repr : best->objs) {
          auto sub_node = find_containing_sub_node(extents(repr), best->sub_nodes.get());
          if (!sub_node)
            ncont.insert(repr);
          else
            sub_node->objs.insert(repr);
        }
        best->objs = std::move(ncont);
      }
    }

    template <typename F = normal_distance_squared>
    std::vector<T> find_nearest_items(m3 const& pt, m dist, F dist_sq = F{}) const {
      auto best = vector_pts{dist};
      find_at_level(pt, node_, best, dist_sq);
      return best.objs;
    }
    template <typename F = normal_distance_squared>
    std::vector<T> find_n_nearest_items(m3 const& pt, int n, F dist_sq = F{}) const {
      auto best = best_vector_pts{n};
      find_at_level(pt, node_, best, dist_sq);
      return best.objs_ | ranges::view::values | ranges::to_vector;
    }

    template <typename F = normal_distance_squared>
    boost::optional<T> find_nearest_item(m3 const& pt, F dist_sq = F{}) const {
      auto best = option_pt{};
      find_at_level(pt, node_, best, dist_sq);
      if (best.valid())
        return best.val->first;
      return boost::none;
    }

    auto get_all() const {
      boost::container::flat_set<decltype(id(std::declval<T>()))> ret;
      std::deque<node const*> nodes{&node_};
      while (!nodes.empty()) {
        auto curr = nodes.front();
        nodes.pop_front();
        for (auto&& repr : curr->objs) {
          ret.insert(id(repr));
        }
        if (curr->sub_nodes) {
          for (int i = 0; i < 8; ++i)
            nodes.insert(nodes.end(), &curr->sub_nodes[i]);
        }
      }
      return ret;
    }
//TODO: Debug only -- non-transitive const problems
    node const& root() const {
      return node_;
    }
  };

  template <typename T>
  cube<m3> extents(octtree<T> const& o) { return o.root().ext; }
}
#endif
