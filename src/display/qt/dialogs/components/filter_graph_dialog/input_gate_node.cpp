/*
 * 2019 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 * 
 */

#include <QStyleOptionGraphicsItem>
#include <QPainter>
#include <QStyle>
#include "common/assert.h"
#include "display/qt/dialogs/components/filter_graph_dialog/input_gate_node.h"

node_edge_s* InputGateNode::output_edge(void)
{
    node_edge_s &outputEdge = this->edges[0];

    k_assert((outputEdge.direction == node_edge_s::direction_e::out), "Unexpected type of output edge.");

    return &outputEdge;
}
