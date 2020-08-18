#ifndef JHMI_GL_PROGRAM_HPP_NRC_20171106
#define JHMI_GL_PROGRAM_HPP_NRC_20171106

#include <GL/glew.h>
#include <unordered_map>

namespace jhmi { namespace gl {

  namespace gl_detail {
    template <unsigned N1, unsigned... Ns>
    struct or_equal : std::false_type {};
    template <unsigned N1, unsigned N2, unsigned... Ns>
    struct or_equal<N1, N2, Ns...> : std::integral_constant<bool, N1 == N2 || or_equal<N1, Ns...>{}> {};
  }

  template <unsigned N, unsigned... All>
  struct gl_type : std::integral_constant<unsigned, N> {
    static_assert(gl_detail::or_equal<N, All...>{}, "invalid value");
  };

  template <unsigned N>
  using buffer_type = gl_type<N, GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER>;
  static buffer_type<GL_ARRAY_BUFFER> const array_buffer{};
  static buffer_type<GL_ELEMENT_ARRAY_BUFFER> const element_buffer{};


  template <typename T> struct type_convert;
  template <> struct type_convert<float> { static unsigned const value = GL_FLOAT; };
  template <> struct type_convert<unsigned> { static unsigned const value = GL_UNSIGNED_INT; };

  struct partition_info {
    GLuint data_type, type_size, row_start, row_size, col_start, col_size, stride;
  };

  template <typename T>
  auto buffer_data(std::vector<T> const& t) { return static_cast<void const*>(t.data()); }
  template <typename T>
  GLuint buffer_size(std::vector<T> const& t) { return static_cast<GLuint>(t.size() * sizeof(T)); }
  template <typename T>
  GLuint buffer_datatype(std::vector<T> const&) { return type_convert<T>::value; }
  template <typename T>
  GLuint buffer_datasize(std::vector<T> const&) { return static_cast<GLuint>(sizeof(T)); }

  class vertex_array {
    GLuint vao_;
    struct buffer_info { GLuint id, data_type, type_size, buffer_size; };
    std::unordered_map<unsigned, buffer_info> buffers_;
  public:
    vertex_array() {
    }
    ~vertex_array() {
      for (auto& b : buffers_)
        glDeleteBuffers(1, &b.second.id);
      if (!buffers_.empty())
        glDeleteVertexArrays(1, &vao_);
    }
    vertex_array(vertex_array const&) = delete;
    vertex_array& operator=(vertex_array const&) = delete;
    vertex_array(vertex_array&&) = default;
    vertex_array& operator=(vertex_array&&) = default;

    bool has_indices() const {
      return buffers_.find(GL_ELEMENT_ARRAY_BUFFER) != buffers_.end();
    }
    template <unsigned N, typename T>
    void load_buffer(buffer_type<N>, T&& t) {
      if (buffers_.empty()) {
        glGenVertexArrays(1, &vao_);
        glBindVertexArray(vao_);
      }
      GLuint b;
      auto it = buffers_.find(N);
      if (it == buffers_.end())
        glGenBuffers(1, &b);
      else
        b = it->second.id;
      buffers_[N] = buffer_info{b, buffer_datatype(t), buffer_datasize(t), buffer_size(t)};
      glBindBuffer(N, b);
      glBufferData(N, buffer_size(t), buffer_data(t), GL_STATIC_DRAW);
    }
    void bind() const {
      glBindVertexArray(vao_);
    }
    auto partition(unsigned int col_start, unsigned int col_size, unsigned int stride) {
      auto& b = buffers_.at(array_buffer());
      assert(col_start + col_size <= stride);
      return partition_info{b.data_type, b.type_size, 0,
        b.buffer_size / b.type_size, col_start, col_size, stride};
    }
    auto partition(unsigned int row_start, unsigned int row_size,
         unsigned int col_start, unsigned int col_size, unsigned int stride) {
      auto& b = buffers_.at(array_buffer());
      assert(col_start + col_size <= stride);
      return partition_info{b.data_type, b.type_size, row_start, row_size,
         col_start, col_size, stride};
    }
  };

}}//end jhmi::gl
#endif
