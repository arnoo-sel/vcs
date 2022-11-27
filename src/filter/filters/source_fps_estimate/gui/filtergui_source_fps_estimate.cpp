/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/source_fps_estimate/filter_source_fps_estimate.h"
#include "filter/filters/source_fps_estimate/gui/filtergui_source_fps_estimate.h"

filtergui_source_fps_estimate_c::filtergui_source_fps_estimate_c(abstract_filter_c *const filter)
{
    {
        auto *threshold = new filtergui_spinbox_s;
        threshold->get_value = [=]{return filter->parameter(filter_frame_rate_c::PARAM_THRESHOLD);};
        threshold->set_value = [=](const double value){filter->set_parameter(filter_frame_rate_c::PARAM_THRESHOLD, value);};
        threshold->minValue = 0;
        threshold->maxValue = 255;

        this->guiFields.push_back({"Threshold", {threshold}});
    }

    {
        auto *corner = new filtergui_combobox_s;
        corner->get_value = [=]{return filter->parameter(filter_frame_rate_c::PARAM_CORNER);};
        corner->set_value = [=](const double value){filter->set_parameter(filter_frame_rate_c::PARAM_CORNER, value);};
        corner->items = {"Top left", "Top right", "Bottom right", "Bottom left"};

        this->guiFields.push_back({"Position", {corner}});
    }

    {
        auto *textColor = new filtergui_combobox_s;
        textColor->get_value = [=]{return filter->parameter(filter_frame_rate_c::PARAM_TEXT_COLOR);};
        textColor->set_value = [=](const double value){filter->set_parameter(filter_frame_rate_c::PARAM_TEXT_COLOR, value);};
        textColor->items = {"Yellow", "Purple", "Black", "White"};

        this->guiFields.push_back({"Color", {textColor}});
    }

    {
        auto *bgOpaque = new filtergui_checkbox_s;
        bgOpaque->get_value = [=]{return filter->parameter(filter_frame_rate_c::PARAM_BG_OPAQUE);};
        bgOpaque->set_value = [=](const double value){filter->set_parameter(filter_frame_rate_c::PARAM_BG_OPAQUE, value);};
        bgOpaque->label = "Opaque";

        this->guiFields.push_back({"Background", {bgOpaque}});
    }

    return;
}
