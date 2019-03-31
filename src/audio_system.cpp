#include "audio_system.hpp"
#include <iostream>
#include <glm/gtc/quaternion.hpp>
#include <array>
#include "resource_system.hpp"

void audio_system::steal_guts(audio_system& other)
{
	device  = other.device;
	context = other.context;

	other.device  = nullptr;
	other.context = nullptr;
}

SF_VIRTUAL_IO audio_system::soundfile_io{};

audio_system::audio_system(const char* device_name)
{
	device = alcOpenDevice(device_name);
	if(!device) throw std::runtime_error("couldn't create openal device");
	context = alcCreateContext(device, nullptr);
	if(!context) throw std::runtime_error("couldn't create openal context");
	alcMakeContextCurrent(context);

	(void)audio_listener::get_listener();

	std::cout << "Initialized OpenAL " << alGetString(AL_VERSION) << " from '" << alGetString(AL_VENDOR) << "' based Audio system\n";

	soundfile_io.get_filelen = [](void* user_data) -> sf_count_t {
		auto* handle = reinterpret_cast<soundfile_buffer*>(user_data);
		return handle->data.size();
	};

	soundfile_io.seek = [](sf_count_t offset, int whence, void* user_data) -> sf_count_t {
		auto* handle = reinterpret_cast<soundfile_buffer*>(user_data);
		switch(whence)
		{
			case SEEK_CUR:
				handle->seek_position += offset;
				break;
			case SEEK_END:
				handle->seek_position = handle->data.size() + offset;
				break;
			case SEEK_SET:
				handle->seek_position = offset;
				break;
			default:
				break;
		}

		return handle->seek_position;
	};

	soundfile_io.read = [](void* ptr, sf_count_t count, void* user_data) -> sf_count_t {
		auto* handle		= reinterpret_cast<soundfile_buffer*>(user_data);
		ALubyte* data_start = handle->data.data() + handle->seek_position;
		memcpy(ptr, data_start, count);
		handle->seek_position += count;
		return count;
	};

	soundfile_io.tell = [](void* user_data) -> sf_count_t {
		return reinterpret_cast<soundfile_buffer*>(user_data)->seek_position;
	};

	soundfile_io.write = nullptr;
}

audio_system::~audio_system()
{
	if(device)
	{
		alcMakeContextCurrent(nullptr);
		if(context)
			alcDestroyContext(context);

		alcCloseDevice(device);
		alGetError();
		std::cout << "Deinitialized Audio system\n";
	}

	device  = nullptr;
	context = nullptr;
}

audio_system::audio_system(audio_system&& other) noexcept
{
	steal_guts(other);
}

audio_system& audio_system::operator=(audio_system&& other) noexcept
{
	steal_guts(other);
	return *this;
}

audio_buffer audio_system::get_buffer(const std::string& virtual_path)
{
	//Open the file
	soundfile_buffer file;
	file.data		   = resource_system::get_file(virtual_path);
	file.seek_position = 0;
	SF_INFO info;
	SNDFILE* sound_file = sf_open_virtual(&soundfile_io, SFM_READ, &info, &file);

	//Allocate buffer transport data
	const ALsizei sample_count = static_cast<ALsizei>(info.channels * info.frames);
	const ALsizei sample_rate  = static_cast<ALsizei>(info.samplerate);
	std::vector<ALshort> sample_buffer(sample_count);

	//read samples into buffer
	const auto actually_read = sf_read_short(sound_file, sample_buffer.data(), sample_count);
	sf_close(sound_file);

	//Handle config
	const buffer_config config{
		[&](const int c) {
			switch(c)
			{
				case 1:
					return AL_FORMAT_MONO16;
				case 2:
					return AL_FORMAT_STEREO16;
				default:
					throw std::runtime_error("Unreconginzed channel count in file " + virtual_path);
			}
		}(info.channels),
		static_cast<ALsizei>(sizeof(ALshort) * sample_count),
		sample_rate
	};

	//Create and return an openal buffer containing that data
	return audio_buffer(sample_buffer, config);
}

