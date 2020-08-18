#ifndef JHMI_BINARY_TREE_HPP_NRC_2014_12_22
#define JHMI_BINARY_TREE_HPP_NRC_2014_12_22

#include <fmt/ostream.h>
#include "range/v3/view/view.hpp"
#include "range/v3/core.hpp"
#include "range/v3/view_facade.hpp"
#include <array>
#include <cassert>
#include <iostream>
#include <memory>
#include <queue>
#include <type_traits>

namespace jhmi {

  template <typename T>
  class binary_tree {
    struct node_impl {
      explicit node_impl(T const& t)
        : parent{nullptr}, lhs{nullptr}, rhs{nullptr}, value(t) {}

      node_impl* parent;
      std::unique_ptr<node_impl> lhs, rhs;
      T value;
    };

    struct maybe_node {
      maybe_node() : p{nullptr} {}
      explicit maybe_node(std::nullptr_t) : p{nullptr} {}
      explicit maybe_node(T t) : p{std::make_unique<node_impl>(t)}
      {}

      std::unique_ptr<node_impl> p;
    };

    template <std::size_t Row>
    struct arg_pack {
    private:
      typedef maybe_node arg;
      static std::size_t const N = 1 << Row;

      void init(std::size_t) {}
      template <typename ...Args>
      void init(std::size_t idx, arg&& a, Args&&... args)
      {
        nodes_[idx] = std::move(a.p);
        init(idx+1, std::forward<Args>(args)...);
      }

    public:
      template <typename ...Args>
      arg_pack(Args&&... args) : nodes_{}
      {
        static_assert(sizeof...(args) <= N,"Too many nodes for tree level");
        init(0, maybe_node{args}...);
      }

      std::array<std::unique_ptr<node_impl>, N> nodes_;
    };

    template <std::size_t Idx>
    static void populate(node_impl* parent, std::unique_ptr<node_impl>&) {}
    template <std::size_t Idx, std::size_t N, typename ...Ns>
    static void populate(node_impl* parent, std::unique_ptr<node_impl>& ptr, arg_pack<N>& n, Ns&... ns) {
      ptr = std::move(n.nodes_[Idx]);
      if (ptr)
      {
        populate<Idx*2>(ptr.get(), ptr->lhs, ns...);
        populate<Idx*2+1>(ptr.get(), ptr->rhs, ns...);
        ptr->parent = parent;
      }
    }
  public:
    class const_node;
    class node {
    public:
      using value_type = T;
      node() : node_{}, tree_{} {}
      node(node_impl* node, binary_tree<T>* tree) : node_(node), tree_(tree) {}

      bool is_left_child() const {
        return node_->parent && node_->parent->lhs.get() == node_;
      }
      bool is_right_child() const {
        return node_->parent && node_->parent->rhs.get() == node_;
      }

      template <typename U>
      node make_left_child_of(U&& t) {
        return make_child_of(std::forward<U>(t), &node_impl::lhs);
      }
      template <typename U>
      node make_right_child_of(U&& t) {
        return make_child_of(std::forward<U>(t), &node_impl::rhs);
      }
      template <typename U>
      node set_left_child(U&& t) {
        node_->lhs = std::make_unique<node_impl>(std::forward<U>(t));
        node_->lhs->parent = node_;
        return {node_->lhs.get(), tree_};
      }
      template <typename U>
      node set_right_child(U&& t) {
        node_->rhs = std::make_unique<node_impl>(std::forward<U>(t));
        node_->rhs->parent = node_;
        return {node_->rhs.get(), tree_};
      }
      node set_left_child(std::nullptr_t) {
        node_->lhs = nullptr;
        return {nullptr, tree_};
      }
      node set_right_child(std::nullptr_t) {
        node_->rhs = nullptr;
        return {nullptr, tree_};
      }

      void swap_children() {
        node_->lhs.swap(node_->rhs);
      }

      void replace_parent() {
        auto new_parent = node_->parent->parent;
        auto parent_access = new_parent->lhs.get() == node_->parent ?
          &node_impl::lhs : &node_impl::rhs;
        auto child_access = is_left_child() ? &node_impl::lhs : &node_impl::rhs;
        auto old_parent = std::move(new_parent->*parent_access);
        new_parent->*parent_access = std::move(old_parent.get()->*child_access);
        (new_parent->*parent_access)->parent = new_parent;
      }

