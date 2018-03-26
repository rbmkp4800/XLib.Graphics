#include <Windows.h>

#include "XLib.Graphics.Internal.Shaders.h"

#include "..\Intermediate\Shaders\Color2DVS.cso.h"
#include "..\Intermediate\Shaders\ColorPS.cso.h"

using namespace XLib::Graphics::Internal;

ShaderData Shaders::Color2DVS = { Color2DVSData, sizeof(Color2DVSData) };
ShaderData Shaders::ColorPS = { ColorPSData, sizeof(ColorPSData) };