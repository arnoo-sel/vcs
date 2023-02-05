/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

/*! @file
 *
 * @brief
 * The display subsystem interface.
 *
 * The VCS GUI is responsible for providing the user with real-time means to
 * enter information (e.g. configuring capture parameters), for directing such
 * input to VCS, and for displaying captured frames and other information about
 * VCS's run-time state to the user.
 *
 * The display interface decouples the GUI's implementation from the rest of
 * VCS, making it possible to replace the GUI with fairly minimal modification
 * to the rest of VCS.
 *
 * VCS's default GUI uses Qt, and its implementation of the display interface
 * can be found in @ref src/display/qt/d_main.cpp.
 *
 * @warning
 * As of VCS 2.0.0, this interface is undergoing refactoring. Many of its
 * functions are expected to become renamed or removed.
 */

#ifndef VCS_DISPLAY_DISPLAY_H
#define VCS_DISPLAY_DISPLAY_H

#include <string>
#include <vector>
#include <cassert>
#include <cstring>
#include "common/types.h"
#include "common/propagate/vcs_event.h"
#include "filter/filter.h"

struct log_entry_s;
struct resolution_alias_s;

/*!
 * An event that can be fired by subsystems to indicate that the output window should
 * redraw its contents.
 */
extern vcs_event_c<void> kd_evDirty;

/*!
 * @brief
 * A GUI-agnostic representation of a filter graph node.
 *
 * Used to mediate data between the disk subsystem's file loader and the display
 * subsystem's (GUI framework-dependent) filter graph implementation, so that the
 * file loader can remain independent from the GUI framework.
 */
struct abstract_filter_graph_node_s
{
    /*! Uniquely identifies this node from other nodes.*/
    int id = 0;

    /*! Whether this node is active (true) or a passthrough that applies no processing (false).*/
    bool isEnabled = true;

    /*! The UUID of the filter type that this node represents (see abstract_filter_c).*/
    std::string typeUuid = "";

    /*! The color of the node's background in the filter graph.*/
    std::string backgroundColor = "black";

    /*! The initial parameters of the filter associated with this node.*/
    filter_params_t initialParameters;

    /*! The node's initial XY coordinates in the filter graph.*/
    std::pair<double, double> initialPosition = {0, 0};

    /*! The nodes (identified by their @ref id) to which this node is connected.*/
    std::vector<int> connectedTo;
};

/*!
 * @brief
 * Describes the resolution of a pixel surface -- e.g. a program window or an image
 * buffer.
 */
struct resolution_s
{
    /*! Width (in pixels).*/
    unsigned long w;

    /*! Height (in pixels).*/
    unsigned long h;

    /*! Number of bits per pixel.*/
    unsigned long bpp;

    bool operator==(const resolution_s &other) const
    {
        return (
            (this->w == other.w) &&
            (this->h == other.h) &&
            (this->bpp == other.bpp)
        );
    }

    bool operator!=(const resolution_s &other) const
    {
        return !(*this == other);
    }
};

/*!
 * @brief
 * Generic container for the data and metadata of a 2D image.
 */
struct image_s
{
    uint8_t *const pixels;
    const resolution_s resolution;

    image_s(uint8_t *const pixels, const resolution_s &resolution) : pixels(pixels), resolution(resolution) {}

    std::size_t byte_size(void) const
    {
        return (this->resolution.w * this->resolution.h * (this->resolution.bpp / 8));
    }

    void operator=(const image_s &other)
    {
        assert((this->resolution == other.resolution) && "Attempting to copy images with mismatching resolutions.");
        std::memcpy(this->pixels, other.pixels, this->byte_size());
    }
};

/*!
 * @brief
 * Maps a value to a named filter graph property.
 */
struct filter_graph_option_s
{
    std::string propertyName;
    int value;

    filter_graph_option_s(const std::string propertyName, const int value)
    {
        this->propertyName = propertyName;
        this->value = value;

        return;
    }
};

/*!
 * Asks the GUI to create and open the output window. The output window is a
 * surface on which VCS's output frames are to be displayed by the GUI.
 *
 * If the GUI makes use of child windows and/or dialogs, this may be a good
 * time to create them, as well.
 *
 * It's expected that VCS will only call this function once, at program launch.
 *
 * @see
 * kd_spin_event_loop()
 */
void kd_acquire_output_window(void);

