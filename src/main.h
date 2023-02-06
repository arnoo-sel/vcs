/*
 * 2022 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_MAIN_H
#define VCS_MAIN_H

#include <vector>
#include <functional>
#include "common/propagate/vcs_event.h"

typedef std::function<void()> subsystem_releaser_t;

extern vcs_event_c<void> k_evEcoModeEnabled;
extern vcs_event_c<void> k_evEcoModeDisabled;

void k_set_eco_mode_enabled(const bool isEnabled);

bool k_is_eco_mode_enabled(void);

#endif
