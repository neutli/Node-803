#include "commands.h"
#include "nodeeditorwidget.h"

// --- AddNodeCommand ---
AddNodeCommand::AddNodeCommand(NodeEditorWidget* widget, Node* node, const QPointF& pos, QUndoCommand* parent)
    : QUndoCommand(parent), m_widget(widget), m_node(node), m_pos(pos), m_firstRun(true)
{
    setText("Add Node");
}

AddNodeCommand::~AddNodeCommand() {
    // If the command is undone (node is removed from scene), we own it and must delete it.
    // If the command is redone (node is in scene), the scene/widget owns it.
    // BUT QUndoStack might delete this command.
    // If the node is NOT in the scene, we must delete it.
    // How to track?
    // Usually, if m_node->graphicsItem() is null or not in scene?
    // A safer way: check if m_node is in m_widget->nodes().
    // If not, we delete it.
    
    if (m_widget && m_node) {
        bool inScene = m_widget->nodes().contains(m_node);
        if (!inScene) {
            delete m_node;
        }
    }
}

void AddNodeCommand::undo() {
    // Remove node from scene but keep ownership (don't delete)
    m_widget->detachNode(m_node);
}

void AddNodeCommand::redo() {
    if (m_firstRun) {
        m_widget->addNode(m_node, m_pos);
        m_firstRun = false;
    } else {
        // Node pointer is valid (we saved it in undo)
        m_widget->addNode(m_node, m_pos);
    }
}

// --- DeleteNodeCommand ---
DeleteNodeCommand::DeleteNodeCommand(NodeEditorWidget* widget, Node* node, QUndoCommand* parent)
    : QUndoCommand(parent), m_widget(widget), m_node(node)
{
    setText("Delete Node");
    // Save connections
    for (NodeSocket* output : node->outputSockets()) {
        for (NodeSocket* input : output->connections()) {
            m_connections.append({node, output->name(), input->parentNode(), input->name()});
        }
    }
    for (NodeSocket* input : node->inputSockets()) {
        for (NodeSocket* output : input->connections()) {
            m_connections.append({output->parentNode(), output->name(), node, input->name()});
        }
    }
}

DeleteNodeCommand::~DeleteNodeCommand() {
    // If node is deleted (Redo state), we own it and delete it.
    if (m_widget && m_node) {
        bool inScene = m_widget->nodes().contains(m_node);
        if (!inScene) {
            delete m_node;
        }
    }
}

void DeleteNodeCommand::undo() {
    // Add node back
    m_widget->addNode(m_node, m_node->position());
    
    // Restore connections
    for (const auto& conn : m_connections) {
        NodeSocket* from = conn.fromNode->findOutputSocket(conn.fromSocket);
        NodeSocket* to = conn.toNode->findInputSocket(conn.toSocket);
        if (from && to) {
            m_widget->createConnection(from, to);
        }
    }
}

void DeleteNodeCommand::redo() {
    // Remove connections FIRST, because detachNode deletes the graphics items (sockets)
    // that ConnectionGraphicsItem depends on.
    for (const auto& conn : m_connections) {
        NodeSocket* from = conn.fromNode->findOutputSocket(conn.fromSocket);
        NodeSocket* to = conn.toNode->findInputSocket(conn.toSocket);
        if (from && to) {
            m_widget->removeConnection(from, to);
        }
    }

    // Remove node (don't delete, we keep ownership)
    m_widget->detachNode(m_node);
}

// --- MoveNodeCommand ---
MoveNodeCommand::MoveNodeCommand(NodeEditorWidget* widget, const QList<Node*>& nodes, const QList<QPointF>& oldPos, const QList<QPointF>& newPos, QUndoCommand* parent)
    : QUndoCommand(parent), m_widget(widget), m_nodes(nodes), m_oldPos(oldPos), m_newPos(newPos)
{
    setText("Move Node");
}

void MoveNodeCommand::undo() {
    for (int i = 0; i < m_nodes.size(); ++i) {
        m_nodes[i]->setPosition(m_oldPos[i]);
        m_widget->updateNodePosition(m_nodes[i]);
    }
    m_widget->viewport()->update();
}

void MoveNodeCommand::redo() {
    for (int i = 0; i < m_nodes.size(); ++i) {
        m_nodes[i]->setPosition(m_newPos[i]);
        m_widget->updateNodePosition(m_nodes[i]);
    }
    m_widget->viewport()->update();
}

// --- ConnectCommand ---
ConnectCommand::ConnectCommand(NodeEditorWidget* widget, NodeSocket* from, NodeSocket* to, QUndoCommand* parent)
    : QUndoCommand(parent), m_widget(widget), m_from(from), m_to(to)
{
    setText("Connect");
}

void ConnectCommand::undo() {
    m_widget->removeConnection(m_from, m_to);
}

void ConnectCommand::redo() {
    m_widget->createConnection(m_from, m_to);
}

// --- DisconnectCommand ---
DisconnectCommand::DisconnectCommand(NodeEditorWidget* widget, NodeSocket* from, NodeSocket* to, QUndoCommand* parent)
    : QUndoCommand(parent), m_widget(widget), m_from(from), m_to(to)
{
    setText("Disconnect");
}

void DisconnectCommand::undo() {
    m_widget->createConnection(m_from, m_to);
}

void DisconnectCommand::redo() {
    m_widget->removeConnection(m_from, m_to);
}
