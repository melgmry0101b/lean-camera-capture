/*-----------------------------------------------------------------*\
 *
 * macros.h
 *   LeanCameraCapture
 *     lean-camera-capture
 *
 * MIT - see LICENSE at root directory
 *
 * CREATED: 2022-4-25 06:48 AM
 * AUTHORS: Mohammed Elghamry <elghamry.connect[at]outlook[dot]com>
 *
\*-----------------------------------------------------------------*/

#pragma once

#include "leancamercapture.h"

// Stringize a token
#define STRINGIZE(x) #x
// Stringize the evaluated value of a token (if it can be pre-processed)
#define STRINGIZE_EXPRESSION(x) STRINGIZE(x)

// Decorate and exception string with appending file and line.
#define MAKE_EX_STR(ex_str) (__SOURCE_FILENAME__ "(" STRINGIZE_EXPRESSION(__LINE__) "): " ex_str)

// Shorthand for dealing with exception string in an HRESULT context.
#define CHECK_FAILED_HR_WITH_GOTO_AND_EX_STR( hr, goto_label, ex_str_var, ex_str )  \
    if (FAILED(hr))                                                                 \
    {                                                                               \
        ex_str_var = MAKE_EX_STR(ex_str);                                           \
        goto goto_label;                                                            \
    }
