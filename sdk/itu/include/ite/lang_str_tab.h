#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#ifndef ITU_LS_T
#define ITU_LS_T

#define ITU_LANGUAGE_STRING_TSIZE   50      ///< The maximum language table count.
#define ITU_LANGUAGE_STRING_ISIZE   1000    ///< The maximum item count for each language table.

extern const wchar_t* ITULanguage_String_Table[ITU_LANGUAGE_STRING_TSIZE][ITU_LANGUAGE_STRING_TSIZE];

// You could edit string table in lang_str_tab.c by yourself

#endif