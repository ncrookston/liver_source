#ifndef JHMI_GL_TRACKBALL_HPP_NRC_20150930
#define JHMI_GL_TRACKBALL_HPP_NRC_20150930

#include <glm/glm.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/string_cast.hpp>

namespace jhmi { namespace gl {
  class trackball {
    camera& camera_;
    glm::uvec2 last_mouse_;
    float radius_;

    float z(glm::vec2 const& v) {
      auto d = glm::dot(v, v);
      if (d <= radius_*radius_ / 2)
        return std::sqrt(radius_*radius_ - d);
      return (radius_*radius_ / 2) / std::sqrt(d);
    }
    auto homogenize(glm::vec4 const& v) { return glm::vec3{v} / v.w; }
  public:
    explicit trackball(camera& c, glm::uvec2 const& last_mouse, float radius)
          : camera_{c},
            last_mouse_{last_mouse},
            radius_{radius}
    {}

    void operator()(glm::uvec2 const& next_mouse) {
      if (next_mouse == last_mouse_)
        return;

      auto om = glm::vec2{last_mouse_} - glm::vec2{camera_.window_size() / 2u};
      auto nm = glm::vec2{next_mouse} - glm::vec2{camera_.window_size() / 2u};
      auto v1 = normalize(glm::vec3{om, z(om)});
      auto v2 = normalize(glm::vec3{nm, z(nm)});
      last_mouse_ = next_mouse;

      auto theta = std::acos(glm::clamp(dot(v1,v2), -1.f, 1.f));
      auto o = camera_.convert_to_world(glm::vec3{0,0,0});
      auto p1 = camera_.convert_to_world(v1) - o;
      auto p2 = camera_.convert_to_world(v2) - o;
      auto n = glm::normalize(glm::cross(p2,p1));

      auto q = normalize(glm::vec4{n * std::sin(theta/2), std::cos(theta/2)});
      auto i = q.x;
      auto j = q.y;
      auto k = q.z;
      auto r = q.w;

      camera_.rotate(glm::mat3{
        1-2*(j*j+k*k), 2*(i*j-k*r),  2*(i*k+j*r),
        2*(i*j+k*r),   1-2*(i*i+k*k), 2*(j*k-i*r),
        2*(i*k-j*r),   2*(j*k+i*r),  1-2*(i*i+j*j)
      });
    }
  };
}}
#endif
