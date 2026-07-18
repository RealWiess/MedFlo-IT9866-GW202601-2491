
#include "capture_config.h"
#include "capture_types.h"
#include "capture_util.h"

//=============================================================================
//                Constant Definition
//=============================================================================

//=============================================================================
//                Macro Definition
//=============================================================================

//=============================================================================
//                Structure Definition
//=============================================================================

//=============================================================================
//                Global Data Definition
//=============================================================================
//=============================================================================
//                Private Function Definition
//=============================================================================
/* Convert the floating-point number fVar to an unsigned integer */
static __inline uint32_t sisftol(float fVar) { return (uint32_t)fVar; }
#if 0
/* Calculate the square root of fVar and return it as a floating-point number */
static __inline float sissqrt(float fVar)
{
    if (fVar >= 0.0f)// Check if fVar is non-negative
    {
        return (float)(sqrtf(fVar)); // Calculate the square root of fVar and return it as a floating-point number
    }
    else
    {
        return 0.0f;
    }
}
#endif
/* Calculate the cosine of fVar and return it as a floating-point number */
static __inline float siscos(float fVar) {
  if ((fVar >= -31.0f) &&
      (fVar <= 31.0f)) // Check if fVar falls within the valid range [-31, 31]
  {
    return (float)(cosf(fVar)); // Calculate the cosine of fVar and return
                                // it as a floating-point number
  } else {
    return 0.0f;
  }
}
/* Calculate the sine of fVar and return it as a floating-point number */
static __inline float sissin(float fVar) {
  if ((fVar >= -31.0f) &&
      (fVar <= 31.0f)) // Check if fVar falls within the valid range [-31, 31]
  {
    return (float)(sinf(fVar)); // Calculate the sine of fVar and return it
                                // as a floating-point number
  } else {
    return 0.0f;
  }
}
//=============================================================================
//                Public Function Definition
//=============================================================================
//=============================================================================
/**
 * Convert standard float s[8].23 to float s[e].m
 *
 * @param  value
 * @param  e
 * @param  m
 * @return
 */
//=============================================================================
uint32_t cap_floattofix_(float value, uint32_t e, uint32_t m) {
  uint32_t dwValue = 0;
  uint32_t i = 0;

  if (value < 0.0f) {
    value *= -1.0f;
    for (i = 0; i < m; i++) {
      value *= 2.0f;
    }
    dwValue = sisftol(value);
    dwValue = (~dwValue + 1U);
  } else {
    for (i = 0; i < m; i++) {
      value *= 2.0f;
    }
    dwValue = sisftol(value);
  }

  return dwValue;
}

//=============================================================================
/**
 * Function for CAP scaling weighting matrix setting.
 *
 * @param
 * @return
 */
//=============================================================================
float cap_sinc_(float x) {
  float pi = 3.14159265358979f;

  if (x == 0.0f) {
    return 1.0f;
  } else {
    return (sissin(pi * x) / (pi * x));
  }
}

//=============================================================================
/**
 * Function for CAP scaling weighting matrix setting.
 *
 * @param
 * @return
 */
//=============================================================================
float cap_rcos_(float x) {
  float pi = 3.14159265358979f;
  float r = 0.5f;

  if (x == 0.0f) {
    return 1.0f;
  } else if ((x == (-1.0f / (2.0f * r))) || (x == (1.0f / (2.0f * r)))) {
    return (r / 2.0f * sissin(pi / (2.0f * r)));
  } else {
    return (sissin(pi * x) / (pi * x) * siscos(r * pi * x) /
            (1.0f - (4.0f * (r * r) * (x * x))));
  }
}

//=============================================================================
/**
 * Catmull-Rom Cubic interpolation.
 * Define the function capCubic01 to calculate the value of a cubic equation
 * within the range [-2, 2]
 * @param float x
 * @return float result
 */
//=============================================================================
float cap_cubic01_(float x) {
  float result = 0.0f;
  float abs_x = fabsf(x);
  /*If the absolute value of x is less than or equal to 1, execute the
   * following code block*/
  if (abs_x <= 1.0f) {
    result = (1.5f * (abs_x * abs_x * abs_x)) - (2.5f * abs_x * abs_x) + 1.0f;
  } else if (abs_x <= 2.0f) {
    /*If the absolute value of x is greater than 1 and less than
                  or equal to 2, execute the following code block*/
    result = (-0.5f * (abs_x * abs_x * abs_x)) + (2.5f * abs_x * abs_x) -
             (4.0f * abs_x) + 2.0f;
  } else {
    (void)printf("abs_x is over 2.0f");
  }

  return result;
}
