/*
 * 2022 Tarpeeksi Hyvae Soft
 *
 */

#ifndef VCS_FILTER_FILTERS_RENDER_TEXT_FONT_10X6_SANS_SERIF_H
#define VCS_FILTER_FILTERS_RENDER_TEXT_FONT_10X6_SANS_SERIF_H

#include <unordered_map>
#include <vector>
#include "filter/filters/render_text/font.h"

// A retro-looking 10 x 6 sans serif font.
class font_10x6_sans_serif_c : public font_c
{
public:
    unsigned cap_height() const override
    {
        return 10;
    }

    unsigned letter_spacing() const override
    {
        return 1;
    }

    unsigned line_spacing() const override
    {
        return (this->letter_spacing() * 2);
    }

    const font_glyph_s& glyph_at(const char key) const override
    {
        return (
            (this->charset.find(key) == this->charset.end())
            ? this->charset.at('?')
            : this->charset.at(key)
        );
    }

private:
    #define X 1
    const std::unordered_map<char, font_glyph_s> charset = {
        {'?', {{{0,X,X,X,0,0},
                {X,0,0,0,X,0},
                {0,0,0,0,0,X},
                {0,0,0,0,0,X},
                {0,0,0,0,X,0},
                {0,0,0,X,0,0},
                {0,0,X,0,0,0},
                {0,0,X,0,0,0},
                {0,0,0,0,0,0},
                {0,0,X,0,0,0}}}},

        {'!', {{{X},
                {X},
                {X},
                {X},
                {X},
                {X},
                {X},
                {X},
                {0},
                {X}}}},

        {' ', {{{0,0}}}},

        {'.', {{{X}}}},

        {'\'', {{{X},
                 {X},
                 {X}}, 7}},

        {'\"', {{{X,0,X},
                 {X,0,X},
                 {X,0,X}}, 7}},

        {',', {{{0,X},
                {0,X},
                {X,0}}, -2}},

        {':', {{{X},
                {0},
                {0},
                {0},
                {0},
                {X}}}},

        {';', {{{0,X},
                {0,0},
                {0,0},
                {0,0},
                {0,0},
                {0,X},
                {0,X},
                {X,0}}, -2}},

        {'/', {{{0,0,0,0,0,0,0,X},
                {0,0,0,0,0,0,X,0},
                {0,0,0,0,0,X,0,0},
                {0,0,0,0,X,0,0,0},
                {0,0,0,X,0,0,0,0},
                {0,0,X,0,0,0,0,0},
                {0,X,0,0,0,0,0,0},
                {X,0,0,0,0,0,0,0}}, 1}},

        {'~', {{{0,X,X,0,0,X},
                {X,0,0,X,X,0}}, 4}},

        {'=', {{{X,X,X,X,X},
                {0,0,0,0,0},
                {X,X,X,X,X}}, 3}},

        {'-', {{{X,X,X,X,X}}, 4}},

        {'+', {{{0,0,X,0,0},
                {0,0,X,0,0},
                {X,X,X,X,X},
                {0,0,X,0,0},
                {0,0,X,0,0}}, 2}},

        {'*', {{{X,0,0,0,X},
                {0,X,0,X,0},
                {0,0,X,0,0},
                {0,X,0,X,0},
                {X,0,0,0,X}}, 2}},

        {'[', {{{X,X,X},
                {X,0,0},
                {X,0,0},
                {X,0,0},
                {X,0,0},
                {X,0,0},
                {X,0,0},
                {X,0,0},
                {X,0,0},
                {X,X,X}}}},

        {']', {{{X,X,X},
                {0,0,X},
                {0,0,X},
                {0,0,X},
                {0,0,X},
                {0,0,X},
                {0,0,X},
                {0,0,X},
                {0,0,X},
                {X,X,X}}}},

        {'(', {{{0,0,X},
                {0,X,0},
                {X,0,0},
                {X,0,0},
                {X,0,0},
                {X,0,0},
                {X,0,0},
                {X,0,0},
                {0,X,0},
                {0,0,X}}}},

        {')', {{{X,0,0},
                {0,X,0},
                {0,0,X},
                {0,0,X},
                {0,0,X},
                {0,0,X},
                {0,0,X},
                {0,0,X},
                {0,X,0},
                {X,0,0}}}},

        {'A', {{{0,X,X,X,X,0},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,X,X,X,X,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X}}}},