      explicit operator bool() const { return node_; }
      auto& value() const { assert(node_); return node_->value; }
      node parent() const { return {node_->parent, tree_}; }
      node left_child() const { return {node_->lhs.get(), tree_}; }
      node right_child() const { return {node_->rhs.get(), tree_}; }
    private:
      friend class const_node;
      template <typename U>
      node make_child_of(U&& t, std::unique_ptr<node_impl> node_impl::*accessor) {
        assert(node_);
        auto old_parent = node_->parent;
        node_impl* new_parent = nullptr;
        if (is_left_child()) {
          auto this_node = std::move(old_parent->lhs);
          old_parent->lhs = std::make_unique<node_impl>(t);
          this_node->parent = old_parent->lhs.get();
          new_parent = old_parent->lhs.get();
          new_parent->*accessor = std::move(this_node);
        }
        else if (is_right_child()) {
          auto this_node = std::move(old_parent->rhs);
          old_parent->rhs = std::make_unique<node_impl>(t);
          this_node->parent = old_parent->rhs.get();
          new_parent = old_parent->rhs.get();
          new_parent->*accessor = std::move(this_node);
        }
        else {//this is the root node
          auto this_node = std::move(tree_->root_);
          tree_->root_ = std::make_unique<node_impl>(t);
          new_parent = tree_->root_.get();
          this_node->parent = new_parent;
          new_parent->*accessor = std::move(this_node);
        }
        new_parent->parent = old_parent;
        return {new_parent, tree_};
      }
      friend bool operator==(node const& lhs, node const& rhs)
      { return lhs.node_ == rhs.node_; }
      node_impl* node_;
      binary_tree<T>* tree_;
    };
    class const_node {
    public:
      using value_type = T const;
      const_node() : node_{} {}
      const_node(node_impl const* n) : node_(n) {}
      explicit const_node(node n) : node_(n.node_) {}

      explicit operator bool() const { return node_; }
      auto& value() const { assert(node_); return node_->value; }
      const_node parent() const { return {node_->parent}; }
      const_node left_child() const { return {node_->lhs.get()}; }
      const_node right_child() const { return {node_->rhs.get()}; }
    private:
      friend bool operator==(const_node const& lhs, const_node const& rhs)
      { return lhs.node_ == rhs.node_; }

      node_impl const* node_;
    };

    binary_tree() : root_(nullptr) {}

    explicit binary_tree(arg_pack<0> n0,
                         arg_pack<1> n1 = {},
                         arg_pack<2> n2 = {},
                         arg_pack<3> n3 = {})
      : root_(nullptr)
    {
      populate<0>(nullptr, root_, n0, n1, n2, n3);
    }

    bool empty() const { return root_ == nullptr; }

    node root() { return node(root_.get(), this); }
    const_node root() const { return const_node(root_.get()); }

  private:
    std::unique_ptr<node_impl> root_;
  };

  namespace jhmi_detail {
    template <bool b, typename T> struct binary_tree_type_impl
    { typedef jhmi::binary_tree<T> type; };
    template <typename T> struct binary_tree_type_impl<true, T>
    { typedef jhmi::binary_tree<T> const type; };
    template <typename T> struct binary_tree_type : binary_tree_type_impl<std::is_const<T>::value, std::remove_const_t<T>> {};
    template <typename T> using binary_tree_t = typename binary_tree_type<T>::type;

    template <bool b, typename T> struct node_type_impl
    { typedef typename jhmi::binary_tree<T>::node type; };
    template <typename T> struct node_type_impl<true, T>
    { typedef typename jhmi::binary_tree<T>::const_node type; };
    template <typename T> struct node_type : node_type_impl<std::is_const<T>::value, std::remove_const_t<T>> {};
    template <typename T> using node_t = typename node_type<T>::type;


