/*
 * 2019 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 * 
 */

/*! @file
 *
 * @brief
 * The disk subsystem interface.
 * 
 * The disk subsystem provides functionality to VCS for saving and loading user
 * data files -- e.g. video preset configurations.
 * 
 * The subsystem transparently handles loading from legacy file formats. Saving
 * is always done using the current file format for the given type of data.
 */

#ifndef VCS_COMMON_DISK_DISK_H
#define VCS_COMMON_DISK_DISK_H

#include <vector>
#include <string>
#include "common/propagate/vcs_event.h"
#include "display/display.h"

class BaseFilterGraphNode;

struct filter_graph_option_s;
struct video_signal_parameters_s;
struct video_preset_s;
struct resolution_alias_s;

/*!
 * Saves the given video presets into a file named @p filename.
 * 
 * Returns true on success, false otherwise.
 * 
 * @see
 * kdisk_load_video_presets()
 */
bool kdisk_save_video_presets(const std::vector<video_preset_s*> &presets,
                              const std::string &filename);

/*!
 * Saves the given filter graph nodes into a file named @p filename.
 * 
 * Returns true on success, false otherwise.
 * 
 * @see
 * kdisk_load_filter_graph()
 */
bool kdisk_save_filter_graph(const std::vector<BaseFilterGraphNode*> &nodes,
                             const std::string &filename);

/*!
 * Loads the video presets stored in the file named @p filename.
 * 
 * Returns the loaded video presets. If the loading fails, returns an empty vector.
 * 
 * @see
 * kdisk_save_video_presets()
 */
std::vector<video_preset_s*> kdisk_load_video_presets(const std::string &filename);

/*!
 * Loads the filter graph nodes stored in the file named @p filename.
 * 
 * Returns the loaded filter graph nodes. If the loading fails, returns an empty vector.
 * 
 * @see
 * kdisk_save_filter_graph()
 */
std::vector<abstract_filter_graph_node_s> kdisk_load_filter_graph(const std::string &filename);

#endif
