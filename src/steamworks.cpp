#include "steamworks.hpp"
#include "texture_manager.hpp"

static steamworks* steamwork_callback_instance = nullptr;

steamworks::steamworks()
{
  if(steamapi_initialized = SteamAPI_Init(); !steamapi_initialized) { printf("failed to init steamworks...\n"); }

  if(steamapi_initialized)
  {
    steamUtils = SteamUtils();
    steamInput = SteamInput();

    steamInput->Init(true);

    steamwork_callback_instance = this;


    steamInput->EnableDeviceCallbacks();

    steamInput->EnableActionEventCallbacks([](SteamInputActionEvent_t* input_action_event)
    {
      steamwork_callback_instance->action_event_callback(input_action_event);
    });

    printf("Initialized SteamWorks with AppID : %d\n", steamUtils->GetAppID());
  }
}

steamworks::~steamworks()
{

  steamInput->Shutdown();
  SteamAPI_Shutdown();
  printf("Deinitialized SteamWorks\n");
}

void steamworks::update_steam_input()
{
  if(!steamapi_initialized) return;

  steamInput->RunFrame();
}

void steamworks::load_controller_glyphs()
{
  if(!steamapi_initialized) return;
  controller_glyphs.resize(k_EInputActionOrigin_Count, texture_manager::invalid_texture);
  for(auto input_origin = k_EInputActionOrigin_None + 1; input_origin < k_EInputActionOrigin_Count; ++input_origin)
  {
    const char* file_path
        = steamInput->GetGlyphPNGForActionOrigin(static_cast<EInputActionOrigin>(input_origin), k_ESteamInputGlyphSize_Large, 0);

    if(file_path)
    {
      printf("loading glyph from %s\n", file_path);
      FILE* png_file = fopen(file_path, "rb");
      if(png_file) //if file open, get the bytes
      {
        fseek(png_file, 0, SEEK_END);
        const size_t png_file_size     = ftell(png_file);
        unsigned char* png_file_buffer = (unsigned char*)calloc(1, png_file_size);
        if(!png_file_buffer)
        {
          fclose(png_file);
          continue;
        }

        fseek(png_file, 0, SEEK_SET);
        if(fread(png_file_buffer, png_file_size, 1, png_file) == 1) { ; }
        fclose(png_file);

        freeimage_memory memory_buffer(png_file_buffer, png_file_size);
        freeimage_image png_image = memory_buffer.load();
        image png_image_load(std::move(png_image));

        texture_handle glyph_tex_handle = texture_manager::create_texture();
        auto& glyph_tex                 = texture_manager::get_from_handle(glyph_tex_handle);

        glyph_tex.load_from(png_image_load);
        glyph_tex.generate_mipmaps();

        controller_glyphs[input_origin] = glyph_tex_handle;
        free(png_file_buffer);
      }
    }
  }
}

void steamworks::action_event_callback(SteamInputActionEvent_t* input_action_event)
{
  printf("action for controller handle %d\n", input_action_event->controllerHandle);
  switch(input_action_event->eEventType)
  {
    case ESteamInputActionEventType_DigitalAction:
      printf("digital action handle : %d\n", input_action_event->digitalAction.actionHandle);
      printf("digital action state : %d\n", input_action_event->digitalAction.digitalActionData.bState);
      break;
    case ESteamInputActionEventType_AnalogAction:
      printf("analog action handle : %d\n", input_action_event->analogAction.actionHandle);
      printf("analog x = %f\n", input_action_event->analogAction.analogActionData.x);
      printf("analog y = %f\n", input_action_event->analogAction.analogActionData.y);
      break;
  }
}