// Visit each node in the graph and while doing so, group together such chains of
// filters that run from an input gate through one or more filters into an output
// gate. The chains will then be submitted to the filter handler for use in applying
// the filters to captured frames.
/*!
 * Asks the GUI to inform the filter interface, @ref src/filter/filter.h, of
 * all filter chains currently configured in the GUI.
 *
 * This assumes that the GUI provides a dialog of some sort in which the user
 * can create and modify filter chains. When this function is called, the GUI
 * is expected to enumerate those filter chains into a format consumed by the
 * filter interface and then to submit them to the filter interface.
 *
 * In pseudocode, the GUI would do something like the following:
 *
 * @code
 * kf_unregister_all_filter_chains();
 *
 * for (chain: guiFilterChains)
 * {
 *     const standardFilterChain = ...convert chain into standard format...
 *     kf_register_filter_chain(standardFilterChain);
 * }
 * @endcode
 */
void kd_recalculate_filter_graph_chains(void);

/*!
 * Asks the GUI to load video presets from the given file.
 *
 * The GUI is expected to inform VCS's video presets subsystem of the new
 * presets.
 */
void kd_load_video_presets(const std::string &filename);

/*!
 * Asks the GUI to load a filter graph from the given file.
 *
 * The GUI is expected to inform VCS's filter subsystem of the new graph.
 */
void kd_load_filter_graph(const std::string &filename);

/*!
 * Asks the GUI to load alias resolutions from the given file.
 *
 * The GUI is expected to inform VCS's alias subsystem of the new aliases.
 */
void kd_load_aliases(const std::string &filename);

/*!
 * Asks the GUI to refresh the output window's title bar text.
 *
 * VCS assumes that the output window's title bar displays certain information
 * about VCS's state - e.g. the current capture resolution. VCS will thus call
 * this function to let the GUI know that the relevant state has changed.
 *
 * If your custom GUI implementation displays different state variables than
 * VCS's default GUI does, you may need to do custom polling of the relevant
 * state in order to be aware of changes to it.
 *
 * @see
 * kd_update_output_window_size()
 */
void kd_update_output_window_title(void);

/*!
 * Lets the GUI know that the size of output frames has changed. The GUI should
 * update the size of its output window accordingly.
 *
 * VCS expects the size of the output window to match the size of output
 * frames; although the GUI may choose not to honor this.
 *
 * The current size of output frames can be obtained via the scaler interface,
 * @ref src/scaler/scaler.h, by calling ks_output_resolution().
 *
 * The following sample Qt 5 code sizes the output window to match the size of
 * VCS's output frames (@a this is the output window's instance):
 *
 * @code
 * resolution_s r = ks_output_resolution();
 * this->setFixedSize(r.w, r.h);
 * @endcode
 *
 * @see
 * kd_update_output_window_title()
 */
void kd_update_output_window_size(void);

/*!
 * Asks the GUI to execute one spin of its event loop.
 *
 * Spinning the event loop would involve e.g. repainting the output window,
 * processing any user input, etc.
 *
 * The following sample Qt 5 code executes one spin of the event loop:
 *
 * @code
 * QCoreApplication::sendPostedEvents();
 * QCoreApplication::processEvents();
 * @endcode
 *
 * VCS will generally call this function at the capture's refresh rate, e.g. 70
 * times per second when capturing VGA mode 13h.
 *
 * @note
 * If the GUI wants to match the capture's refresh rate, it should repaint its
 * output only when VCS calls this function.
 */
void kd_spin_event_loop(void);

/*!
 * Asks the GUI to display an info message to the user.
 *
 * The message should be deliverable with a headless GUI, i.e. requiring
 * minimal prior GUI initialization.
 *
 * The following sample Qt 5 code creates a conforming message box:
 *
 * @code
 * QMessageBox mb;
 *
 * mb.setWindowTitle(strlen(title)? title : "VCS has this to say");
 * mb.setText(msg);
 * mb.setStandardButtons(QMessageBox::Ok);
 * mb.setIcon(QMessageBox::Information);
 * mb.setDefaultButton(QMessageBox::Ok);
 *
 * mb.exec();
 * @endcode
 *
 * @see
 * kd_show_headless_error_message(), kd_show_headless_assert_error_message()
 */
void kd_show_headless_info_message(const char *const title, const char *const msg);

/*!
 * Asks the GUI to display an error message to the user.
 *
 * The message should be deliverable with a headless GUI, i.e. requiring
 * minimal prior GUI initialization.
 *
 * @see
 * kd_show_headless_info_message(), kd_show_headless_assert_error_message()
 */
void kd_show_headless_error_message(const char *const title, const char *const msg);

/*!
 * Asks the GUI to display an assertion error message to the user.
 *
 * The message should be deliverable with a headless GUI, i.e. requiring
 * minimal prior GUI initialization.
 *
 * @see
 * kd_show_headless_info_message(), kd_show_headless_error_message()
 */
void kd_show_headless_assert_error_message(const char *const msg, const char * const filename, const uint lineNum);

/*!
 * Returns true if the output window is in fullscreen mode; false
 * otherwise.
 */
bool kd_is_fullscreen(void);

#endif
