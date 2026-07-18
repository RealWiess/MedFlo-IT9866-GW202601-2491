#ifndef PKG_VERIFY_H
#define PKG_VERIFY_H

#include "ite/itc.h"

bool pkgRSAVerify(ITCStream *f);
bool pkgRSAVerify_APP(ITCStream *f);
#endif