#ifndef BOOTLOG_H
#define BOOTLOG_H

#include "../const/define.h"

#if VUESP_LOG_LEVEL >= 1
void bootLog();
#else
inline void bootLog() {}
#endif

#endif
