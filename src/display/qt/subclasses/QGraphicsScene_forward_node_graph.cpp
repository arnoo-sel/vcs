/*
 * 2019 Tarpeeksi Hyvae Soft /
 * VCS
 *
 * A forward-flowing node graph implemented as a subclass of QGraphicsScene.
 *
 * Usage is along the lines of:
 *
 *     // Establish the scene and set up a view to draw it.
 *     QGraphicsView *view = new QGraphicsView(this);
 *     QGraphicsScen *scene = new ForwardNodeGraph(this);
 *     view->setScene(scene);
 *
 *     // Create a node to be displayed in the scene.
 *     ForwardNodeGraphNode *node = new ForwardNodeGraphNode("Test");
 *     node->setFlags(node->flags() | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemSendsGeometryChanges);
 *     scene->addItem(node);
 *
 * Please see Qt's documentation on QGraphicsScene (and QGraphicsView) for more
 * information on how to use the node graph.
 *
 * Each node in the graph has one or more input and/or output edges, via which
 * connections can be established to other nodes.
 *
 * The graph can be visited via its nodes in the forward direction only, as each
 * node stores only its outgoing connections, not incoming ones. You can check out
 * the implementation of ForwardNodeGraphNode for more information.
 *
 * The user can connect nodes to each other by pressing the mouse button over a
 * node's edge and releasing the press over the target edge. In-code, this is
 * implemented as a 'connection event' (node_connection_event_s).
 *
 */

#include <QApplication>
#include <QWidget>
#include <QCursor>
#include <QDebug>
#include "QGraphicsScene_forward_node_graph.h"

ForwardNodeGraph::ForwardNodeGraph(QObject *parent) :
    QGraphicsScene(parent)
{
    return;
}

void ForwardNodeGraph::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        // If this press was over a node edge, start an edge connection event.
        const auto intersectedItems = this->items(event->scenePos(), Qt::IntersectsItemBoundingRect);
        for (const auto item: intersectedItems)
        {
            const auto node = dynamic_cast<ForwardNodeGraphNode*>(item);

            if (node)
            {
                const auto edge = node->intersected_edge(event->scenePos());

                if (edge)
                {
                    start_connection_event(edge, event->scenePos());
                    return;
                }

                break;
            }
        }
    }

    QGraphicsScene::mousePressEvent(event);

    return;
}

void ForwardNodeGraph::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        // If this release was over a node edge, finalize the current edge connection event.
        const auto intersectedItems = this->items(event->scenePos(), Qt::IntersectsItemBoundingRect);
        for (const auto item: intersectedItems)
        {
            const auto node = dynamic_cast<ForwardNodeGraphNode*>(item);

            if (node)
            {
                const auto edge = node->intersected_edge(event->scenePos());

                if (edge)
                {
                    complete_connection_event(edge);
                    return;
                }

                // In case the user released the mouse button after dragging a node.
                update_scene_connections();

                break;
            }
        }

        reset_current_connection_event();
    }

    QGraphicsScene::mouseReleaseEvent(event);

    return;
}

void ForwardNodeGraph::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    // Assume that if the user is holding the mouse button down while moving
    // the cursor, they're dragging one of the items in the scene.
    if (QApplication::mouseButtons() == Qt::LeftButton)
    {
        update_scene_connections();

        this->connectionEvent.mousePos = event->scenePos();
    }

    QGraphicsScene::mouseMoveEvent(event);

    return;
}

// Reposition any connections between nodes in the scene, to account for any changes
// in the nodes' positioning etc.
void ForwardNodeGraph::update_scene_connections(void)
{
    // Refresh connections between nodes.
    for (const auto &connection: this->edgeConnections)
    {
        const QPoint p1 = QPoint(connection.edge1->parentNode->mapToScene(connection.edge1->rect.center()).x(),
                                 connection.edge1->parentNode->mapToScene(connection.edge1->rect.center()).y());

        const QPoint p2 = QPoint(connection.edge2->parentNode->mapToScene(connection.edge2->rect.center()).x(),
                                 connection.edge2->parentNode->mapToScene(connection.edge2->rect.center()).y());

        const QLine line = QLine(p1, p2);

        connection.line->setLine(line);
    }

    // If the user is dragging a new connection, draw a line from the source
    // edge to the cursor.
    if (this->connectionEvent.sourceEdge)
    {
        const QPoint p1 = QPoint(this->connectionEvent.sourceEdge->parentNode->mapToScene(this->connectionEvent.sourceEdge->rect.center()).x(),
                                 this->connectionEvent.sourceEdge->parentNode->mapToScene(this->connectionEvent.sourceEdge->rect.center()).y());

        const QPoint p2 = QPoint(this->connectionEvent.mousePos.x(), this->connectionEvent.mousePos.y());

        const QLine line = QLine(p1, p2);

        if (this->connectionEvent.graphicsLine)
        {
            this->connectionEvent.graphicsLine->setLine(line);
        }
        else
        {
            this->connectionEvent.graphicsLine = this->addLine(line, QPen("mediumseagreen"));
        }
    }

    return;
}

// Connect the two edges to each other in the scene using a line.
void ForwardNodeGraph::connect_scene_edges(const node_edge_s *const sourceEdge,
                                           const node_edge_s *const targetEdge)
{
    const QPoint p1 = QPoint(sourceEdge->parentNode->mapToScene(sourceEdge->rect.center()).x(),
                             sourceEdge->parentNode->mapToScene(sourceEdge->rect.center()).y());

    const QPoint p2 = QPoint(targetEdge->parentNode->mapToScene(targetEdge->rect.center()).x(),
                             targetEdge->parentNode->mapToScene(targetEdge->rect.center()).y());

    const QLine line = QLine(p1, p2);

    // Find whether a line between these edges already exists.
    const auto existingConnection = std::find_if(this->edgeConnections.begin(), this->edgeConnections.end(), [=](const node_edge_connection_s &connection)
    {
        return ((connection.edge1 == sourceEdge && connection.edge2 == targetEdge) ||
                (connection.edge1 == targetEdge && connection.edge2 == sourceEdge));
    });

    if (existingConnection != this->edgeConnections.end())
    {
        (*existingConnection).line->setLine(line);
    }
    else
    {
        this->edgeConnections.push_back(node_edge_connection_s(sourceEdge, targetEdge,
                                                               this->addLine(line, QPen("mediumseagreen"))));
    }

    return;
}

void ForwardNodeGraph::start_connection_event(node_edge_s *const sourceEdge,
                                              const QPointF mousePos)
{
    if (this->connectionEvent.sourceEdge)
    {
        qCritical() << "The previous connect event must finish before another can be started.";
        return;
    }

    this->connectionEvent.sourceEdge = sourceEdge;
    this->connectionEvent.mousePos = mousePos;

    return;
}

void ForwardNodeGraph::complete_connection_event(node_edge_s *const finalEdge)
{
    if (!this->connectionEvent.sourceEdge)
    {
        qCritical() << "Can't finalize a connect event without a source edge.";
        return;
    }

    if (this->connectionEvent.sourceEdge->connect_to(finalEdge))
    {
        connect_scene_edges(this->connectionEvent.sourceEdge, finalEdge);
    }

    reset_current_connection_event();

    return;
}

void ForwardNodeGraph::reset_current_connection_event(void)
{
    this->connectionEvent.sourceEdge = nullptr;
    this->connectionEvent.mousePos = QPointF();

    if (this->connectionEvent.graphicsLine)
    {
        this->removeItem(this->connectionEvent.graphicsLine);
        delete this->connectionEvent.graphicsLine;
        this->connectionEvent.graphicsLine = nullptr;
    }

    return;
}