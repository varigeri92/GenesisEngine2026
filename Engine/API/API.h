#pragma once
#ifdef BUILD_DLL
#define GNS_API __declspec(dllexport)
#else
#define GNS_API __declspec(dllimport)
#endif