#ifndef MAPRANGENODE_H
#define MAPRANGENODE_H

#include "node.h"

class MapRangeNode : public Node {
public:
    MapRangeNode();
    ~MapRangeNode() override;

    void evaluate() override;
    QVariant compute(const QVector3D& pos, NodeSocket* socket) override;
    
    QVector<ParameterInfo> parameters() const override;

    bool clamp() const { return m_clamp; }
    void setClamp(bool c);

    QJsonObject save() const override;
    void restore(const QJsonObject& json) override;

private:
    NodeSocket* m_valueInput;
    NodeSocket* m_fromMinInput;
    NodeSocket* m_fromMaxInput;
    NodeSocket* m_toMinInput;
    NodeSocket* m_toMaxInput;
    NodeSocket* m_resultOutput;
    
    bool m_clamp;
};

#endif // MAPRANGENODE_H
