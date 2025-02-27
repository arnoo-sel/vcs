/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_FILTERGUI_H
#define VCS_FILTER_FILTERS_FILTERGUI_H

#include <functional>
#include <cstring>
#include <string>
#include <vector>
#include "common/types.h"

class abstract_filter_c;

enum class filtergui_alignment_e
{
    left,
    right,
    center
};

// The possible types of GUI components.
enum class filtergui_component_e
{
    label,
    spinbox,
    doublespinbox,
    combobox,
    checkbox,
    textedit,
};

struct filtergui_component_s
{
    virtual ~filtergui_component_s(void){}

    virtual filtergui_component_e type(void) const = 0;

    std::function<void(const bool)> set_enabled;
    bool isInitiallyEnabled = true;
};

struct filtergui_label_s : public filtergui_component_s
{
    filtergui_component_e type(void) const override { return filtergui_component_e::label; }

    std::function<void(const std::string&)> set_text;
    std::function<std::string(void)> get_text;

    std::string text;
};

struct filtergui_textedit_s : public filtergui_component_s
{
    filtergui_component_e type(void) const override { return filtergui_component_e::textedit; }

    std::function<void(const std::string&)> set_text;
    std::function<std::string(void)> get_text;

    std::string text;
    std::size_t maxLength = 255;
};

struct filtergui_checkbox_s : public filtergui_component_s
{
    filtergui_component_e type(void) const override { return filtergui_component_e::checkbox; }

    std::function<void(const bool)> set_value;
    std::function<bool(void)> get_value;

    std::string label;
};

struct filtergui_spinbox_s : public filtergui_component_s
{
    filtergui_component_e type(void) const override { return filtergui_component_e::spinbox; }

    std::function<void(const int)> set_value;
    std::function<int(void)> get_value;

    int maxValue = 0;
    int minValue = 0;
    std::string prefix = "";
    std::string suffix = "";
    filtergui_alignment_e alignment = filtergui_alignment_e::left;
};

struct filtergui_doublespinbox_s : public filtergui_component_s
{
    filtergui_component_e type(void) const override { return filtergui_component_e::doublespinbox; }

    std::function<void(const double)> set_value;
    std::function<double(void)> get_value;

    double maxValue = 0;
    double minValue = 0;
    int numDecimals = 1;
    double stepSize = 1;
    std::string prefix = "";
    std::string suffix = "";
    filtergui_alignment_e alignment = filtergui_alignment_e::left;
};

struct filtergui_combobox_s : public filtergui_component_s
{
    filtergui_component_e type(void) const override { return filtergui_component_e::combobox; }

    std::function<void(const int)> set_value;
    std::function<int(void)> get_value;

    std::vector<std::string> items;
};

// A GUI field is a combination of a label (which provides a user-facing field name)
// and one or more components for modifying related filter parameters. For instance,
// a field for a blur filter might be "Radius [x]", where "Radius" is the label and
// [x] is a spin box component that lets the user change the value of the filter's
// radius parameter.
struct filtergui_field_s
{
    const std::string label = "";
    const std::vector<filtergui_component_s*> components;

    filtergui_field_s(const std::string label,
                      const std::vector<filtergui_component_s*> components) :
        label(label),
        components(components)
    {
        return;
    }
};

// A description of the GUI elements required by a given filter for modifying
// its parameters. VCS's chosen GUI framework will interpret the descriptions
// to create the actual GUI.
class filtergui_c
{
    friend class abstract_filter_c;

public:
    virtual ~filtergui_c(void);

protected:
    std::vector<filtergui_field_s> guiFields;
};  

#endif
