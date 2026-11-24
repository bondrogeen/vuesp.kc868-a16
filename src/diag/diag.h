#ifndef DIAG_H
#define DIAG_H

#include <stdint.h>

#ifndef VUESP_DIAGNOSTICS
#define VUESP_DIAGNOSTICS 1
#endif

/** Diagnostics stop collecting if no INFO request arrives within this window. */
#define DIAG_TIMEOUT_MS 60000UL

struct Info;

#if VUESP_DIAGNOSTICS
void diagInit();
void diagSetDiagnostics(uint8_t on);
void diagLoopBegin();
void diagLoopEnd();
void diagFillInfo(Info& info);
#else
inline void diagInit() {}
inline void diagSetDiagnostics(uint8_t) {}
inline void diagLoopBegin() {}
inline void diagLoopEnd() {}
inline void diagFillInfo(Info&) {}
#endif

#endif
