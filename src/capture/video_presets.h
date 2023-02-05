/*
 * 2020-2021 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */


#ifndef VCS_CAPTURE_VIDEO_PRESETS_H
#define VCS_CAPTURE_VIDEO_PRESETS_H

#include "common/globals.h"
#include "common/refresh_rate.h"
#include "capture/capture.h"
#include "common/propagate/vcs_event.h"

struct video_preset_s
{
    enum class refresh_rate_comparison_e
    {
        equals,
        floored,
        rounded,
        ceiled,
    };

    video_signal_parameters_s videoParameters = {};

    // Internal identifier for this particular video preset. Not user-facing. Used
    // to tell presets apart.
    unsigned id = 0;

    // A user-facing, user-created string describing what this video preset is
    // for (e.g. "Quake on Windows 95").
    std::string name = "";

    bool activatesWithResolution = false;
    resolution_s activationResolution = {0, 0, 32};

    bool activatesWithRefreshRate = false;
    refresh_rate_s activationRefreshRate = 0;
    refresh_rate_comparison_e refreshRateComparator = refresh_rate_comparison_e::equals;

    // Only handled by the GUI.
    bool activatesWithShortcut = false;
    std::string activationShortcut = "Ctrl+F1";

    // Returns the strength of activation, expressed as an integer, of this preset
    // to the given capture conditions (resolution, refresh rate, etc.). The activation
    // level depends on the number of conditions that have been specified for the
    // preset - e.g. if it's specified to require a certain resolution and a certain
    // refresh rate, its activation level will be 2 in case both of those conditions
    // are satified, and 0 otherwise (i.e. if none or just one condition is satisfied).
    int activation_level(const resolution_s resolution,
                         const refresh_rate_s refreshRate)
    {
        int activationLevel = 0;
        int requiredActivationLevel = 0;

        if (this->activatesWithResolution)
        {
            requiredActivationLevel++;

            if ((resolution.w == this->activationResolution.w) &&
                (resolution.h == this->activationResolution.h))
            {
                activationLevel++;
            }
        }

        if (this->activatesWithRefreshRate)
        {
            requiredActivationLevel++;

            refresh_rate_s referenceRate;

            switch (this->refreshRateComparator)
            {
                case refresh_rate_comparison_e::equals:  referenceRate = refreshRate; break;
                case refresh_rate_comparison_e::floored: referenceRate = floor(refreshRate.value<double>()); break;
                case refresh_rate_comparison_e::rounded: referenceRate = round(refreshRate.value<double>()); break;
                case refresh_rate_comparison_e::ceiled:  referenceRate = ceil(refreshRate.value<double>()); break;
                default: break;
            }

            if (this->activationRefreshRate == referenceRate)
            {
                activationLevel++;
            }
        }

        if ((requiredActivationLevel <= 0) || // If this preset has no activation conditions.
            (activationLevel < requiredActivationLevel))
        {
            return false;
        }
        else
        {
            return activationLevel;
        }
    }

    // Whether this preset should be activated by the given shortcut (e.g.
    // "ctrl+f1").
    bool activates_with_shortcut(std::string shortcutString)
    {
        for (auto &chr: shortcutString)
        {
            chr = char(std::tolower(chr));
        }

        for (auto &chr: this->activationShortcut)
        {
            chr = char(std::tolower(chr));
        }

        return (this->activatesWithShortcut && (this->activationShortcut == shortcutString));
    }
};

// Should be fired whenever changes have been made to the given video preset's
// parameters.
//
// Warning: If you change the preset's activation conditions, you should call
// kvideopreset_apply_current_active_preset() instead.
extern vcs_event_c<const video_preset_s*> kc_evVideoPresetParamsChanged;

bool kvideopreset_is_preset_active(const video_preset_s *const preset);

void kvideopreset_initialize(void);

void kvideopreset_remove_all_presets(void);

void kvideopreset_assign_presets(const std::vector<video_preset_s*> &presets);

void kvideopreset_activate_keyboard_shortcut(const std::string &shortcutString);

const std::vector<video_preset_s*>& kvideopreset_all_presets(void);

void kvideopreset_apply_current_active_preset(void);

video_preset_s* kvideopreset_get_preset_ptr(const unsigned presetId);

// If 'duplicateDataSrc' is a non-null pointer to an existing video preset
// object, its parameter values will be copied into the created preset.
video_preset_s* kvideopreset_create_new_preset(const video_preset_s *const duplicateDataSrc = nullptr);

bool kvideopreset_remove_preset(const unsigned presetId);

video_signal_parameters_s kvideopreset_current_video_parameters(void);

#endif