    struct begin_tag {};
    struct end_tag {};
    template <typename T> struct order_cursor_base {
      jhmi_detail::node_t<T> node_;
      order_cursor_base() : node_{} {}
      order_cursor_base(jhmi_detail::node_t<T> node) : node_(node) {}
      bool equal(order_cursor_base const& other) const { return node_ == other.node_; }
    };
    template <typename T, bool Node>
    struct order_cursor : order_cursor_base<T> {
      using order_cursor_base<T>::order_cursor_base;
      auto& read() const { return this->node_.value(); }
    };
    template <typename T>
    struct order_cursor<T,true> : order_cursor_base<T> {
      using order_cursor_base<T>::order_cursor_base;
      auto& read() const { return this->node_; }
    };
    template <typename T, bool Node>
    struct in_order_cursor : order_cursor<T, Node> {
      in_order_cursor() = default;
      in_order_cursor(begin_tag, jhmi_detail::node_t<T> node) : order_cursor<T,Node>{node} {
        //Start at the bottom left.
        while (this->node_ && this->node_.left_child())
          this->node_ = this->node_.left_child();
      }
      in_order_cursor(end_tag, jhmi_detail::node_t<T> node) : order_cursor<T,Node>{node} {
        if (this->node_)
          this->node_ = this->node_.parent();
      }
      void next() {
        if (this->node_.right_child()) {
          this->node_ = this->node_.right_child();
          while (this->node_.left_child())
            this->node_ = this->node_.left_child();
          return;
        }

        auto old_node = this->node_;
        this->node_ = this->node_.parent();
        while (this->node_ && this->node_.right_child() == old_node) {
          old_node = this->node_;
          this->node_ = this->node_.parent();
        }
      }
    };
    template <typename T, bool Node>
    struct pre_order_cursor : order_cursor<T, Node> {
      pre_order_cursor() = default;
      pre_order_cursor(begin_tag, jhmi_detail::node_t<T> node) : order_cursor<T,Node>{node} {}
      pre_order_cursor(end_tag, jhmi_detail::node_t<T> node) : order_cursor<T,Node>{node} {
        if (this->node_)
          move_above_node();
      }
      void next() {
        if (this->node_.left_child()) {
          this->node_ = this->node_.left_child();
          return;
        }
        if (this->node_.right_child()) {
          this->node_ = this->node_.right_child();
          return;
        }
        move_above_node();
      }

      void move_above_node() {
        auto old_node = this->node_;
        this->node_ = this->node_.parent();
        while (this->node_ && (this->node_.right_child() == old_node || !this->node_.right_child()))
        {
          old_node = this->node_;
          this->node_ = this->node_.parent();
        }
        assert(!this->node_ || this->node_.right_child());
        if (this->node_ && this->node_.right_child())
          this->node_ = this->node_.right_child();
      }
    };
    template <typename T, bool Node>
    struct post_order_cursor : order_cursor<T, Node> {
      post_order_cursor() = default;
      post_order_cursor(begin_tag, jhmi_detail::node_t<T> node) : order_cursor<T,Node>{node} {
        if (!this->node_)
          return;
        //Start at the bottom-most, tending toward the left.
        while (this->node_.left_child() || this->node_.right_child()) {
          this->node_ = this->node_.left_child() ? this->node_.left_child()
                                                 : this->node_.right_child();
        }
      }
      post_order_cursor(end_tag, jhmi_detail::node_t<T> node) : order_cursor<T,Node>{node} {
        if (this->node_)
          next();
      }
      void next() {
        auto old_node = this->node_;
        this->node_ = this->node_.parent();
        if (this->node_ && this->node_.left_child() == old_node) {
          if (this->node_.right_child()) {
            this->node_ = this->node_.right_child();
            while (this->node_ && (this->node_.left_child() || this->node_.right_child())) {
              this->node_ = this->node_.left_child() ? this->node_.left_child()
                                                     : this->node_.right_child();
            }
          }
        }
      }
    };
    template <typename T, bool Node>
    struct level_order_cursor : order_cursor<T, Node> {
      std::queue<jhmi_detail::node_t<T>> children_;

      void push_node_children() {
        if (this->node_.left_child())
          children_.push(this->node_.left_child());
        if (this->node_.right_child())
          children_.push(this->node_.right_child());
      }

      level_order_cursor() = default;
      level_order_cursor(begin_tag, jhmi_detail::node_t<T> node) : order_cursor<T,Node>(node), children_() {
        if (!this->node_)
          return;

        push_node_children();
      }
      level_order_cursor(end_tag, jhmi_detail::node_t<T>) : order_cursor<T,Node>(), children_() {
        this->node_ = jhmi_detail::node_t<T>{}; }
      void next() {
        this->node_ = jhmi_detail::node_t<T>{};
        if (!children_.empty()) {
          this->node_ = children_.front();
          children_.pop();
          push_node_children();
        }
      }
      bool equal(level_order_cursor const& other) const {
        return this->node_ == other.node_ && children_ == other.children_;
      }
    };

