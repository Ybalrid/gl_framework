#pragma once

#include <memory>

#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"

#include "transform.hpp"

namespace bullet_utils
{
  inline glm::vec3 convert(btVector3 v) { return glm::vec3(v.x(), v.y(), v.z());}
  inline glm::quat convert(btQuaternion q) { return glm::quat(q.x(), q.y(), q.z(), q.w()); };
  inline btVector3 convert(glm::vec3 v) { return btVector3(v.x, v.y, v.z); }
  inline btQuaternion convert(glm::quat q) { return btQuaternion(q.x, q.y, q.z, q.w); }

  inline transform convert(btTransform t)
  {
    const auto rotation = t.getRotation();
    const auto origin   = t.getOrigin();

    transform xform;
    xform.set_position(convert(origin));
    xform.set_orientation(convert(rotation));
    xform.set_scale(glm::vec3(1.f));
    return xform;
  }
}


class physics_system
{

public:

  static constexpr glm::vec3 earth_average_gravitational_acceleration_field = glm::vec3(0.f, -9.80665f, 0.f);


  struct
  {
    int substep_per_frame = 10;
    float substep_simulation_fps = 250;
  } simulation_configuration;

  physics_system()
  {
    std::cout << "Initializing physics system with Bullet Physics version " << btGetVersion() << " : " << btIsDoublePrecision() << "\n";
    collision_configuration = std::make_unique<btDefaultCollisionConfiguration>();
    collision_dispatcher    = std::make_unique<btCollisionDispatcher>(collision_configuration.get());
    overlapping_pair_cache  = std::make_unique<btDbvtBroadphase>();
    dynamics_world          = std::make_unique<btDiscreteDynamicsWorld>(
        collision_dispatcher.get(), overlapping_pair_cache.get(), solver.get(), collision_configuration.get());


    dynamics_world->setDebugDrawer(&my_drawer);
  }

  ~physics_system() { std::cout << "Deinitializing physics system\n";
  }

  void set_gravity(glm::vec3 G)
  {
    dynamics_world->setGravity(bullet_utils::convert(G));
  }

  void step_simulation(float delta_time_seconds)
  {
    dynamics_world->stepSimulation(
        delta_time_seconds, simulation_configuration.substep_per_frame, 1.F / simulation_configuration.substep_simulation_fps);
  }

  struct physics_proxy
  {
    float mass = 0;
    btVector3 local_inertia;
    btTransform xform = btTransform::getIdentity();

    void compute_inertia()
    {
      if(!collision_shape) return;
      local_inertia = btVector3(0, 0, 0);
      if(mass == 0) return;
      collision_shape->calculateLocalInertia(mass, local_inertia);
    }

    template<class MotionState>
    void create_motion_state()
    {
      motion_state = std::make_unique<MotionState>(xform);
    }

    void create_default_motion_state()
    {
      create_motion_state<btDefaultMotionState>();
    }

    void create_rigid_body()
    {
      btRigidBody::btRigidBodyConstructionInfo rb_info(mass, motion_state.get(), collision_shape.get(), local_inertia);
      rigid_body               = std::make_unique<btRigidBody>(rb_info);
    }

    transform get_world_transform() const
    {
      if(!rigid_body) return transform {};


      btTransform world_xform {};
      rigid_body->getMotionState()->getWorldTransform(world_xform);

      return bullet_utils::convert(world_xform);
    }

    std::unique_ptr<btCollisionShape> collision_shape{nullptr};
    std::unique_ptr<btMotionState> motion_state { nullptr };
    std::unique_ptr<btRigidBody> rigid_body{nullptr};
  };

  struct box_proxy : physics_proxy
  {
    box_proxy(glm::vec3 start_position, glm::vec3 half_extent, float mass_) : physics_proxy()
    {
      const btVector3 box_size = bullet_utils::convert(half_extent);
      xform.setOrigin(bullet_utils::convert(start_position));

      collision_shape          = std::make_unique<btBoxShape>(box_size);

      mass = mass_;
      compute_inertia();
      create_default_motion_state();
      create_rigid_body();
    }

    box_proxy(float size, float mass_) {

      const auto half_size = size / 2.f;
      glm::vec3 half_extent { half_size, half_size, half_size };
      *this = box_proxy({} , half_extent, mass);
    }
  };

  void add_to_world(physics_proxy& proxy)
  { dynamics_world->addRigidBody(proxy.rigid_body.get());
  }

  void add_ground_plane()
  {
    fake_ground_plane = std::make_unique<box_proxy>(glm::vec3(0, -1, 0), glm::vec3(500, 1, 500), 0);
    add_to_world(*fake_ground_plane);
  }

  void draw_phy_debug(const glm::mat4& view, const glm::mat4& projection, GLuint vao, shader_handle shader)
  {
    my_drawer.new_frame();
    dynamics_world->debugDrawWorld();
    my_drawer.draw_debug_data(view, projection, vao, shader);
  }

private:

    std::unique_ptr<physics_proxy> fake_ground_plane;
    std::unique_ptr<btCollisionConfiguration> collision_configuration;
    std::unique_ptr<btCollisionDispatcher> collision_dispatcher;
    std::unique_ptr<btBroadphaseInterface> overlapping_pair_cache;
    std::unique_ptr<btSequentialImpulseConstraintSolver> solver; //TODO see the multithreaded bullet3 examples;
    std::unique_ptr<btDynamicsWorld> dynamics_world;

  class debug_drawer : public btIDebugDraw
  {
    struct world_line
    {
      glm::vec3 from, to, color;
    };

    std::vector<world_line> to_draw;

  public:

    int debug_mode = DBG_DrawWireframe | DBG_DrawAabb | DBG_DrawContactPoints;

    void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override {
      to_draw.push_back({ bullet_utils::convert(from), bullet_utils::convert(to), bullet_utils::convert(color) });
    }

    void new_frame()
    {
      to_draw.clear();
    }

    void draw_debug_data(const glm::mat4& view, const glm::mat4& projection, GLuint vao, shader_handle shader_h)
    {
      GLuint restore_vao;
      glGetIntegerv(GL_VERTEX_ARRAY_BINDING, (GLint*)&restore_vao);

      glBindVertexArray(vao);

      auto& shader = shader_program_manager::get_from_handle(shader_h);
      shader.use();
      shader.set_uniform(shader::uniform::view, view);
      shader.set_uniform(shader::uniform::projection, projection);

      glm::vec3 line_vertex_data[2];
      for(auto& line : to_draw)
      {
        glm::vec4 color = { line.color, 1 };
        shader.set_uniform(shader::uniform::debug_color, color);
        line_vertex_data[0] = line.from;
        line_vertex_data[1] = line.to;
        glBufferData(GL_ARRAY_BUFFER, 2 * 3 * sizeof(float), line_vertex_data, GL_STREAM_DRAW);
        glDrawElements(GL_LINES, 2, GL_UNSIGNED_SHORT, (void*)0);
      }
      glBindVertexArray(restore_vao);
    }

    void drawContactPoint(const btVector3& PointOnB,
                          const btVector3& normalOnB,
                          btScalar distance,
                          int lifeTime,
                          const btVector3& color) override
    {
      drawLine(PointOnB, PointOnB + normalOnB * distance * 20, color);
    }

    void reportErrorWarning(const char* warningString) override { std::cerr << "physics error : " << warningString << "\n";}

    void draw3dText(const btVector3& location, const char* textString) override { }

    void setDebugMode(int debugMode) override
    {
      debug_mode = debugMode;
    }

    int getDebugMode() const override
    {
      return debug_mode;
    }
  };

  debug_drawer my_drawer;

};