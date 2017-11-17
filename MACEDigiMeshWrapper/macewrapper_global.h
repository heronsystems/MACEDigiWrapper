#ifndef MACEWRAPPER_GLOBAL_H
#define MACEWRAPPER_GLOBAL_H

#ifdef _MSC_VER
#if defined(MACEDIGIMESHWRAPPER_LIBRARY)
#  define MACEWRAPPERSHARED_EXPORT __declspec(dllexport)
#else
#  define MACEWRAPPERSHARED_EXPORT __declspec(dllimport)
#endif
#else
#  define MACEWRAPPERSHARED_EXPORT
#endif

#endif // MACEWRAPPER_GLOBAL_H
