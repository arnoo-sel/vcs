/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/denoise_nonlocal_means/filter_denoise_nonlocal_means.h"
#include "filter/filters/denoise_nonlocal_means/gui/filtergui_denoise_nonlocal_means.h"

filtergui_denoise_nonlocal_means_c::filtergui_denoise_nonlocal_means_c(abstract_filter_c *const filter)
{
    {
        auto *const luminance = new filtergui_spinbox_s;

        luminance->get_value = [=]{return filter->parameter(filter_denoise_nonlocal_means_c::PARAM_H);};
        luminance->set_value = [=](const int value){filter->set_parameter(filter_denoise_nonlocal_means_c::PARAM_H, value);};
        luminance->minValue = 0;
        luminance->maxValue = 255;

        this->guiFields.push_back({"Luminance", {luminance}});
    }

    {
        auto *const color = new filtergui_spinbox_s;

        color->get_value = [=]{return filter->parameter(filter_denoise_nonlocal_means_c::PARAM_H_COLOR);};
        color->set_value = [=](const int value){filter->set_parameter(filter_denoise_nonlocal_means_c::PARAM_H_COLOR, value);};
        color->minValue = 0;
        color->maxValue = 255;

        this->guiFields.push_back({"Color", {color}});
    }

    {
        auto *const templateWindow = new filtergui_spinbox_s;

        templateWindow->get_value = [=]{return filter->parameter(filter_denoise_nonlocal_means_c::PARAM_TEMPLATE_WINDOW_SIZE);};
        templateWindow->set_value = [=](const int value){filter->set_parameter(filter_denoise_nonlocal_means_c::PARAM_TEMPLATE_WINDOW_SIZE, value);};
        templateWindow->minValue = 0;
        templateWindow->maxValue = 255;

        this->guiFields.push_back({"Template wnd.", {templateWindow}});
    }

    {
        auto *const searchWindow = new filtergui_spinbox_s;

        searchWindow->get_value = [=]{return filter->parameter(filter_denoise_nonlocal_means_c::PARAM_SEARCH_WINDOW_SIZE);};
        searchWindow->set_value = [=](const int value){filter->set_parameter(filter_denoise_nonlocal_means_c::PARAM_SEARCH_WINDOW_SIZE, value);};
        searchWindow->minValue = 0;
        searchWindow->maxValue = 255;

        this->guiFields.push_back({"Search wnd.", {searchWindow}});
    }
    
    return;
}
