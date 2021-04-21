/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_COLOR_DEPTH_FILTER_COLOR_DEPTH_H
#define VCS_FILTER_FILTERS_COLOR_DEPTH_FILTER_COLOR_DEPTH_H

#include "filter/filter.h"
#include "filter/filters/color_depth/gui/filtergui_color_depth.h"

class filter_color_depth_c : public filter_c
{
public:
    enum { PARAM_BIT_COUNT_RED,
           PARAM_BIT_COUNT_GREEN,
           PARAM_BIT_COUNT_BLUE };
    
    filter_color_depth_c(FILTER_CTOR_FUNCTION_PARAMS) :
        filter_c({{PARAM_BIT_COUNT_RED, 8},
                  {PARAM_BIT_COUNT_GREEN, 8},
                  {PARAM_BIT_COUNT_BLUE, 8}},
                 initialParameterValues)
    {
        this->guiDescription = new filtergui_color_depth_c(this);
    }

    CLONABLE_FILTER_TYPE(filter_color_depth_c)

    void apply(FILTER_APPLY_FUNCTION_PARAMS) override;

    std::string name(void) const override { return "Color depth"; }
    std::string uuid(void) const override { return "c87f6967-b82d-4a10-b74f-9923f5ed00f8"; }
    filter_category_e category(void) const override { return filter_category_e::reduce; }

private:
};

#endif