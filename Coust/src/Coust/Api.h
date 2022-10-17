#pragma once

#ifdef COUST_DYNAMIC_BUILD
#define COUST_API __declspec(dllexport)
#else
#define COUST_API __declspec(dllimport)
#endif