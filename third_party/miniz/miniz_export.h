#ifndef MINIZ_EXPORT_H
#define MINIZ_EXPORT_H

#ifdef MINIZ_DLL
#ifdef _WIN32
#ifdef MINIZ_EXPORTS
#define MINIZ_EXPORT __declspec(dllexport)
#else
#define MINIZ_EXPORT __declspec(dllimport)
#endif
#else
#define MINIZ_EXPORT __attribute__((visibility("default")))
#endif
#else
#define MINIZ_EXPORT
#endif

#endif
