#include "physics_system.hpp"

#include "node.hpp"

bullet_utils::transform_sync::transform_sync(node* attachee)
{
  scene_node = attachee;
  local_scale = scene_node->local_xform.get_scale();
}

void bullet_utils::transform_sync::getWorldTransform(btTransform& worldTrans) const
{
  worldTrans = convert(scene_node->local_xform);
}
void bullet_utils::transform_sync::setWorldTransform(const btTransform& worldTrans)
{
  scene_node->local_xform = convert(worldTrans);
  scene_node->local_xform.set_scale(local_scale);
}

physics_system::physics_system()
{
  std::cout << "Initializing physics system with Bullet Physics version " << btGetVersion() << " : " << btIsDoublePrecision()
            << "\n";
  collision_configuration = std::make_unique<btDefaultCollisionConfiguration>();
  collision_dispatcher    = std::make_unique<btCollisionDispatcher>(collision_configuration.get());
  overlapping_pair_cache  = std::make_unique<btDbvtBroadphase>();
  dynamics_world          = std::make_unique<btDiscreteDynamicsWorld>(
      collision_dispatcher.get(), overlapping_pair_cache.get(), solver.get(), collision_configuration.get());

  dynamics_world->setDebugDrawer(&simple_debug_drawer);
}

physics_system::~physics_system()
{
  fprintf(stderr,"Uninitialized physics system\n");
}

void physics_system::set_gravity(glm::vec3 G) const { dynamics_world->setGravity(bullet_utils::convert(G)); }

void physics_system::step_simulation(float delta_time_seconds) const
{
  dynamics_world->stepSimulation(
      delta_time_seconds, simulation_configuration.substep_per_frame, 1.F / simulation_configuration.substep_simulation_fps);
}

void physics_system::physics_proxy::compute_inertia()
{
  if(!collision_shape) return;
  local_inertia = btVector3(0, 0, 0);
  if(mass == 0) return;
  collision_shape->calculateLocalInertia(mass, local_inertia);
}

void physics_system::physics_proxy::create_default_motion_state() { create_motion_state<btDefaultMotionState>(); }

void physics_system::physics_proxy::create_rigid_body()
{
  btRigidBody::btRigidBodyConstructionInfo rb_info(mass, motion_state.get(), collision_shape.get(), local_inertia);
  rigid_body = std::make_unique<btRigidBody>(rb_info);
}

transform physics_system::physics_proxy::get_world_transform() const
{
  if(!rigid_body) return transform {};

  btTransform world_xform {};
  rigid_body->getMotionState()->getWorldTransform(world_xform);

  return bullet_utils::convert(world_xform);
}

physics_system::box_proxy::box_proxy(glm::vec3 start_position, glm::vec3 half_extent, float mass_) : physics_proxy()
{
  const btVector3 box_size = bullet_utils::convert(half_extent);
  xform.setOrigin(bullet_utils::convert(start_position));

  collision_shape = std::make_unique<btBoxShape>(box_size);

  mass = mass_;
  compute_inertia();
  create_default_motion_state();
  create_rigid_body();
}

physics_system::box_proxy::box_proxy(float size, float mass_)
{
  mass = mass_;
  const auto half_size = size * .5f;
  glm::vec3 half_extent { half_size, half_size, half_size };
  *this = box_proxy({}, half_extent, mass);
}

void physics_system::add_to_world(physics_proxy& proxy) const
{
  if(!proxy.rigid_body) return;
  proxy.rigid_body->setWorldTransform(proxy.xform);
  dynamics_world->addRigidBody(proxy.rigid_body.get());
}

void physics_system::add_ground_plane()
{
  fake_ground_plane = std::make_unique<box_proxy>(glm::vec3(0, -1, 0), glm::vec3(500, 1, 500), 0);
  add_to_world(*fake_ground_plane);
}

void physics_system::draw_phy_debug(const glm::mat4& view, const glm::mat4& projection, GLuint vao, shader_handle shader)
{
  static int counter = 0;
  //if(counter++ % 4 == 0)
  {
    simple_debug_drawer.setDebugMode(btIDebugDraw::DBG_FastWireframe | btIDebugDraw::DBG_DrawAabb
                                     | btIDebugDraw::DBG_DrawContactPoints | (draw_debug_wireframe? btIDebugDraw::DBG_DrawWireframe : 0));
    simple_debug_drawer.new_frame();
    dynamics_world->debugDrawWorld();
  }
  simple_debug_drawer.draw_debug_data(view, projection, vao, shader);
}

