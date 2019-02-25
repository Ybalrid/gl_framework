#pragma once

#include "resource_system.hpp"
#include <FreeImage.h>
#include <GL/glew.h>

class image
{
	FIBITMAP* bitmap = nullptr;

	void rgbize_bitmap()
	{
		const auto blue_mask = FreeImage_GetBlueMask(bitmap);
		const auto red_mask = FreeImage_GetRedMask(bitmap);
		const auto bits_per_pixel = FreeImage_GetBPP(bitmap);
		const auto bits = FreeImage_GetBits(bitmap);
		const auto w = get_width();
		const auto h = get_height();
		const auto px_count = w * h;

		if (bits_per_pixel == 32 && red_mask > blue_mask)
		{
			auto* data_array = reinterpret_cast<uint32_t*>(bits);
			for (size_t i = 0; i < px_count; ++i)
			{
				const auto pixel = data_array[i];
				const auto a = 0xff000000 & pixel;
				const auto r = 0x00ff0000 & pixel;
				const auto g = 0x0000ff00 & pixel;
				const auto b = 0x000000ff & pixel;

				data_array[i] = (a) | (b << 16) | (g) | (r >> 16);
			}
		}

		if (bits_per_pixel == 24)
		{
			if (red_mask > blue_mask)
			{
				auto* data_array = reinterpret_cast<uint8_t*>(bits);
				for (size_t i = 0; i < px_count; ++i)
				{
					std::swap(data_array[i * 3 + 0], data_array[i * 3 + 2]);
				}
			}
		}
	}

	void steal_guts(image& other)
	{
		bitmap = other.bitmap;
		other.bitmap = nullptr;
	}

public:
	explicit image(const std::string& virtual_path)
	{
		auto image_data = resource_system::get_file(virtual_path);

		const auto image_memory_stream = FreeImage_OpenMemory(image_data.data(), unsigned long(image_data.size()));
		const auto image_type = FreeImage_GetFileTypeFromMemory(image_memory_stream);

		if (image_type == FIF_UNKNOWN)
		{
			//TODO RAII this please!
			FreeImage_CloseMemory(image_memory_stream);
			throw std::runtime_error("The data from " + virtual_path + " doesn't seem to be a readable image");
		}
		bitmap = FreeImage_LoadFromMemory(image_type, image_memory_stream);
		FreeImage_CloseMemory(image_memory_stream);
		image_data.clear();

		if(!bitmap)
		{
			//For the love of my sanity, TODO RAII this!
			throw std::runtime_error("Couldn't load bitmap from " + virtual_path);
		}

		rgbize_bitmap();
	}

	~image()
	{
		if(bitmap)
			FreeImage_Unload(bitmap);
	}

	image(const image&&) = delete;
	image& operator=(const image&&) = delete;
	image(image&& other)
	{
		steal_guts(other);
	}

	image& operator=(image&& other)
	{
		steal_guts(other);
		return *this;
	}

	int get_width() const
	{
		return FreeImage_GetWidth(bitmap);
	}

	int get_height() const
	{
		return FreeImage_GetHeight(bitmap);
	}

	enum class type { rgba, rgb };
	static GLint get_gl_type(type t)
	{
		switch(t)
		{
		case type::rgb: 
			return GL_RGB;
		case type::rgba: 
			return GL_RGBA;
		default:
			throw std::runtime_error("cannot make sence of your type thing");
		}
	}

	type get_type() const
	{
		if (FreeImage_GetBPP(bitmap) == 32)
			return type::rgba;
		return type::rgb;
	}

	BYTE* get_binary() const
	{
		return FreeImage_GetBits(bitmap);
	}
};
