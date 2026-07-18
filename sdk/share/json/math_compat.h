#ifndef __math_compat_h
#define __math_compat_h

/**
 * @file
 * @brief Do not use, json-c internal, may be changed or removed at any time.
 */

/* Define isnan, isinf, infinity and nan on Windows/MSVC */

#ifndef HAVE_DECL_ISNAN
# ifdef HAVE_DECL__ISNAN
#include <float.h>
//#define isnan(x) _isnan(x)
# endif
#endif

#ifndef HAVE_DECL_ISINF
#ifdef HAVE_DECL__FINITE
#include <float.h>
//#define isinf(x) (!_finite(x))
#endif
#endif

#ifndef INFINITY
#include <float.h>
#ifndef INFINITY
#define INFINITY (DBL_MAX + DBL_MAX)
#endif
#endif

#ifndef NAN
#define NAN (INFINITY - INFINITY)
#endif

#endif
