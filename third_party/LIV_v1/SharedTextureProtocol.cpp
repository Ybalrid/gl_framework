#include "SharedTextureProtocol.h"
#ifdef _WIN32

#include <cstdint>
#include <string>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <cassert>

struct SharedTextureInfo
{
	uint32_t Version;
	uint32_t Width;
	uint32_t Height;
	uint32_t VFlip;
	uint64_t TextureHandle;
	bool bIsActive;
};
#define SHM_NAME L"liv-client-shared-texture-info"

struct SharedTextureState
{
	SharedTextureState()
	{
		const auto ProcessId = GetCurrentProcessId();

		MappedDataHandle = CreateFileMappingW(
			INVALID_HANDLE_VALUE,
			nullptr,
			PAGE_READWRITE,
			0,
			sizeof(SharedTextureInfo),
			(SHM_NAME + std::to_wstring(ProcessId)).c_str()
		);

		if (MappedDataHandle == nullptr)
			return;

		Data = static_cast<SharedTextureInfo*>(MapViewOfFile(
			MappedDataHandle,
			FILE_MAP_ALL_ACCESS,
			0,
			0,
			sizeof(SharedTextureInfo)
		));

		if (Data == nullptr)
			return;

		Data->Version = 1;
	}

	~SharedTextureState()
	{
		if (Data)
		{
			UnmapViewOfFile(Data);
			Data = nullptr;
		}

		if (MappedDataHandle)
		{
			CloseHandle(MappedDataHandle);
			MappedDataHandle = nullptr;
		}
	}

	bool IsValid() const
	{
		return Data != nullptr;
	}

	HANDLE MappedDataHandle = nullptr;
	SharedTextureInfo* Data = nullptr;
};

static SharedTextureState ProtocolState;


bool SharedTextureProtocol::IsActive()
{
	return ProtocolState.IsValid() && ProtocolState.Data->bIsActive;
}

int SharedTextureProtocol::GetWidth()
{
	if (!ProtocolState.IsValid()) return 0;

	return ProtocolState.Data->Width;
}

int SharedTextureProtocol::GetHeight()
{
	if (!ProtocolState.IsValid()) return 0;

	return ProtocolState.Data->Height;
}

void SharedTextureProtocol::SubmitTexture(void* Texture)
{
	if (!IsActive()) return;

  ProtocolState.Data->VFlip         = 1;
  ProtocolState.Data->TextureHandle = (uint64_t)Texture;
	
	// Assuming you are passing in your own texture at the end of the render step,
	// copy the texture into another texture here, and submit that texture's handle
	// to the shared texture protocol.

	// When creating another "copy" texture for the compositor to consume, use
	// the following description:
	/*

	D3D11_TEXTURE2D_DESC TextureDescription = {};
	TextureDescription.Width = GetWidth();
	TextureDescription.Height = GetHeight();
	TextureDescription.MipLevels = 1;
	TextureDescription.ArraySize = 1;
	TextureDescription.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	TextureDescription.SampleDesc.Count = 1;
	TextureDescription.Usage = D3D11_USAGE_DEFAULT;
	TextureDescription.BindFlags = 0;
	TextureDescription.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

	*/
}

#endif
