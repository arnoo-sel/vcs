#ifndef VCS_DISPLAY_QT_DIALOGS_FILTER_GRAPH_DIALOG_H
#define VCS_DISPLAY_QT_DIALOGS_FILTER_GRAPH_DIALOG_H

#include "display/qt/subclasses/QDialog_vcs_base_dialog.h"
#include "filter/filter.h"

class InteractibleNodeGraph;
class FilterGraphNode;
class QMenuBar;

namespace Ui {
class FilterGraphDialog;
}

class FilterGraphDialog : public VCSBaseDialog
{
    Q_OBJECT

public:
    explicit FilterGraphDialog(QWidget *parent = 0);
    ~FilterGraphDialog();

    void recalculate_filter_chains(void);

    void clear_filter_graph(void);

    FilterGraphNode* add_filter_graph_node(const filter_type_enum_e &filterType, const u8 * const initialParameterValues);

    FilterGraphNode* add_filter_node(const filter_type_enum_e type, const u8 *const initialParameterValues = nullptr);

    void refresh_filter_graph(void);

    void set_filter_graph_options(const std::vector<filter_graph_option_s> &graphOptions);

    // Loads the filter graph from the most recent used graph file. Returns true
    // if the graph was loaded; false otherwise.
    bool restore_persistent_graph(void);

    bool load_graph_from_file(const QString &filename);

private:
    void reset_graph(const bool autoAccept = false);
    void save_filters(void);
    void load_filters(void);

    Ui::FilterGraphDialog *ui;

    InteractibleNodeGraph *graphicsScene = nullptr;

    // All the nodes that are currently in the graph.
    std::vector<FilterGraphNode*> inputGateNodes;

    unsigned numNodesAdded = 0;
};

#endif
