/*
 * 2018 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 * Handles alias resolutions.
 * 
 * An alias resolution is an alternate resolution for some other resolution.
 * For instance, an alias for 600 x 400 might be 640 x 480, where you have
 * 600 x 400 as the source resolution, and 640 x 480 as the target (alias)
 * resolution.
 * 
 * Alias resolutions in VCS solve the problem of the capture device at times
 * initializing an incorrect resolution for a given capture signal. The user
 * can define aliases for these incorrect resolutions; VCS will then detect
 * when the capture device sets a resolution for which there is an alias, and
 * tell the device to adopt the alias resolution instead.
 * 
 * 
 * Usage:
 * 
 *   1. On initialization, call ka_set_aliases() to establish a list of alias
 *      resolutions.
 * 
 *   2. When a new capture video mode is introduced, call ka_has_alias() to
 *      find out if it has an alias. If it does, call ka_aliased() to retrieve
 *      the alias resolution.
 *
 */

#ifndef VCS_CAPTURE_ALIAS_H
#define VCS_CAPTURE_ALIAS_H

#include "display/display.h"

struct resolution_alias_s
{
    resolution_s from;
    resolution_s to;
};

// If the given resolution has an alias, returns the corresponding
// alias resolution; otherwise, returns the resolution that was passed
// in.
resolution_s ka_aliased(const resolution_s &r);

// Returns true if the given resolution has an alias; false otherwise.
bool ka_has_alias(const resolution_s &r);

// Returns all aliases we know of.
const std::vector<resolution_alias_s>& ka_aliases(void);

// Assign the list of aliases we'll operate on.
void ka_set_aliases(const std::vector<resolution_alias_s> &aliases);

#endif
