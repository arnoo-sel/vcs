#ifndef VCS_DISPLAY_QT_DIALOGS_VIDEO_PARAMETER_DIALOG_H
#define VCS_DISPLAY_QT_DIALOGS_VIDEO_PARAMETER_DIALOG_H

#include "filter/filter.h"
#include "display/qt/subclasses/QGraphicsItem_interactible_node_graph_node.h"
#include "display/qt/subclasses/QDialog_vcs_base_dialog.h"

class InteractibleNodeGraph;
class VideoGraphNode;
class QMenuBar;

struct video_preset_s;

enum class video_graph_node_type_e
{
    video_parameters,
    resolution,
    label,
    refresh_rate,
    shortcut
};

class VideoGraphNode : public QObject, public InteractibleNodeGraphNode
{
    Q_OBJECT

public:
};

namespace Ui {
class VideoParameterDialog;
}

class VideoParameterDialog : public VCSBaseDialog
{
    Q_OBJECT

public:
    explicit VideoParameterDialog(QWidget *parent = 0);
    ~VideoParameterDialog();

    // Called to inform the dialog of a new source file for video presets.
    void assign_presets(const std::vector<video_preset_s*> &presets);

    // Attempts to load the presets stored in the given file. Returns true on
    // success; false otherwise.
    bool load_presets_from_file(const QString &filename);

    bool save_video_presets_to_file(QString filename);

signals:
    // Emitted when the last item in the preset list is removed.
    void preset_list_became_empty(void);

    // Emitted when an item is added to an empty preset list.
    void preset_list_no_longer_empty(void);

    // Emitted when the user has requested that a different preset be shown in
    // the GUI.
    void preset_change_requested(const unsigned newPresetId);

    // Emitted when presets are loaded from or saved to a file.
    void new_presets_source_file(const QString &filename);

private:
    void add_video_preset_to_list(const video_preset_s *const preset);

    // Returns the index in the list of presets of the preset with the given id.
    // Asserts on failure.
    unsigned find_preset_idx_in_list(const unsigned presetId);

    void update_preset_control_ranges(void);

    void update_preset_controls_with_current_preset_data(void);

    QString make_preset_list_text(const video_preset_s *const preset);

    void broadcast_current_preset_parameters(void);

    void update_current_present_list_text(void);

    void resort_preset_list(void);

    void remove_video_preset_from_list(const video_preset_s *const preset);

    void remove_all_video_presets_from_list(void);

    Ui::VideoParameterDialog *ui;

    InteractibleNodeGraph *graphicsScene = nullptr;

    // All the nodes that are currently in the graph.
    std::vector<FilterGraphNode*> inputGateNodes;

    // The video preset we're currently editing.
    video_preset_s *currentPreset = nullptr;

    unsigned numNodesAdded = 0;
};

#endif