        {'a', {{{0,X,X,X,0},
                {X,0,0,0,X},
                {0,0,0,0,X},
                {0,X,X,X,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {0,X,X,X,X}}}},

        {'B', {{{X,X,X,X,X,0},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,X,X,X,X,0},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,X,X,X,X,0}}}},

        {'b', {{{X,0,0,0,0},
                {X,0,0,0,0},
                {X,0,0,0,0},
                {X,X,X,X,0},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,X,X,X,0}}}},

        {'C', {{{0,0,X,X,X,0},
                {0,X,0,0,0,X},
                {X,0,0,0,0,0},
                {X,0,0,0,0,0},
                {X,0,0,0,0,0},
                {X,0,0,0,0,0},
                {X,0,0,0,0,0},
                {X,0,0,0,0,0},
                {0,X,0,0,0,X},
                {0,0,X,X,X,0}}}},

        {'c', {{{0,X,X,X,0},
                {X,0,0,0,X},
                {X,0,0,0,0},
                {X,0,0,0,0},
                {X,0,0,0,0},
                {X,0,0,0,X},
                {0,X,X,X,0}}}},

        {'D', {{{X,X,X,X,0,0},
                {X,0,0,0,X,0},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,X,0},
                {X,X,X,X,0,0}}}},

        {'d', {{{0,0,0,0,X},
                {0,0,0,0,X},
                {0,0,0,0,X},
                {0,X,X,X,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {0,X,X,X,X}}}},

        {'E', {{{X,X,X,X,X,X},
                {X,0,0,0,0,0},
                {X,0,0,0,0,0},
                {X,0,0,0,0,0},
                {X,X,X,X,0,0},
                {X,0,0,0,0,0},
                {X,0,0,0,0,0},
                {X,0,0,0,0,0},
                {X,0,0,0,0,0},
                {X,X,X,X,X,X}}}},

        {'e', {{{0,X,X,X,0},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,X,X,X,0},
                {X,0,0,0,0},
                {X,0,0,0,X},
                {0,X,X,X,0}}}},

        {'F', {{{X,X,X,X,X,X},
                {X,0,0,0,0,0},
                {X,0,0,0,0,0},
                {X,0,0,0,0,0},
                {X,X,X,X,X,0},
                {X,0,0,0,0,0},
                {X,0,0,0,0,0},
                {X,0,0,0,0,0},
                {X,0,0,0,0,0},
                {X,0,0,0,0,0}}}},

        {'f', {{{0,X,X},
                {X,0,0},
                {X,0,0},
                {X,X,X},
                {X,0,0},
                {X,0,0},
                {X,0,0},
                {X,0,0},
                {X,0,0},
                {X,0,0}}}},

        {'G', {{{0,0,X,X,X,0},
                {0,X,0,0,0,X},
                {X,0,0,0,0,0},
                {X,0,0,0,0,0},
                {X,0,0,0,0,0},
                {X,0,0,X,X,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {0,X,0,0,0,X},
                {0,0,X,X,X,0}}}},

        {'g', {{{0,X,X,X,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {0,X,X,X,X},
                {0,0,0,0,X},
                {0,X,X,X,0}},-2}},

        {'H', {{{X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,X,X,X,X,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X}}}},

        {'h', {{{X,0,0,0,0},
                {X,0,0,0,0},
                {X,0,0,0,0},
                {X,X,X,X,0},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X}}}},

        {'I', {{{X},
                {X},
                {X},
                {X},
                {X},
                {X},
                {X},
                {X},
                {X},
                {X}}}},

        {'i', {{{X},
                {0},
                {0},
                {X},
                {X},
                {X},
                {X},
                {X},
                {X},
                {X}}}},

        {'J', {{{0,0,0,0,0,X},
                {0,0,0,0,0,X},
                {0,0,0,0,0,X},
                {0,0,0,0,0,X},
                {0,0,0,0,0,X},
                {0,0,0,0,0,X},
                {0,0,0,0,0,X},
                {0,0,0,0,0,X},
                {X,0,0,0,0,X},
                {0,X,X,X,X,0}}}},

        {'j', {{{0,X},
                {0,0},
                {0,0},
                {0,X},
                {0,X},
                {0,X},
                {0,X},
                {0,X},
                {0,X},
                {0,X},
                {0,X},
                {X,0}},-2}},

        {'K', {{{X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,X,0},
                {X,X,X,X,0,0},
                {X,0,0,0,X,0},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X}}}},

        {'k', {{{X,0,0,0,0},
                {X,0,0,0,0},
                {X,0,0,0,0},
                {X,0,0,0,X},
                {X,0,0,X,0},
                {X,0,X,0,0},
                {X,X,X,0,0},
                {X,0,0,X,0},
                {X,0,0,0,X},
                {X,0,0,0,X}}}},

        {'L', {{{X,0,0,0,0,0},
                {X,0,0,0,0,0},
                {X,0,0,0,0,0},
                {X,0,0,0,0,0},
                {X,0,0,0,0,0},
                {X,0,0,0,0,0},
                {X,0,0,0,0,0},
                {X,0,0,0,0,0},
                {X,0,0,0,0,0},
                {X,X,X,X,X,X}}}},

        {'l', {{{X},
                {X},
                {X},
                {X},
                {X},
                {X},
                {X},
                {X},
                {X},
                {X}}}},

        {'M', {{{X,0,0,0,0,0,0,0,X},
                {X,X,0,0,0,0,0,X,X},
                {X,0,X,0,0,0,X,0,X},
                {X,0,0,X,0,X,0,0,X},
                {X,0,0,0,X,0,0,0,X},
                {X,0,0,0,0,0,0,0,X},
                {X,0,0,0,0,0,0,0,X},
                {X,0,0,0,0,0,0,0,X},
                {X,0,0,0,0,0,0,0,X},
                {X,0,0,0,0,0,0,0,X}}}},

        {'m', {{{X,X,X,0,X,X,0},
                {X,0,0,X,0,0,X},
                {X,0,0,X,0,0,X},
                {X,0,0,X,0,0,X},
                {X,0,0,X,0,0,X},
                {X,0,0,X,0,0,X},
                {X,0,0,X,0,0,X}}}},

        {'N', {{{X,0,0,0,0,0,X},
                {X,X,0,0,0,0,X},
                {X,0,X,0,0,0,X},
                {X,0,0,X,0,0,X},
                {X,0,0,0,X,0,X},
                {X,0,0,0,0,X,X},
                {X,0,0,0,0,0,X},
                {X,0,0,0,0,0,X},
                {X,0,0,0,0,0,X},
                {X,0,0,0,0,0,X}}}},

        {'n', {{{X,X,X,X,0},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X}}}},

        {'O', {{{0,X,X,X,X,0},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {0,X,X,X,X,0}}}},

        {'o', {{{0,X,X,X,0},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {0,X,X,X,0}}}},

        {'P', {{{X,X,X,X,X,0},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,X,X,X,X,0},
                {X,0,0,0,0,0},
                {X,0,0,0,0,0},
                {X,0,0,0,0,0},
                {X,0,0,0,0,0},
                {X,0,0,0,0,0}}}},

        {'p', {{{X,X,X,X,0},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,X,X,X,0},
                {X,0,0,0,0},
                {X,0,0,0,0}},-2}},

        {'Q', {{{0,X,X,X,X,0},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,X,0,X},
                {X,0,0,0,X,0},
                {0,X,X,X,0,X}}}},

        {'q', {{{0,X,X,X,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {0,X,X,X,X},
                {0,0,0,0,X},
                {0,0,0,0,X}},-2}},

        {'R', {{{X,X,X,X,X,0},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,X,X,X,X,0},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X}}}},

        {'r', {{{0,X,X,X},
                {X,0,0,0},
                {X,0,0,0},
                {X,0,0,0},
                {X,0,0,0},
                {X,0,0,0},
                {X,0,0,0}}}},

        {'S', {{{0,X,X,X,X,0},
                {X,0,0,0,0,X},
                {X,0,0,0,0,0},
                {0,X,0,0,0,0},
                {0,0,X,X,0,0},
                {0,0,0,0,X,0},
                {0,0,0,0,0,X},
                {0,0,0,0,0,X},
                {X,0,0,0,0,X},
                {0,X,X,X,X,0}}}},

        {'s', {{{0,X,X,0},
                {X,0,0,X},
                {X,0,0,0},
                {0,X,X,0},
                {0,0,0,X},
                {X,0,0,X},
                {0,X,X,0}}}},

        {'T', {{{X,X,X,X,X,X,X},
                {0,0,0,X,0,0,0},
                {0,0,0,X,0,0,0},
                {0,0,0,X,0,0,0},
                {0,0,0,X,0,0,0},
                {0,0,0,X,0,0,0},
                {0,0,0,X,0,0,0},
                {0,0,0,X,0,0,0},
                {0,0,0,X,0,0,0},
                {0,0,0,X,0,0,0}}}},

        {'t', {{{X,0,0},
                {X,0,0},
                {X,0,0},
                {X,X,X},
                {X,0,0},
                {X,0,0},
                {X,0,0},
                {X,0,0},
                {X,0,0},
                {0,X,X}}}},

        {'U', {{{X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {0,X,X,X,X,0}}}},

        {'u', {{{X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {0,X,X,X,0}}}},

        {'V', {{{X,0,0,0,0,0,X},
                {X,0,0,0,0,0,X},
                {X,0,0,0,0,0,X},
                {0,X,0,0,0,X,0},
                {0,X,0,0,0,X,0},
                {0,X,0,0,0,X,0},
                {0,0,X,0,X,0,0},
                {0,0,X,0,X,0,0},
                {0,0,X,0,X,0,0},
                {0,0,0,X,0,0,0}}}},

        {'v', {{{X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {0,X,0,X,0},
                {0,X,0,X,0},
                {0,0,X,0,0}}}},

        {'W', {{{X,0,0,0,0,0,X},
                {X,0,0,0,0,0,X},
                {X,0,0,0,0,0,X},
                {X,0,0,0,0,0,X},
                {X,0,0,X,0,0,X},
                {X,0,0,X,0,0,X},
                {X,0,X,0,X,0,X},
                {X,0,X,0,X,0,X},
                {X,0,X,0,X,0,X},
                {0,X,0,0,0,X,0}}}},

        {'w', {{{X,0,0,X,0,0,X},
                {X,0,0,X,0,0,X},
                {X,0,0,X,0,0,X},
                {X,0,0,X,0,0,X},
                {X,0,0,X,0,0,X},
                {X,0,0,X,0,0,X},
                {X,X,X,0,X,X,0}}}},

        {'X', {{{X,0,0,0,0,0,X},
                {X,0,0,0,0,0,X},
                {0,X,0,0,0,X,0},
                {0,0,X,0,X,0,0},
                {0,0,0,X,0,0,0},
                {0,0,X,0,X,0,0},
                {0,X,0,0,0,X,0},
                {X,0,0,0,0,0,X},
                {X,0,0,0,0,0,X},
                {X,0,0,0,0,0,X}}}},

        {'x', {{{X,0,0,0,X},
                {X,0,0,0,X},
                {0,X,0,X,0},
                {0,0,X,0,0},
                {0,X,0,X,0},
                {X,0,0,0,X},
                {X,0,0,0,X}}}},

        {'Y', {{{X,0,0,0,0,0,X},
                {X,0,0,0,0,0,X},
                {X,0,0,0,0,0,X},
                {0,X,0,0,0,X,0},
                {0,0,X,X,X,0,0},
                {0,0,0,X,0,0,0},
                {0,0,0,X,0,0,0},
                {0,0,0,X,0,0,0},
                {0,0,0,X,0,0,0},
                {0,0,0,X,0,0,0}}}},

        {'y', {{{X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {0,X,X,X,X},
                {0,0,0,0,X},
                {0,X,X,X,0}},-2}},

        {'Z', {{{X,X,X,X,X,X},
                {0,0,0,0,0,X},
                {0,0,0,0,0,X},
                {0,0,0,0,X,0},
                {0,0,0,X,0,0},
                {0,0,X,0,0,0},
                {0,X,0,0,0,0},
                {X,0,0,0,0,0},
                {X,0,0,0,0,0},
                {X,X,X,X,X,X}}}},

        {'z', {{{X,X,X,X,X},
                {0,0,0,0,X},
                {0,0,0,X,0},
                {0,0,X,0,0},
                {0,X,0,0,0},
                {X,0,0,0,0},
                {X,X,X,X,X}}}},

        {'0', {{{0,X,X,X,X,0},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {X,0,0,0,0,X},
                {0,X,X,X,X,0}}}},

        {'1', {{{0,X},
                {X,X},
                {0,X},
                {0,X},
                {0,X},
                {0,X},
                {0,X},
                {0,X},
                {0,X},
                {0,X}}}},

        {'2', {{{0,X,X,X,0},
                {X,0,0,0,X},
                {0,0,0,0,X},
                {0,0,0,0,X},
                {0,0,0,X,0},
                {0,0,X,0,0},
                {0,X,0,0,0},
                {X,0,0,0,0},
                {X,0,0,0,0},
                {X,X,X,X,X}}}},

        {'3', {{{0,X,X,X,0},
                {X,0,0,0,X},
                {0,0,0,0,X},
                {0,0,0,0,X},
                {0,0,X,X,0},
                {0,0,0,0,X},
                {0,0,0,0,X},
                {0,0,0,0,X},
                {X,0,0,0,X},
                {0,X,X,X,0}}}},

        {'4', {{{0,0,0,0,X},
                {0,0,0,X,X},
                {0,0,X,0,X},
                {0,X,0,0,X},
                {X,0,0,0,X},
                {X,X,X,X,X},
                {0,0,0,0,X},
                {0,0,0,0,X},
                {0,0,0,0,X},
                {0,0,0,0,X}}}},

        {'5', {{{X,X,X,X,X},
                {X,0,0,0,0},
                {X,0,0,0,0},
                {X,0,0,0,0},
                {X,X,X,X,0},
                {0,0,0,0,X},
                {0,0,0,0,X},
                {0,0,0,0,X},
                {X,0,0,0,X},
                {0,X,X,X,0}}}},

        {'6', {{{0,X,X,X,0},
                {X,0,0,0,X},
                {X,0,0,0,0},
                {X,0,0,0,0},
                {X,X,X,X,0},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {0,X,X,X,0}}}},

        {'7', {{{X,X,X,X,X},
                {0,0,0,0,X},
                {0,0,0,0,X},
                {0,0,0,X,0},
                {0,0,X,0,0},
                {0,X,0,0,0},
                {0,X,0,0,0},
                {0,X,0,0,0},
                {0,X,0,0,0},
                {0,X,0,0,0}}}},

        {'8', {{{0,X,X,X,0},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {0,X,X,X,0},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {0,X,X,X,0}}}},

        {'9', {{{0,X,X,X,0},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {X,0,0,0,X},
                {0,X,X,X,X},
                {0,0,0,0,X},
                {0,0,0,0,X},
                {0,0,0,0,X},
                {X,0,0,0,X},
                {0,X,X,X,0}}}},
    };
    #undef X
};

#endif
