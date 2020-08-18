#ifndef JHMI_GL_VISUALIZER_HPP_NRC_20150924
#define JHMI_GL_VISUALIZER_HPP_NRC_20150924

#include "gl/camera.hpp"
#include "gl/program.hpp"
#include "gl/trackball.hpp"
#include "liver/macrocell_tree.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Window.hpp>
#include <fstream>
#include <streambuf>

namespace jhmi { namespace gl {
  class gui {
    std::vector<std::unique_ptr<program>> prog_;
    camera c_;
    std::unique_ptr<sf::Window> window_;
    boost::optional<gl::trackball> trackball_;
    int save_idx_ = 0;
    boost::optional<std::chrono::steady_clock::time_point> rot_start_;
  public:
    enum action { none, request_close };
    gui(int width, int height, camera const& c) : c_{c} {
      // create the window
      sf::ContextSettings settings;
      settings.depthBits = 24;
      settings.stencilBits = 8;
      settings.majorVersion = 3;
      settings.minorVersion = 3;
      settings.antialiasingLevel = 4;
      window_.reset(new sf::Window(sf::VideoMode(width, height), "Liver Model", sf::Style::Default,settings));
      window_->setVerticalSyncEnabled(true);

      glewExperimental = true;
      if (glewInit() != GLEW_OK)
        throw std::runtime_error("Cannot setup OpenGL (GLEW)");
    }

    gui& operator=(gui const&) = delete;
    gui(gui const&) = delete;
    gui& operator=(gui&&) = default;
    gui(gui&&) = default;

    sf::Image save_screen() {
      auto size = window_->getSize();
      draw();
      std::vector<std::uint8_t> data(size.x * size.y * 4, 0);
      glReadPixels(0, 0, size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
      sf::Image img;
      img.create(size.x, size.y, &data[0]);
      img.flipVertically();
      return img;
    }

    void load_program(std::unique_ptr<program>&& prog) {
      prog->bind_input("view", c_.view_matrix());
      prog->bind_input("proj", c_.projection_matrix());
      prog_.push_back(std::move(prog));
    }

    void draw() {
      if (rot_start_) {
        auto elapsed = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - *rot_start_).count() / 5;
        if (elapsed > 1) {
          elapsed = 1;
          rot_start_ = boost::none;
        }
        c_.rotate_to(-2 * 3.141592659 * elapsed);
      }
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      for (auto&& p : prog_)
        p->draw();
      // end the current frame (internally swaps the front and back buffers)
      window_->display();
    }

