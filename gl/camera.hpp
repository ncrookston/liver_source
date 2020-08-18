#ifndef JHMI_GL_CAMERA_HPP_NRC_20150928
#define JHMI_GL_CAMERA_HPP_NRC_20150928

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <fmt/ostream.h>
#include <algorithm>

namespace jhmi { namespace gl {
  class camera {
  public:
    glm::vec3 initial_eye_, initial_up_, eye_, locus_, up_;
    glm::uvec2 window_size_;
    glm::mat4 view_, proj_;
    float dist_, fov_, near_, far_;

  public:
    camera(glm::vec3 const& eye, glm::vec3 const& locus, glm::vec3 const& up,
           glm::uvec2 const& window_size, float fov_deg = 45.f,
           float near = .01f, float far = 20.f)
     : initial_eye_{eye}, initial_up_{up},
       eye_{eye}, locus_{locus}, up_{up}, window_size_{window_size},
       view_{glm::lookAt(eye, locus, up)},
       proj_{glm::perspective(glm::radians(fov_deg),
         float(window_size.x) / window_size.y, near, far)},
       dist_{glm::length(eye - locus)}, fov_{glm::radians(fov_deg)}, near_{near}, far_{far}
    {}

    auto const& view_matrix() const { return view_; }
    auto const& projection_matrix() const { return proj_; }
    auto convert_to_world(glm::vec3 const& win) const {
      return glm::unProject(win, view_, proj_, glm::vec4{0,0,window_size_});
    }
    auto window_size() const { return window_size_; }
    auto const& locus() const { return locus_; }
    auto const& eye() const { return eye_; }
    auto const& up() const { return up_; }

    void resize(glm::uvec2 const& window_size) {
      window_size_ = window_size;
      proj_ = glm::perspective(fov_, float(window_size.x) / window_size.y, near_, far_);
    }
    void rotate(double a) {
      glm::mat3 m;
      m[0][0] = std::cos(a); m[0][1] = std::sin(a); m[0][2] = 0;
      m[1][0] =-std::sin(a); m[1][1] = std::cos(a); m[1][2] = 0;
      m[2][0] = 0;           m[2][1] = 0;           m[2][2] = 1;
      rotate(m);
    }
    void rotate(glm::mat3 const& m) {
      eye_ = m * (eye_ - locus_) + locus_;
      up_ = {0,0,1};//m * (up_ - locus_) + locus_;
      view_ = glm::lookAt(eye_, locus_, up_);
    }
    void rotate_to(double a) {
      eye_ = initial_eye_;
      rotate(a);
      //eye_ = m * (eye_ - locus_) + locus_;
      //up_ = m * (up_ - locus_) + locus_;
      //view_ = glm::lookAt(eye_, locus_, up_);
    }
    void scale(float s) {
      assert(s > 0.f);
      dist_ *= s;
      eye_ = locus_ + dist_ * glm::normalize(eye_ - locus_);
      view_ = glm::lookAt(eye_, locus_, up_);
    }
    void reset_view() {
      eye_ = initial_eye_;
      up_ = initial_up_;
      view_ = glm::lookAt(eye_, locus_, up_);
      dist_ = glm::length(eye_ - locus_);
    }
  };
}}
#endif