audio_listener* audio_listener::unique_listener = nullptr;

audio_listener::audio_listener()
{
	if(unique_listener) throw std::runtime_error("Cannot have more that one audio listener");
	unique_listener = this;
}

audio_listener::~audio_listener()
{
	unique_listener = nullptr;
}

void audio_listener::set_world_transform(const glm::mat4& transform) const
{
	const glm::vec3 position(transform[3]);							   //extract translation vector
	const glm::quat orientation(glm::normalize(glm::quat(transform))); //extract orientation quaternion

	//rotate a vector pointing through the screen (-Z axis) and to the top (+Y axis)
	const glm::vec3 orientation_at = orientation * glm::vec3(0, 0, -1.f);
	const glm::vec3 orientation_up = orientation * glm::vec3(0, 1.f, 0);

	//Construct an orientation vector as OpenAL want's it
	const std::array<ALfloat, 6> al_orientation{ orientation_at.x, orientation_at.y, orientation_at.z, orientation_up.x, orientation_up.y, orientation_up.z };

	//Set the listener position and orientation
	alListener3f(AL_POSITION, position.x, position.y, position.z);
	alListenerfv(AL_ORIENTATION, al_orientation.data());
}

audio_listener* audio_listener::get_listener()
{
	if(!unique_listener) return new audio_listener;
	return unique_listener;
}

void audio_buffer::steal_guts(audio_buffer& other)
{
	buffer		   = other.buffer;
	other.buffer   = 0;
	current_config = other.current_config;
}

audio_buffer::audio_buffer(const std::vector<ALshort>& samples, const buffer_config& config) :
 current_config(config)
{
	alGenBuffers(1, &buffer);
	alBufferData(buffer, config.format, samples.data(), config.size, config.freq);
}

audio_buffer::~audio_buffer()
{
	if(buffer > 0)
		alDeleteBuffers(1, &buffer);
}

audio_buffer::audio_buffer(audio_buffer&& other) noexcept
{
	steal_guts(other);
}

audio_buffer& audio_buffer::operator=(audio_buffer&& other) noexcept
{
	steal_guts(other);
	return *this;
}

ALuint audio_buffer::get_al_buffer() const
{
	return buffer;
}

buffer_config audio_buffer::get_config() const
{
	return current_config;
}

void audio_source::steal_guts(audio_source& other)
{
	source		 = other.source;
	other.source = 0;
}

audio_source::audio_source()
{
	alGenSources(1, &source);
}

audio_source::~audio_source()
{
	if(source > 0)
		alDeleteSources(1, &source);
}

audio_source::audio_source(audio_source&& other) noexcept
{
	steal_guts(other);
}

audio_source& audio_source::operator=(audio_source&& other) noexcept
{
	steal_guts(other);
	return *this;
}

ALuint audio_source::get_al_source() const
{
	return source;
}

void audio_source::set_world_transform(const glm::mat4& transform) const
{
	//extract 3D translation vector
	const glm::vec3 position = transform[3];
	alSource3f(source, AL_POSITION, position.x, position.y, position.z);
}

void audio_source::set_buffer(const audio_buffer & buffer) const
{
	alSourcei(source, AL_BUFFER, buffer.get_al_buffer());
}

void audio_source::play() const
{
	alSourcePlay(source);
}

void audio_source::set_looping(bool loop_state) const
{
	alSourcei(source, AL_LOOPING, loop_state ? AL_TRUE : AL_FALSE);
}

void audio_source::set_volume(float level) const
{
	alSourcef(source, AL_GAIN, level);
}

void audio_source::set_pitch(float level) const
{
	alSourcef(source, AL_PITCH, level);
}

void audio_source::pause() const
{
	alSourcePause(source);
}

void audio_source::stop() const
{
	alSourceStop(source);
}

void audio_source::rewind() const
{
	alSourceRewind(source);
}

void listener_marker::set_world_transform(const glm::mat4& transform) const
{
	audio_listener::get_listener()->set_world_transform(transform);
}
