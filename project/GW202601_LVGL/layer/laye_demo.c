#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "scene.h"
#include "ctrlboard.h"
#include "ite/itp.h"

/* widgets:
demoLayer
fakeBackground
TextBox1
*/

bool DemoOnEnter(ITUWidget* widget, char* param)
{
    printf(" >>> %s\n", __func__);//Add by Hardy for log
    return true;
}

bool DemoOnTimer(ITUWidget* widget, char* param)
{
    return true;
}