    action process_events() {
      // run the main loop
      auto ret = none;

      // handle events
      sf::Event event;
      while (window_->pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
          // end the program
          ret = request_close;
        }
        else if (event.type == sf::Event::Resized) {
          // adjust the viewport when the window is resized
          c_.resize(glm::uvec2{event.size.width, event.size.height});
          for (auto&& p : prog_)
            p->bind_input("proj", c_.projection_matrix());
          glViewport(0, 0, event.size.width, event.size.height);
        }
        else if (event.type == sf::Event::MouseButtonPressed) {
          auto btn = event.mouseButton;
          if (btn.button == sf::Mouse::Left) {
            trackball_.emplace(c_, glm::uvec2{btn.x, window_->getSize().y - btn.y}, 300);
          }
        }
        else if (event.type == sf::Event::MouseMoved && trackball_) {
          (*trackball_)(glm::uvec2{event.mouseMove.x, window_->getSize().y - event.mouseMove.y});
        }
        else if (event.type == sf::Event::MouseButtonReleased) {
          if (event.mouseButton.button == sf::Mouse::Left) {
            trackball_ = boost::none;
          }
        }
        else if (event.type == sf::Event::MouseWheelMoved) {
          c_.scale(std::pow(.90, event.mouseWheel.delta));
        }
        else if (event.type == sf::Event::KeyReleased) {
          if (event.key.code == sf::Keyboard::Escape) {
            rot_start_ = boost::none;
            c_.reset_view();
          }
          else if (event.key.code == sf::Keyboard::Left)
            c_.rotate(pi/6);
          else if (event.key.code == sf::Keyboard::Right)
            c_.rotate(-pi/6);
          else if (event.key.code == sf::Keyboard::Equal
             || event.key.code == sf::Keyboard::Add)
            c_.scale(.90);
          else if (event.key.code == sf::Keyboard::Num0)
            c_.scale(1/.90);
          else if (event.key.code == sf::Keyboard::P) {
            auto eye = c_.eye();
            auto up = c_.up();
            fmt::print("Loc: {} {} {}\n", eye[0], eye[1], eye[2]);
            fmt::print("Up: {} {} {}\n", up[0], up[1], up[2]);
          }
          else if (event.key.code == sf::Keyboard::S) {
            auto im = save_screen();
            im.saveToFile(fmt::format("tree_capture{:02}.png", save_idx_++));
          }
          else if (event.key.code == sf::Keyboard::G) {
            rot_start_ = std::chrono::steady_clock::now();
          }
          else
            fmt::print("Unknown code: {}\n", event.key.code);
        }
        else if (event.type == sf::Event::MouseLeft) {
          trackball_ = boost::none;
        }
      }
      for (auto&& p : prog_)
        p->bind_input("view", c_.view_matrix());
      return ret;
    }
  };
  std::string gulp(std::string const& file) {
    std::ifstream in{file};
    if (!in)
      throw std::runtime_error(fmt::format("Invalid file {} supplied", file));
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
  }
  auto load_cell_buffers(macrocell_tree const& tree) {
    auto data = std::vector<float>{};

    for (auto const& c : tree.macrocells().list()) {
      data.push_back(c.center.x.value());
      data.push_back(c.center.y.value());
      data.push_back(c.center.z.value());
      data.push_back(c.radius.value());
    }

    auto buffer = std::make_unique<vertex_array>();
    buffer->load_buffer(gl::array_buffer, data);
    return buffer;
  }
  auto load_tree_buffers(macrocell_tree const& tree) {
    auto get_parent = [](auto node) {
      auto parent = node.parent();
      while (parent && distance(node.value().end() - parent.value().end()) < .1_um)
        parent = parent.parent();
      return parent;
    };
    auto data = std::vector<float>{};
    auto indices = std::vector<unsigned>{};
    auto to_idx = std::unordered_map<vessel_id, int>{};
    auto vessels = tree.vessel_tree().vessel_nodes();
    auto& fv = vessels.begin()->value();
    to_idx[fv.id()] = 0;
    data.push_back(fv.start().x.value());
    data.push_back(fv.start().y.value());
    data.push_back(fv.start().z.value());
    data.push_back(fv.radius().value());
    data.push_back(fv.strahler_order);
    to_idx[fv.id()] = 1;
    auto mid = .5 * fv.start() + .5 * fv.end();
    data.push_back(mid.x.value());
    data.push_back(mid.y.value());
    data.push_back(mid.z.value());
    data.push_back(fv.radius().value());
    data.push_back(fv.strahler_order);
    int idx = 2;

    for (auto const& v : vessels) {
      auto curr_idx = idx++;
      to_idx[v.value().id()] = curr_idx;

      data.push_back(v.value().end().x.value());
      data.push_back(v.value().end().y.value());
      data.push_back(v.value().end().z.value());
      data.push_back(v.value().radius().value());
      data.push_back(v.value().strahler_order);

      auto vmid = get_parent(v);

      if (vmid) {
        auto vtop = get_parent(vmid);

        indices.push_back(vtop ? to_idx[vtop.value().id()] : 0);
        indices.push_back(to_idx[vmid.value().id()]);
        indices.push_back(curr_idx);
      }
      else {
        indices.push_back(0);
        indices.push_back(1);
        indices.push_back(curr_idx);
      }
    }

    auto buffer = std::make_unique<vertex_array>();
    buffer->load_buffer(gl::array_buffer, data);
    buffer->load_buffer(gl::element_buffer, indices);
    return buffer;
  }
  gui make_gui(macrocell_tree const& tree, bool show_macrocells,
               int width = 800, int height = 600, flt3 const& ctr = flt3{}) {
    auto ctr_loc = glm::vec3{ctr.x, ctr.y, ctr.z};
    gui g{width, height, camera{ctr_loc - glm::vec3{0,.35,0}, ctr_loc, glm::vec3{0,0,1}, glm::uvec2{width, height}}};

    g.load_program(std::make_unique<program>(true, triangles,
          [&] { return load_tree_buffers(tree); },
          vertex_shader, gulp("../gl/vessel.vs"),
          geometry_shader, gulp("../gl/vessel.gs"),
          fragment_shader, gulp("../gl/vessel.fs"),
          "loc", 0, 3, 5,
          "radius", 3, 1, 5,
          "idx", 4, 1, 5));
    if (show_macrocells) {
      g.load_program(std::make_unique<program>(true, points,
            [&] { return load_cell_buffers(tree); },
            vertex_shader, gulp("../gl/macrocell.vs"),
            geometry_shader, gulp("../gl/macrocell.gs"),
            fragment_shader, gulp("../gl/macrocell.fs"),
            "loc", 0, 3, 4,
            "radius", 3, 1, 4));
    }
    glClearColor(1.f, 1.f, 1.f, 1.f);
    //glClearColor(143/255.f,191/255.f,255/255.f,1.f);
    //glClearColor(213/255.f,231/255.f,255/255.f,1.f);
    //glClearColor(120/255.f,192/255.f,168/255.f,1.f);
    //glClearColor(0.6f,0.6f,0.6f,1.f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return g;
  }
}}//jhmi::gl
#endif
