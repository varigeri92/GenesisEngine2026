#pragma once
#if defined(_WIN32)
    #ifdef BUILD_DLL
        #define GNS_API __declspec(dllexport)
    #else
        #define GNS_API __declspec(dllimport)
    #endif
#elif defined(__GNUC__)
    #define GNS_API __attribute__((visibility("default")))
#else
    #define GNS_API
#endif
