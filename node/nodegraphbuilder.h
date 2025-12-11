#ifndef NODEGRAPHBUILDER_H
#define NODEGRAPHBUILDER_H

#include <QObject>
#include <QMap>
#include <QString>

class NodeEditorWidget;
class Node;

class NodeGraphBuilder : public QObject {
    Q_OBJECT
public:
    explicit NodeGraphBuilder(NodeEditorWidget* editor, QObject* parent = nullptr);

    void buildDemoGraph();

private:
    NodeEditorWidget* m_editor;
    QMap<QString, Node*> m_nodes;

    Node* createNode(const QString& type, const QString& name, double x, double y);
    void setSocketValue(const QString& nodeName, const QString& socketName, const QVariant& value);
    void connectNodes(const QString& fromNode, const QString& fromSocket, const QString& toNode, const QString& toSocket);
};

#endif // NODEGRAPHBUILDER_H
