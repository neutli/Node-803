#ifndef COMMANDS_H
#define COMMANDS_H

#include <QUndoCommand>
#include <QPointF>
#include <QList>
#include "node.h"
#include "nodeeditorwidget.h"

class AddNodeCommand : public QUndoCommand {
public:
    AddNodeCommand(NodeEditorWidget* widget, Node* node, const QPointF& pos, QUndoCommand* parent = nullptr);
    ~AddNodeCommand();

    void undo() override;
    void redo() override;

private:
    NodeEditorWidget* m_widget;
    Node* m_node;
    QPointF m_pos;
    bool m_firstRun;
};

class DeleteNodeCommand : public QUndoCommand {
public:
    DeleteNodeCommand(NodeEditorWidget* widget, Node* node, QUndoCommand* parent = nullptr);
    ~DeleteNodeCommand();

    void undo() override;
    void redo() override;

private:
    NodeEditorWidget* m_widget;
    Node* m_node;
    
    struct ConnectionInfo {
        Node* fromNode;
        QString fromSocket;
        Node* toNode;
        QString toSocket;
    };
    QList<ConnectionInfo> m_connections;
};

class MoveNodeCommand : public QUndoCommand {
public:
    MoveNodeCommand(NodeEditorWidget* widget, const QList<Node*>& nodes, const QList<QPointF>& oldPos, const QList<QPointF>& newPos, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    NodeEditorWidget* m_widget;
    QList<Node*> m_nodes;
    QList<QPointF> m_oldPos;
    QList<QPointF> m_newPos;
};

class ConnectCommand : public QUndoCommand {
public:
    ConnectCommand(NodeEditorWidget* widget, NodeSocket* from, NodeSocket* to, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    NodeEditorWidget* m_widget;
    NodeSocket* m_from;
    NodeSocket* m_to;
};

class DisconnectCommand : public QUndoCommand {
public:
    DisconnectCommand(NodeEditorWidget* widget, NodeSocket* from, NodeSocket* to, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    NodeEditorWidget* m_widget;
    NodeSocket* m_from;
    NodeSocket* m_to;
};

#endif // COMMANDS_H
