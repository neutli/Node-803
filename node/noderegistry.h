#ifndef NODEREGISTRY_H
#define NODEREGISTRY_H

#include <QString>
#include <QMap>
#include <QVector>
#include <functional>
#include <memory>
#include "node.h"

// Factory function type
using NodeFactory = std::function<Node*()>;

struct NodeRegistration {
    QString name;
    QString category;
    NodeFactory factory;
};

class NodeRegistry {
public:
    static NodeRegistry& instance();
    void registerNodes();
    void registerNode(const QString& category, const QString& name, NodeFactory factory);
    Node* createNode(const QString& name);

    QStringList getCategories() const;
    QStringList getNodesByCategory(const QString& category) const;

private:
    NodeRegistry() {}
    QMap<QString, NodeRegistration> m_nodes; // Key: Node Name
    QMap<QString, QStringList> m_categories; // Key: Category, Value: List of Node Names
};

#endif // NODEREGISTRY_H
