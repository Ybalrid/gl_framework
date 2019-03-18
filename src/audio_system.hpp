#pragma once

#ifdef __APPLE__
#include <al.h>
#include <alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <sndfile.h>
#include <physfs.h>

class audio_buffer;

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
	audio_system(const char* device_name = nullptr);
	~audio_system();

	audio_system(const audio_system&) = delete;
	audio_system& operator=(const audio_system&) = delete;

	audio_system(audio_system&& other) noexcept;
	audio_system& operator=(audio_system&& other) noexcept;

	static audio_buffer get_buffer(const std::string& virtual_path);
};

class audio_listener
{
	static audio_listener* unique_listener;

public:
	audio_listener();
	~audio_listener();
	void set_world_transform(const glm::mat4& transform) const;
	static audio_listener* get_listener();
};

struct buffer_config
{
	ALenum format;
	ALsizei size, freq;
};

class audio_buffer
{
	ALuint buffer = 0;
	buffer_config current_config{};

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
	void set_world_transform(const glm::mat4& transform) const;
};

//Empty object that can be stashed into a node
struct listener_marker
{
	void set_world_transform(const glm::mat4& transform) const;
};
