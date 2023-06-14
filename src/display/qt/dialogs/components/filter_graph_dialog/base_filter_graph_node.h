/*
 * 2019 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 * 
 */

#ifndef VCS_DISPLAY_QT_DIALOGS_COMPONENTS_FILTER_GRAPH_DIALOG_FILTER_GRAPH_NODE_H
#define VCS_DISPLAY_QT_DIALOGS_COMPONENTS_FILTER_GRAPH_DIALOG_FILTER_GRAPH_NODE_H

#include "display/qt/subclasses/QGraphicsItem_interactible_node_graph_node.h"

class abstract_filter_c;

enum class filter_node_type_e
{
    gate,
    scaler,
    filter,
};

class BaseFilterGraphNode : public QObject, public InteractibleNodeGraphNode
{
    Q_OBJECT

public:
    BaseFilterGraphNode(
        const filter_node_type_e filterType,
        const QString title,
        const unsigned width = 240,
        const unsigned height = 130
    );
    virtual ~BaseFilterGraphNode();

    // Convenience functions that can be used to access the node's (default) input and output edge.
    virtual node_edge_s* input_edge(void) { return nullptr; }
    virtual node_edge_s* output_edge(void) { return nullptr; }

    void set_background_color(const QString &colorName);
    const QList<QString>& background_color_list(void);
    const QString& current_background_color_name(void);
    const QColor current_background_color(void);
    bool is_enabled(void) const;
    void set_enabled(const bool isEnabled);
    QRectF boundingRect(void) const;
    void generate_right_click_menu(void);

    abstract_filter_c *associatedFilter = nullptr;

signals:
    void enabled_state_set(const bool isEnabled);
    void background_color_changed(const QString &newColor);

protected:
    const filter_node_type_e filterType;

    // A list of the node background colors we support. Note: You shouldn't
    // delete or change the names of any of the items on this list. But
    // you can add more.
    const QList<QString> backgroundColorList = {
        "Black", "Blue", "Cyan", "Green", "Magenta", "Red", "Yellow",
    };

    QString backgroundColor = this->backgroundColorList.at(0);

    // Whether the filter corresponding to this node will get applied. If false,
    // the node will just act as a passthrough.
    // Note: Use set_enabled() to toggle this value at runtime, rather than
    // setting it directly.
    bool isEnabled = true;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
};

#endif
