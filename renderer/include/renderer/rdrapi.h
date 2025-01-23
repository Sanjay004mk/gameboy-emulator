#pragma once

// Define RDRAPI for any platform
#if defined _WIN32 || defined __CYGWIN__

#ifdef RDR_EXPORT
	// Exporting...
#ifdef __GNUC__
#define RDRAPI __attribute__((dllexport))
#else
#define RDRAPI __declspec(dllexport) 
#endif
#else
#ifdef __GNUC__
#define RDRAPI __attribute__((dllimport))
#else
#define RDRAPI __declspec(dllimport) 
#endif
#endif
#else
#if __GNUC__ >= 4
#define RDRAPI __attribute__((visibility("default")))
#else
#define RDRAPI
#endif
#endif

// converting to static lib
#undef RDRAPI
#define RDRAPI