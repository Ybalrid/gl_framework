#pragma once

#include <FreeImage.h>

#include <iostream>

struct freeimage
{
	freeimage()
	{
		FreeImage_Initialise();
#ifndef HIDE_BANNER
		std::cout << FreeImage_GetCopyrightMessage() << '\n';
#endif
	}

	~freeimage()
	{
		FreeImage_DeInitialise();
	}

	freeimage(const freeimage&) = delete;
	freeimage& operator=(const freeimage&) = delete;
	freeimage(freeimage&&) = delete;
	freeimage& operator=(freeimage&&) = delete;
};

struct freeimage_image
{
	freeimage_image()
	{
	}

	freeimage_image(FIBITMAP* naked) : ptr(naked)
	{
	}

	~freeimage_image()
	{
		if(ptr)
			FreeImage_Unload(ptr);
	}

	FIBITMAP* get() const
	{
		return ptr;
	}

	void set_ptr_to(FIBITMAP* naked)
	{
		if (ptr)
			FreeImage_Unload(ptr);
		ptr = naked;
	}

	freeimage_image(const freeimage_image&) = delete;
	freeimage_image& operator=(const freeimage_image&) = delete;
	freeimage_image(freeimage_image&& o) noexcept
	{
		ptr = o.ptr;
		o.ptr = nullptr;
	}

	freeimage_image& operator=(freeimage_image&& o) noexcept
	{
		ptr = o.ptr;
		o.ptr = nullptr;
		return *this;
	}
private:
	FIBITMAP* ptr = nullptr;
};

struct freeimage_memory
{
	freeimage_memory()
	{
	}

	freeimage_memory(unsigned char* memory, unsigned char size) : ptr(FreeImage_OpenMemory(memory, size))
	{
	}

	freeimage_memory(FIMEMORY* naked) : ptr(naked)
	{
	}

	~freeimage_memory()
	{
		if(ptr)
			FreeImage_CloseMemory(ptr);
	}

	FIMEMORY* get() const
	{
		return ptr;
	}

	void set_ptr_to(FIMEMORY* new_ptr)
	{
		if (ptr) FreeImage_CloseMemory(ptr);
		ptr = new_ptr;
	}

	freeimage_memory(const freeimage_memory&) = delete;
	freeimage_memory& operator=(const freeimage_memory&) = delete;
	freeimage_memory(freeimage_memory&& o) noexcept
	{
		ptr = o.ptr;
		o.ptr = nullptr;
	}

	freeimage_memory& operator=(freeimage_memory&& o) noexcept
	{
		ptr = o.ptr;
		o.ptr = nullptr;
		return *this;
	}

	freeimage_image load()
	{
		const auto type = FreeImage_GetFileTypeFromMemory(ptr);
		return { FreeImage_LoadFromMemory(type, ptr) };
	}

private:
	FIMEMORY* ptr = nullptr;
};

