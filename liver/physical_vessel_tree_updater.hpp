#ifndef JHMI_LIVER_PHYSICAL_VESSEL_TREE_UPDATER_HPP_NRC_20170228
#define JHMI_LIVER_PHYSICAL_VESSEL_TREE_UPDATER_HPP_NRC_20170228

#include "liver/physical_vessel.hpp"
#include "utility/binary_tree.hpp"

namespace jhmi {
  class physical_vessel_tree_updater {
    using tree_t = binary_tree<physical_vessel>;
    using node_t = binary_node_t<physical_vessel>;
    tree_t& tree_;
    double gamma_;
    std::vector<double> scalars_;
    Pa cell_pressure_;
    cubic_meters_per_second cell_flow_;
    std::mt19937& gen_;

    void update_pressures(physical_vessel& v, node_t ln, node_t rn) {
      auto& l = ln.value();
      auto& r = rn.value();
      auto rscalar = get_scalar(rn, l.entry_pressure_);

      v.radius_ = std::pow(std::pow(l.radius_.value(), gamma_) + std::pow(r.radius_.value() * rscalar, gamma_), 1/gamma_) * meters;
      v.flow_ = l.flow_ + r.flow_;
      v.exit_pressure_ = l.entry_pressure_;
      v.entry_pressure_ = v.exit_pressure_ + v.delta_pressure();

      scalars_[r.id().value()] = rscalar;
    }

    void update_vessel(node_t n) {
      auto& v = n.value();
      auto idx = v.id().value();
      if (scalars_.size() <= idx)
        scalars_.resize(idx + 1, 1.);
      auto ln = n.left_child(), rn = n.right_child();
      if (ln && rn) {
        if (ln.value().entry_pressure() > rn.value().entry_pressure())
          update_pressures(v, ln, rn);
        else
          update_pressures(v, rn, ln);
      }
      else if (ln || rn) {
        auto& c = ln ? ln.value() : rn.value();
        v.radius_ = c.radius_;
        v.flow_ = c.flow_;
        v.exit_pressure_ = c.entry_pressure_;
        v.entry_pressure_ = v.exit_pressure_ + v.delta_pressure();
      }
      else {
        //We're at a terminal vessel.  Sample radius.
        //std::normal_distribution<> dist{11.8, 4.7};
        //auto sample = [&] { return std::min(56.6, std::max(4.7, dist(gen_))) / 2 * micrometers; };
        v.radius_ = 11.8 * micrometers;//sample();
        v.flow_ = cell_flow_;
        v.exit_pressure_ = cell_pressure_;
        v.entry_pressure_ = v.exit_pressure_ + v.delta_pressure();
      }
    }

    void update_vessel_recursive(node_t n) {
      if (!n)
        return;
      update_vessel(n);
      update_vessel_recursive(n.parent());
    }
    double get_scalar(node_t n, Pa desired_pressure) {
      return boost::units::pow<boost::units::static_rational<1,4>>(
          (n.value().entry_pressure_ - cell_pressure_)  / (desired_pressure - cell_pressure_));
    }
    void normalize_pressure(node_t start_node, Pa start_pressure) {
      auto items = std::deque<std::tuple<node_t, Pa, double>>{
        std::make_tuple(start_node, start_pressure, get_scalar(start_node, start_pressure))};
      while (!items.empty()) {
        auto [n, entry_pressure, prev_scalar] = items.front();
        items.pop_front();
        auto& v = n.value();
        auto idx = v.id().value();
        auto curr_scalar = prev_scalar * scalars_[idx];
        v.radius_ *= curr_scalar;
        v.entry_pressure_ = entry_pressure;
        v.exit_pressure_ = v.entry_pressure_ - v.delta_pressure();
        if (n.left_child())
          items.push_back(std::make_tuple(n.left_child(), v.exit_pressure_, curr_scalar));
        if (n.right_child())
          items.push_back(std::make_tuple(n.right_child(), v.exit_pressure_, curr_scalar));
      }
      ranges::fill(scalars_, 1.);
    }

  public:
    explicit physical_vessel_tree_updater(tree_t& tree, double gamma, Pa cell_pressure, cubic_meters_per_second cell_flow, std::mt19937& gen)
      : tree_{tree}, gamma_{gamma}, cell_pressure_{cell_pressure}, cell_flow_{cell_flow}, gen_{gen}
    {}

    void normalize_all() {
      RANGES_FOR(auto& n, tree_.root() | view::node_post_order) {
        update_vessel(n);
      }
      normalize_pressure(tree_.root(), input_pressure);
    }
  };
}//jhmi

#endif

