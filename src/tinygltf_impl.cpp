#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tiny_gltf.h>
#include "FreeImage.h"
#include "resource_system.hpp"

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

bool load_image_data(tinygltf::Image* image, const int image_idx, std::string* error, std::string* , int req_width, int req_height, const unsigned char* bytes, int size, void* context)
{
	//FreeImage's API inst const correct. Need to cast const away to give the pointer
	//Opening a memory stream in freeimage doesn't change the bytes given to it in our usage
	//But you could write to the opened memory stream...
	const auto image_stream = FreeImage_OpenMemory(const_cast<unsigned char*>(bytes), size);
	if(!image_stream)
	{
		if(error) *error = "FreeImage  " + std::to_string(image_idx) + " cannot open the image memory stream from the given pointer";

		return false;
	}

	const auto type = FreeImage_GetFileTypeFromMemory(image_stream);
	if(type == FIF_UNKNOWN)
	{
		if(error)
			*error = "FreeImage " + std::to_string(image_idx) + " cannot understand the type of the image";

		FreeImage_CloseMemory(image_stream);
		return false;
	}

	auto freeimage_image = FreeImage_LoadFromMemory(type, image_stream);

	if (!freeimage_image)
	{
		if(error)
		*error = "FreeImage loading [" + std::to_string(image_idx) + "] couldn't load image from given binary data";
		goto error_exit;
	}

	const auto w = FreeImage_GetWidth(freeimage_image);
	const auto h = FreeImage_GetWidth(freeimage_image);
	const auto bpp = FreeImage_GetBPP(freeimage_image);

	if(req_width && w != req_width)
	{
		if(error)
			*error = "FreeImage loading [" + std::to_string(image_idx) + "]: image width doesn't match requested one";

		goto error_exit;
	}

	if (req_height && h != req_height)
	{
		if(error)
			*error = "FreeImage loading [" + std::to_string(image_idx) + "]: image height doesn't match requested one";

		goto error_exit;
	}

	image->width = w;
	image->height = h;
	FIBITMAP* converted32bit = FreeImage_ConvertTo32Bits(freeimage_image);
	FreeImage_FlipVertical(converted32bit);
	image->component = 4;
	RGBQUAD pixel;
	for (unsigned x = 0; x < w; ++x)
		for (unsigned y = 0; y < h; ++y)
		{
			FreeImage_GetPixelColor(converted32bit, y, x, &pixel);
			image->image.push_back(pixel.rgbRed);
			image->image.push_back(pixel.rgbGreen);
			image->image.push_back(pixel.rgbBlue);
			image->image.push_back(0xFF);
		}
	image->image.shrink_to_fit();

	FreeImage_Unload(converted32bit);
	FreeImage_Unload(freeimage_image);
	FreeImage_CloseMemory(image_stream);
	return true;
	error_exit:
	FreeImage_Unload(freeimage_image);
	FreeImage_CloseMemory(image_stream);
	return false;
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

