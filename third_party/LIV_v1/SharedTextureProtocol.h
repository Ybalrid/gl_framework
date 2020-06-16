#pragma once

class SharedTextureProtocol
{
public:
	static bool IsActive();

	static int GetWidth();
	static int GetHeight();

	static void SubmitTexture(void* Texture);
};
