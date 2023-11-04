#pragma once

#include <memory>

#include <vector>
#include <iostream>
#include "shader_program_manager.hpp"
#include "opengl_debug_group.hpp"

#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"

#include "transform.hpp"

class node;

namespace bullet_utils
{
  inline glm::vec3 convert(btVector3 v) { return glm::vec3(v.x(), v.y(), v.z()); }
  inline glm::quat convert(btQuaternion q) { return glm::quat(q.w(), q.x(), q.y(), q.z()); };
  inline btVector3 convert(glm::vec3 v) { return btVector3(v.x, v.y, v.z); }
  inline btQuaternion convert(glm::quat q) { return btQuaternion(q.x, q.y, q.z, q.w); }

  inline transform convert(const btTransform& t)
  {
    const auto rotation = t.getRotation();
    const auto origin   = t.getOrigin();

    transform xform;
    xform.set_position(convert(origin));
    xform.set_orientation(convert(rotation));
    xform.set_scale(glm::vec3(1.f));
    return xform;
  }

  inline btTransform convert(const transform& t)
  {
    btTransform xform;

    xform.setOrigin(convert(t.get_position()));
    xform.setRotation(convert(t.get_orientation()));

    return xform;
  }

  class transform_sync : public btMotionState
  {
public:
    transform_sync(node* attachee);
    void getWorldTransform(btTransform& worldTrans) const override;
    void setWorldTransform(const btTransform& worldTrans) override;

private:
    node* scene_node;
    glm::vec3 local_scale;
  };
}

class physics_system
{
  public:
  bool draw_debug_wireframe                                                 = false;
  static constexpr glm::vec3 earth_average_gravitational_acceleration_field = glm::vec3(0.f, -9.80665f, 0.f);

  struct
  {
    int substep_per_frame        = 4;
    float substep_simulation_fps = 60;
  } simulation_configuration;

  physics_system();
  ~physics_system();

  void set_gravity(glm::vec3 G) const;

  void step_simulation(float delta_time_seconds) const;

  struct physics_proxy
  {
    float mass = 0;
    btVector3 local_inertia;
    btTransform xform = btTransform::getIdentity();

    void compute_inertia();

    template <class MotionState>
    void create_motion_state()
    {
      motion_state = std::make_unique<MotionState>(xform);
    }

    void create_default_motion_state();
    void create_rigid_body();

    [[nodiscard]] transform get_world_transform() const;

    std::unique_ptr<btCollisionShape> collision_shape { nullptr };
    std::unique_ptr<btMotionState> motion_state { nullptr };
    std::unique_ptr<btRigidBody> rigid_body { nullptr };
  };

  struct box_proxy : physics_proxy
  {
    box_proxy(glm::vec3 start_position, glm::vec3 half_extent, float mass_);
    box_proxy(float size, float mass_);
  };

  void add_to_world(physics_proxy& proxy) const;

  void add_ground_plane();

  void draw_phy_debug(const glm::mat4& view, const glm::mat4& projection, GLuint vao, shader_handle shader);

  enum class shape { box, static_triangle_mesh, convex_hull };

  physics_proxy create_proxy(shape s,
                             const std::vector<float>& vertex_buffer,
                             const std::vector<unsigned int>& index_buffer,
                             size_t vertex_buffer_stride,
                             float mass,
                             glm::vec3 local_scale       = glm::vec3(1.f),
                             btMotionState* motion_state = nullptr);

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
      glm::vec3 from;
      glm::vec4 from_color;
      glm::vec3 to;
      glm::vec4 to_color;
    };
    std::vector<world_line> to_draw;
    int debug_mode = DBG_FastWireframe | DBG_DrawAabb | DBG_DrawContactPoints;

public:
    debug_drawer();
    void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override;
    void new_frame();
    void draw_debug_data(const glm::mat4& view, const glm::mat4& projection, GLuint vao, shader_handle shader_h);
    void drawContactPoint(
        const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override;
    void reportErrorWarning(const char* warningString) override;
    void draw3dText(const btVector3& location, const char* textString) override;
    void setDebugMode(int debugMode) override;
    int getDebugMode() const override;
  };
  debug_drawer simple_debug_drawer;
};