#ifdef _WIN32
#include <Windows.h>
//Force GPU drivers on systems with gpu power management features that switches between integrated and dedicated graphics to run on dedicated GPU
extern "C" _declspec(dllexport) DWORD NvOptimusEnablement                  = 0x00000001;
extern "C" _declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;
#endif