    template <typename T, bool Node, template <typename, bool> class Cursor>
    struct order_view : ranges::view_facade<order_view<T,Node,Cursor>> {
      jhmi_detail::node_t<T> node_;
      Cursor<T,Node> begin_cursor() const { return {begin_tag{}, node_}; }
      Cursor<T,Node> end_cursor() const { return {end_tag{}, node_}; }

      explicit order_view(jhmi_detail::binary_tree_t<T>& tree) : node_(tree.root()) {}
      explicit order_view(jhmi_detail::node_t<T> node) : node_(node) {}
      order_view() = default;
    };
    template <template <typename> class View> struct order_fn
    {
      template <typename T> auto operator()(binary_tree<T> & tree) const { return View<T>{tree.root()}; }
      template <typename T> auto operator()(binary_tree<T> const& tree) const { return View<T const>{tree.root()}; }
      template <typename T> auto operator()(T node) const { return View<typename T::value_type>{node}; }
    };
  }//jhmi_detail

  template <typename T> using in_order_view
    = jhmi_detail::order_view<T, false, jhmi_detail::in_order_cursor>;
  template <typename T> using node_in_order_view
    = jhmi_detail::order_view<T, true, jhmi_detail::in_order_cursor>;
  template <typename T> using pre_order_view
    = jhmi_detail::order_view<T, false, jhmi_detail::pre_order_cursor>;
  template <typename T> using node_pre_order_view
    = jhmi_detail::order_view<T, true, jhmi_detail::pre_order_cursor>;
  template <typename T> using post_order_view
    = jhmi_detail::order_view<T, false, jhmi_detail::post_order_cursor>;
  template <typename T> using node_post_order_view
    = jhmi_detail::order_view<T, true, jhmi_detail::post_order_cursor>;
  template <typename T> using level_order_view
    = jhmi_detail::order_view<T, false, jhmi_detail::level_order_cursor>;
  template <typename T> using node_level_order_view
    = jhmi_detail::order_view<T, true, jhmi_detail::level_order_cursor>;

  namespace view {
    struct in_order_fn : jhmi_detail::order_fn<in_order_view> {};
    struct pre_order_fn : jhmi_detail::order_fn<pre_order_view> {};
    struct post_order_fn : jhmi_detail::order_fn<post_order_view> {};
    struct level_order_fn : jhmi_detail::order_fn<level_order_view> {};
    struct node_in_order_fn : jhmi_detail::order_fn<node_in_order_view> {};
    struct node_pre_order_fn : jhmi_detail::order_fn<node_pre_order_view> {};
    struct node_post_order_fn : jhmi_detail::order_fn<node_post_order_view> {};
    struct node_level_order_fn : jhmi_detail::order_fn<node_level_order_view> {};

    template <typename T> auto operator|(T&& tree,      in_order_fn fn) { return fn(tree); }
    template <typename T> auto operator|(T&& tree,      pre_order_fn fn) { return fn(tree); }
    template <typename T> auto operator|(T&& tree,      post_order_fn fn) { return fn(tree); }
    template <typename T> auto operator|(T&& tree,      level_order_fn fn) { return fn(tree); }
    template <typename T> auto operator|(T&& tree, node_in_order_fn fn) { return fn(tree); }
    template <typename T> auto operator|(T&& tree, node_pre_order_fn fn) { return fn(tree); }
    template <typename T> auto operator|(T&& tree, node_post_order_fn fn) { return fn(tree); }
    template <typename T> auto operator|(T&& tree, node_level_order_fn fn) { return fn(tree); }

    constexpr      in_order_fn           in_order{};
    constexpr      pre_order_fn          pre_order{};
    constexpr      post_order_fn         post_order{};
    constexpr      level_order_fn        level_order{};
    constexpr node_in_order_fn      node_in_order{};
    constexpr node_pre_order_fn     node_pre_order{};
    constexpr node_post_order_fn    node_post_order{};
    constexpr node_level_order_fn   node_level_order{};
  }
  template <typename T> using binary_node_t = typename binary_tree<T>::node;
  template <typename T> using binary_const_node_t = typename binary_tree<T>::const_node;
}

#endif