physics_system::physics_proxy physics_system::create_proxy(shape s,
                                                           const std::vector<float>& vertex_buffer,
                                                           const std::vector<unsigned>& index_buffer,
                                                           size_t stride,
                                                           float mass,
                                                           glm::vec3 local_scale,
                                                           btMotionState* motion_state)
{
  if(vertex_buffer.size() == 0)
      return {};

  physics_proxy proxy;

  proxy.mass = mass;

  switch(s)
  {
    case shape::box:
      //TODO compute bounding box, and create a box shape
      break;
    case shape::static_triangle_mesh: {
      const auto triangle_count = index_buffer.size() / 3;
      auto triangle_mesh        = new btTriangleIndexVertexArray();

      btIndexedMesh indexed_mesh;
      indexed_mesh.m_indexType = PHY_INTEGER;
      indexed_mesh.m_numTriangles = index_buffer.size() / 3;
      indexed_mesh.m_numVertices  = vertex_buffer.size() / stride;
      indexed_mesh.m_vertexStride = sizeof(float) * stride;
      indexed_mesh.m_triangleIndexStride = 3 * sizeof(unsigned);
      indexed_mesh.m_vertexBase = (const unsigned char*)vertex_buffer.data();
      indexed_mesh.m_triangleIndexBase = (const unsigned char*)index_buffer.data();
      indexed_mesh.m_vertexType          = PHY_FLOAT;
      triangle_mesh->addIndexedMesh(indexed_mesh);

      const auto collision_shape = new btBvhTriangleMeshShape(triangle_mesh, true);
      proxy.collision_shape      = std::unique_ptr<btCollisionShape>(collision_shape);

      //TODO check if the btBvhTriangleMeshShape takes ownership of the btTriangleMesh pointer...
    }
    break;

    case shape::convex_hull: {
      const auto collision_shape = new btConvexHullShape(static_cast<const btScalar*>(vertex_buffer.data()),
                                                         int(vertex_buffer.size()) / int(stride),
                                                         static_cast<int>(stride) * sizeof(float));
      proxy.collision_shape      = std::unique_ptr<btCollisionShape>(collision_shape);
    }
  }

  proxy.collision_shape->setLocalScaling(bullet_utils::convert(local_scale));

  if(mass != 0) proxy.compute_inertia();

  if(motion_state == nullptr)
    proxy.create_motion_state<btDefaultMotionState>();
  else
    proxy.motion_state = std::unique_ptr<btMotionState>(motion_state);

  proxy.create_rigid_body();

  proxy.rigid_body->setSleepingThresholds(10000, 10000);

  return proxy;
}

physics_system::debug_drawer::debug_drawer()
{
  to_draw.reserve(1024*1024*1024*4);
}

void physics_system::debug_drawer::drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
{
  to_draw.push_back(world_line{ bullet_utils::convert(from),
                                   glm::vec4(bullet_utils::convert(color), 1),
                                   bullet_utils::convert(to),
                                   glm::vec4(bullet_utils::convert(color), 1) });
}

void physics_system::debug_drawer::new_frame() { to_draw.clear(); }

void physics_system::debug_drawer::draw_debug_data(const glm::mat4& view,
                                                   const glm::mat4& projection,
                                                   GLuint vao,
                                                   shader_handle shader_h)
{
  opengl_debug_group debug_group("physics_system::debug_draw::draw_debug_data()");

  //Note, this is a debug feature, and exist only outside of the renderer. This will corrupt the VAO state tracking, so manually restore the currently bound VAO
  GLuint vao_to_be_restored;
  glGetIntegerv(GL_VERTEX_ARRAY_BINDING, (GLint*)&vao_to_be_restored);
  GLfloat line_width_to_be_restored;
  glGetFloatv(GL_LINE_WIDTH, &line_width_to_be_restored);

  glBindVertexArray(vao);

  auto& shader = shader_program_manager::get_from_handle(shader_h);
  shader.use();
  shader.set_uniform(shader::uniform::view, view);
  shader.set_uniform(shader::uniform::projection, projection);

  glLineWidth(1);

  {
    glBufferData(GL_ARRAY_BUFFER, to_draw.size() * sizeof(world_line), to_draw.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_LINES, 0, to_draw.size() * (sizeof(world_line) / sizeof(float)));
  }

  glBindVertexArray(vao_to_be_restored);
  glLineWidth(line_width_to_be_restored);
}

void physics_system::debug_drawer::drawContactPoint(
    const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color)
{
  drawLine(PointOnB, PointOnB + normalOnB * distance * 20, color);
}

void physics_system::debug_drawer::reportErrorWarning(const char* warningString)
{
  std::cerr << "physics error : " << warningString << "\n";
}

void physics_system::debug_drawer::draw3dText(const btVector3& location, const char* textString)
{
  //Note if we ever add something that can draw 3D text, well... We will use it here.
}

void physics_system::debug_drawer::setDebugMode(int debugMode) { debug_mode = debugMode; }

int physics_system::debug_drawer::getDebugMode() const { return debug_mode; }
