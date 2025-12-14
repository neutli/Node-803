#ifndef GRAPHNODE_H
#define GRAPHNODE_H

#include "node.h"

class GraphNode : public Node {
public:
    GraphNode();

    // QVector<NodeSocket*> inputSockets() const override { return m_inputSockets; } // Removed: Not virtual in base
    // QVector<NodeSocket*> outputSockets() const override { return m_outputSockets; } // Removed: Not virtual in base
    QVector<ParameterInfo> parameters() const override;

    void evaluate() override; // Required pure virtual
    QVariant compute(const QVector3D& pos, NodeSocket* socket) override; // Correct signature

private:
    // QVector<NodeSocket*> m_inputSockets; // Removed: Base class handles this
    // QVector<NodeSocket*> m_outputSockets; // Removed: Base class handles this

    // Parameters
    int m_functionType;
    float m_coeffA;
    float m_coeffB;
    float m_coeffC;
    float m_coeffD;
    float m_thickness;
    bool m_fillBelow; // Optional: Fill area under curve
};

#endif // GRAPHNODE_H
