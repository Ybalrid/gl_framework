#include "level_system.hpp"
#include "resource_system.hpp"
#include "node.hpp"
#include "scene.hpp"
#include "script_system.hpp"

using namespace nlohmann;

bool level_system::load_level(script_system& script_engine, gltf_loader & gltf, scene& s, const std::string& level_name) const
{
  try
  {
    const std::string level_file_path = "/levels/" + level_name + ".json";
    const auto level_json_file        = resource_system::get_file(level_file_path);
    const std::string level_json_text(reinterpret_cast<const char*>(level_json_file.data()), level_json_file.size());
    const auto level_json_array = json::parse(level_json_text);

    node* level_root = s.scene_root->push_child(create_node(level_name));

    if(level_json_array.is_array())
    {
      bool has_at_least_one_asset = false;
      for(const auto& level_asset : level_json_array)
      {
        const auto asset_it       = level_asset.find("asset");
        const auto position_it    = level_asset.find("position");
        const auto orientation_it = level_asset.find("orientation");
        const auto scale_it       = level_asset.find("scale");
        const auto script_it      = level_asset.find("script");

        if(asset_it == std::end(level_asset)) continue;
        has_at_least_one_asset = true; //Signal potential success in loading *something* :-)

        std::string asset_path = *asset_it;
        auto level_asset_root  = level_root->push_child(create_node(asset_path));
        auto asset             = gltf.load_mesh(asset_path);
        level_asset_root->assign(scene_object(asset));

        if(scale_it != std::end(level_asset) && scale_it->is_array())
        {

          glm::vec3 scale;

          scale.x = scale_it->at(0);
          scale.y = scale_it->at(1);
          scale.z = scale_it->at(2);

          level_asset_root->local_xform.set_scale(scale);
        }

        if(position_it != std::end(level_asset) && position_it->is_array())
        {
          glm::vec3 position;

          position.x = position_it->at(0);
          position.y = position_it->at(1);
          position.z = position_it->at(2);

          level_asset_root->local_xform.set_position(position);
        }

        if(orientation_it != std::end(level_asset) && orientation_it->is_array())
        {
          glm::quat orientation;

          orientation.x = orientation_it->at(0);
          orientation.y = orientation_it->at(1);
          orientation.z = orientation_it->at(2);
          orientation.w = orientation_it->at(3);

          level_asset_root->local_xform.set_orientation(orientation);
        }

        if(script_it != std::end(level_asset) && script_it->is_string())
        {
          script_engine.attach_behavior_script(*script_it, level_asset_root);
        }

      }
      return has_at_least_one_asset;
    }
    return false;
  }
  catch(const std::exception& e)
  {
    std::cerr << "Exception triggered while loading " << level_name << " : " << e.what() << "\n";
    return false;
  }
}