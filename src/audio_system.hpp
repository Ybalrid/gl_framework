#pragma once

#ifdef __APPLE__
#include <al.h>
#include <alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif
#include <memory>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <sndfile.h>
#include <physfs.h>

class audio_buffer;
class audio_source;

///Initialize OpenAL
class audio_system
{

  ALCdevice* device   = nullptr;
  ALCcontext* context = nullptr;

  void steal_guts(audio_system& other);

  static SF_VIRTUAL_IO soundfile_io;

  struct soundfile_buffer
  {
    sf_count_t seek_position = 0;
    std::vector<uint8_t> data;
  };

  public:
  ///Open an audio output device
  audio_system(const char* device_name = nullptr);
  ~audio_system();

  ///No Copy
  audio_system(const audio_system&) = delete;
  ///No Copy
  audio_system& operator=(const audio_system&) = delete;

  ///Move
  audio_system(audio_system&& other) noexcept;
  ///Move
  audio_system& operator=(audio_system&& other) noexcept;

  ///Get an audio buffer with the data from an audio file
  static audio_buffer get_buffer(const std::string& virtual_path);

  void play_bgm(const std::string& virtual_path);
  audio_source* get_bgm_source() const { return bgm_source.get(); }

  std::shared_ptr<audio_source> bgm_source;
  std::shared_ptr<audio_buffer> bgm_buffer;
};

///Audio listener, represent the point in spcace where audio is "captured from"
class audio_listener
{
  ///This is a singleton-ish pattern
  static audio_listener* unique_listener;

  public:
  ///Create teh listener
  audio_listener();
  ///Destroy the listener
  ~audio_listener();

  ///Set the world transformation of the listener ( = model matrix )
  void set_world_transform(const glm::mat4& transform) const;

  ///Get the audio listener
  static audio_listener* get_listener();
};

///Configuration cookie of an audio buffer
struct buffer_config
{
  ALenum format;
  ALsizei size, freq;
};

///Audio buffer : Represent an open al buffer, created from an array of samples
class audio_buffer
{
  ALuint buffer = 0;
  buffer_config current_config {};

  void steal_guts(audio_buffer& other);

  public:
  audio_buffer() = default;
  audio_buffer(const std::vector<ALshort>& samples, const buffer_config& config);
  ~audio_buffer();

  audio_buffer(const audio_buffer&) = delete;
  audio_buffer& operator=(const audio_buffer&) = delete;

  audio_buffer(audio_buffer&&) noexcept;
  audio_buffer& operator=(audio_buffer&&) noexcept;

  ALuint get_al_buffer() const;
  buffer_config get_config() const;
};

///Audio source : point in space that can emit sound
class audio_source
{
  ALuint source = 0;

  void steal_guts(audio_source& other);

  public:
  audio_source();
  ~audio_source();

  audio_source(const audio_source&) = delete;
  audio_source& operator=(const audio_buffer&) = delete;

  audio_source(audio_source&& other) noexcept;
  audio_source& operator=(audio_source&& other) noexcept;

  ALuint get_al_source() const;

  ///Set the model matrix that place this source in the world
  void set_world_transform(const glm::mat4& transform) const;

  void set_buffer(const audio_buffer& buffer) const;
  void play() const;

  void set_looping(bool loop_state = true) const;
  void set_volume(float level) const;
  void set_pitch(float level) const;

  bool get_looping() const;
  float get_volume() const;
  float get_pitch() const;

  float get_playback_position() const;

  enum class state : ALint
  { playing = AL_PLAYING,
    paused = AL_PAUSED,
    stopped = AL_STOPPED,
  };

  static std::string state_to_string(state s)
  {
    switch(s)
    {
      case state::playing: return "playing";
      case state::paused: return "paused";
      case state::stopped: return "stopped";
    }
    return "error";
  }

  state get_state();

  void pause() const;
  void stop() const;
  void rewind() const;
};

///Empty object that can be stashed into a node
struct listener_marker
{
  void set_world_transform(const glm::mat4& transform) const;
};
