/*
 * 2019 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 * 
 */

#ifndef VCS_DISPLAY_QT_DIALOGS_COMPONENTS_FILTER_GRAPH_DIALOG_INPUT_GATE_NODE_H
#define VCS_DISPLAY_QT_DIALOGS_COMPONENTS_FILTER_GRAPH_DIALOG_INPUT_GATE_NODE_H

#include "filter/filter.h"
#include "display/qt/dialogs/components/filter_graph_dialog/base_filter_graph_node.h"

class InputGateNode : public BaseFilterGraphNode
{
public:
    InputGateNode(const QString title,
                  const unsigned width = 200,
                  const unsigned height = 130) : BaseFilterGraphNode(filter_node_type_e::gate, title, width, height)
    {
        this->edges =
        {
            node_edge_s(node_edge_s::direction_e::out, QRect(this->width-12, 6, 24, 74), this),
        };

        return;
    }

    node_edge_s* output_edge(void) override;

private:
};

#endif
