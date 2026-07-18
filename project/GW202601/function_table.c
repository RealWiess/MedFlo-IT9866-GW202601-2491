#include "ite/itu.h"

extern bool LogoOnEnter(ITUWidget* widget, char* param);
extern bool DemoOnEnter(ITUWidget* widget, char* param);
extern bool DemoOnTimer(ITUWidget* widget, char* param);

ITUActionFunction actionFunctions[] =
{
    "LogoOnEnter", LogoOnEnter,
    "DemoOnEnter", DemoOnEnter,
    "DemoOnTimer", DemoOnTimer,
    NULL, NULL
};
