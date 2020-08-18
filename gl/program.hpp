#ifndef JHMI_GL_PROGRAM_HPP_NRC_20150924
#define JHMI_GL_PROGRAM_HPP_NRC_20150924

#include "gl/vertex_array.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fmt/ostream.h>
#include <vector>
#include <cassert>

namespace jhmi { namespace gl {

  template <unsigned N>
  using shader_type = gl_type<N, GL_FRAGMENT_SHADER, GL_VERTEX_SHADER, GL_GEOMETRY_SHADER>;

  static shader_type<GL_FRAGMENT_SHADER> const fragment_shader{};
  static shader_type<GL_VERTEX_SHADER> const vertex_shader{};
  static shader_type<GL_GEOMETRY_SHADER> const geometry_shader{};

  template <unsigned N>
  using primitive_type = gl_type<N, GL_POINTS, GL_LINES, GL_LINE_STRIP,
                                    GL_LINE_LOOP, GL_TRIANGLES,
                                    GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN,
                                    GL_QUADS, GL_QUAD_STRIP, GL_POLYGON>;

  static primitive_type<GL_POINTS> const points{};
  static primitive_type<GL_LINES> const lines{};
  static primitive_type<GL_LINE_STRIP> const line_strip{};
  static primitive_type<GL_LINE_LOOP> const line_loop{};
  static primitive_type<GL_TRIANGLES> const triangles{};
  static primitive_type<GL_TRIANGLE_STRIP> const triangle_strip{};
  static primitive_type<GL_TRIANGLE_FAN> const triangle_fan{};
  static primitive_type<GL_QUADS> const quads{};
  static primitive_type<GL_QUAD_STRIP> const quad_strip{};
  static primitive_type<GL_POLYGON> const polygon{};

  class program {
    std::vector<GLuint> shaders_;
    std::unique_ptr<vertex_array> buffer_;
    GLuint program_, primitive_type_, row_start_, row_size_;
    bool use_depth_test_;

    void bind_program() {}
    template <typename... Ts>
    void bind_program(std::string const& name, int col_start, int col_size,
          int stride, Ts&&... rest) {
      bind_input(name.c_str(), buffer_->partition(col_start, col_size, stride));
      bind_program(std::forward<Ts>(rest)...);
    }
    template <typename... Ts>
    void bind_program(std::unique_ptr<vertex_array>&& buffer, Ts&&... rest) {
      buffer_ = std::move(buffer);
      bind_program(std::forward<Ts>(rest)...);
    }
    template <unsigned N, typename... Ts>
    void bind_program(shader_type<N>, std::string const&, Ts&&... rest) {
      bind_program(std::forward<Ts>(rest)...);
    }

    void load_program() {}
    template <typename... Ts>
    void load_program(std::string const&, int, int, int, Ts&&... rest) {
      load_program(std::forward<Ts>(rest)...);
    }
    template <typename... Ts>
    void load_program(std::unique_ptr<vertex_array>&&, Ts&&... rest) {
      load_program(std::forward<Ts>(rest)...);
    }
    template <unsigned N, typename... Ts>
    void load_program(shader_type<N> t, std::string const& code, Ts&&... rest) {
      char const* code_ptr = code.c_str();
      shaders_.push_back(glCreateShader(N));
      glShaderSource(shaders_.back(), 1, &code_ptr, nullptr);
      glCompileShader(shaders_.back());
      int is_compiled = GL_TRUE;
      glGetShaderiv(shaders_.back(), GL_COMPILE_STATUS, &is_compiled);
      if (is_compiled == GL_FALSE) {
        int length = 0;
        glGetShaderiv(shaders_.back(), GL_INFO_LOG_LENGTH, &length);
        std::unique_ptr<char[]> msg_buffer{new char[length]};
        glGetShaderInfoLog(shaders_.back(), length, &length, msg_buffer.get());
        throw std::logic_error(fmt::format("Invalid shader {}", msg_buffer.get()));
      }
      load_program(std::forward<Ts>(rest)...);
    }

  public:
    template <unsigned N, typename T, typename... Ts>
    program(bool use_depth_test, primitive_type<N>, T make_buffer, Ts&&... rest)
        : shaders_{}, program_{0}, primitive_type_{N}, row_start_{0}, row_size_{0}, use_depth_test_{use_depth_test} {
      load_program(std::forward<Ts>(rest)...);
      // Link the vertex and fragment shader into a shader program
      program_ = glCreateProgram();
      for (auto s : shaders_)
        glAttachShader(program_, s);
      glLinkProgram(program_);
      int is_linked = GL_TRUE;
      glGetProgramiv(program_, GL_LINK_STATUS, &is_linked);
      if (is_linked == GL_FALSE) {
        int length = 0;
        glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &length);
        std::unique_ptr<char[]> msg_buffer{new char[length]};
        glGetProgramInfoLog(program_, length, &length, msg_buffer.get());
        throw std::logic_error(fmt::format("Invalid program {}", msg_buffer.get()));
      }
      glUseProgram(program_);
      buffer_ = make_buffer();
      bind_program(std::forward<Ts>(rest)...);
    }
    ~program() {
      glDeleteProgram(program_);
      for (auto s : shaders_)
        glDeleteShader(s);
    }
    program(program const&) = delete;
    program& operator=(program const&) = delete;

    program(program&&) = default;
    program& operator=(program&&) = default;

    void bind_input(char const* bind_point, partition_info const& p) {
      // Specify the layout of the vertex data
      glUseProgram(program_);
      GLint a = glGetAttribLocation(program_, bind_point);
      glEnableVertexAttribArray(a);
      glVertexAttribPointer(a, p.col_size, p.data_type, GL_FALSE,
        p.stride * p.type_size,
        static_cast<char*>(nullptr) + p.col_start * p.type_size);
      row_start_ = p.row_start;
      row_size_ = p.row_size;
    }
    void bind_input(char const* bind_point, glm::mat4 const& mat) {
      // Specify the layout of the vertex data
      glUseProgram(program_);
      GLint u = glGetUniformLocation(program_, bind_point);
      glProgramUniformMatrix4fv(program_, u, 1, GL_FALSE, glm::value_ptr(mat));
    }

    void draw() const {
      glUseProgram(program_);
      buffer_->bind();
      if (!use_depth_test_)
        glDisable(GL_DEPTH_TEST);
      if (buffer_->has_indices())
        glDrawElements(primitive_type_, row_size_, GL_UNSIGNED_INT, nullptr);
      else
        glDrawArrays(primitive_type_, row_start_, row_size_);
      if (!use_depth_test_)
        glEnable(GL_DEPTH_TEST);
    }
  };
}}//jhmi::gl

#endif
