#ifndef COLORRAMPNODE_H
#define COLORRAMPNODE_H

#include "node.h"
#include <QVector>
#include <QColor>
#include <QPair>

class ColorRampNode : public Node {
public:
    ColorRampNode();
    ~ColorRampNode() override;

    void evaluate() override;
    QVariant compute(const QVector3D& pos, NodeSocket* socket) override;

    // Ramp management
    struct Stop {
        double position;
        QColor color;
    };
    
    void addStop(double pos, const QColor& color);
    void removeStop(int index);
    void setStopPosition(int index, double pos);
    void setStopColor(int index, const QColor& color);
    void clearStops();
    QVector<Stop> stops() const { return m_stops; }

    QJsonObject save() const override;
    void restore(const QJsonObject& json) override;

private:
    QColor evaluateRamp(double t);

    NodeSocket* m_facInput;
    NodeSocket* m_colorOutput;
    NodeSocket* m_alphaOutput;

    QVector<Stop> m_stops;
};

#endif // COLORRAMPNODE_H
