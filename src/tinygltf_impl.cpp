#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tiny_gltf.h>
#include "FreeImage.h"
#include "resource_system.hpp"
#include "image.hpp"

std::string expand_file_path(const std::string& filepath, void*)
{
	//TODO do we need to do anything?
	return filepath;
}

bool file_exist(const std::string& path, void*)
{
	return PHYSFS_exists(path.c_str()) != 0;
}

bool read_whole_file(std::vector<unsigned char>* bytes, std::string* err, const std::string& path, void*)
{
	try
	{
		*bytes = resource_system::get_file(path);
	}
	catch (const std::exception& e)
	{
		*err = e.what();
		return false;
	}

	return true;
}

bool load_image_data(tinygltf::Image* image, const int image_idx, std::string* error, std::string*, int req_width, int req_height, const unsigned char* bytes, int size, void* context)
{
	//FreeImage's API inst const correct. Need to cast const away to give the pointer
	//Opening a memory stream in freeimage doesn't change the bytes given to it in our usage
	//But you could write to the opened memory stream...
	freeimage_memory image_stream(FreeImage_OpenMemory(const_cast<unsigned char*>(bytes), size));
	if (!image_stream.get())
	{
		if (error)
			*error = "FreeImage  " + std::to_string(image_idx) + " cannot open the image memory stream from the given pointer";
		return false;
	}

	const auto type = FreeImage_GetFileTypeFromMemory(image_stream.get());
	if (type == FIF_UNKNOWN)
	{
		if (error)
			*error = "FreeImage " + std::to_string(image_idx) + " cannot understand the type of the image";
		return false;
	}

	freeimage_image loaded_image(image_stream.load());

	if (!loaded_image.get())
	{
		if (error)
			*error = "FreeImage loading [" + std::to_string(image_idx) + "] couldn't load image from given binary data";
		return false;
	}

	const auto w = FreeImage_GetWidth(loaded_image.get());
	const auto h = FreeImage_GetWidth(loaded_image.get());

	if (req_width && w != req_width)
	{
		if (error)
			*error = "FreeImage loading [" + std::to_string(image_idx) + "]: image width doesn't match requested one";
		return false;
	}

	if (req_height && h != req_height)
	{
		if (error)
			*error = "FreeImage loading [" + std::to_string(image_idx) + "]: image height doesn't match requested one";
		return false;
	}

	//Some drivers will only accept 32bit images apparently, convert everything.
	image->width = w;
	image->height = h;
	image->component = 4;

	if(FreeImage_GetBPP(loaded_image.get()) != 32)
		loaded_image = FreeImage_ConvertTo32Bits(loaded_image.get());
	
	//Oh, my dear OpenGL. You and your silly texture coordinate space. Let me fix that for you...
	FreeImage_FlipVertical(loaded_image.get());
	auto bits = FreeImage_GetBits(loaded_image.get());

	image->image.reserve(w*h * 4);
	for (auto pixel = 0U; pixel < w*h; ++pixel)
	{
		image->image.push_back(bits[pixel * 4 + 2]);
		image->image.push_back(bits[pixel * 4 + 1]);
		image->image.push_back(bits[pixel * 4 + 0]);
		image->image.push_back(bits[pixel * 4 + 3]);
	}
	image->image.shrink_to_fit();

	return true;
}

void tinygltf_freeimage_setup(tinygltf::TinyGLTF& gltf)
{
	gltf.SetImageLoader(load_image_data, nullptr);
}

void tinygltf_resource_system_setup(tinygltf::TinyGLTF& gltf)
{
	tinygltf::FsCallbacks callbacks{};

	callbacks.FileExists = file_exist;
	callbacks.ReadWholeFile = read_whole_file;
	callbacks.ExpandFilePath = expand_file_path;
	callbacks.WriteWholeFile = nullptr;
	callbacks.user_data = nullptr;

	gltf.SetFsCallbacks(callbacks);
}

