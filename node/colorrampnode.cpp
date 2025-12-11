#include "colorrampnode.h"
#include <algorithm>
#include <QJsonArray>
#include <QPainterPath>

ColorRampNode::ColorRampNode() : Node("Color Ramp") {
    m_facInput = new NodeSocket("Fac", SocketType::Float, SocketDirection::Input, this);
    m_facInput->setDefaultValue(0.5);
    addInputSocket(m_facInput);

    m_colorOutput = new NodeSocket("Color", SocketType::Color, SocketDirection::Output, this);
    m_alphaOutput = new NodeSocket("Alpha", SocketType::Float, SocketDirection::Output, this);
    addOutputSocket(m_colorOutput);
    addOutputSocket(m_alphaOutput);

    // Default stops: Black at 0, White at 1
    addStop(0.0, Qt::black);
    addStop(1.0, Qt::white);
}

ColorRampNode::~ColorRampNode() {}

void ColorRampNode::evaluate() {
    // Nothing to pre-calculate
}

void ColorRampNode::clearStops() {
    m_stops.clear();
    setDirty(true);
}

void ColorRampNode::addStop(double pos, const QColor& color) {
    m_stops.append({pos, color});
    // Sort by position
    std::sort(m_stops.begin(), m_stops.end(), [](const Stop& a, const Stop& b) {
        return a.position < b.position;
    });
    setDirty(true);
}

void ColorRampNode::removeStop(int index) {
    if (index >= 0 && index < m_stops.size() && m_stops.size() > 1) {
        m_stops.remove(index);
        setDirty(true);
    }
}

void ColorRampNode::setStopPosition(int index, double pos) {
    if (index >= 0 && index < m_stops.size()) {
        m_stops[index].position = std::max(0.0, std::min(1.0, pos));
        std::sort(m_stops.begin(), m_stops.end(), [](const Stop& a, const Stop& b) {
            return a.position < b.position;
        });
        setDirty(true);
    }
}

void ColorRampNode::setStopColor(int index, const QColor& color) {
    if (index >= 0 && index < m_stops.size()) {
        m_stops[index].color = color;
        setDirty(true);
    }
}

QColor ColorRampNode::evaluateRamp(double t) {
    t = std::max(0.0, std::min(1.0, t));

    if (m_stops.isEmpty()) return Qt::black;
    if (m_stops.size() == 1) return m_stops[0].color;

    // Find segment
    for (int i = 0; i < m_stops.size() - 1; ++i) {
        if (t >= m_stops[i].position && t <= m_stops[i+1].position) {
            double range = m_stops[i+1].position - m_stops[i].position;
            if (range < 0.0001) return m_stops[i].color;
            
            double localT = (t - m_stops[i].position) / range;
            
            // Linear interpolation
            double r = m_stops[i].color.redF() * (1.0 - localT) + m_stops[i+1].color.redF() * localT;
            double g = m_stops[i].color.greenF() * (1.0 - localT) + m_stops[i+1].color.greenF() * localT;
            double b = m_stops[i].color.blueF() * (1.0 - localT) + m_stops[i+1].color.blueF() * localT;
            double a = m_stops[i].color.alphaF() * (1.0 - localT) + m_stops[i+1].color.alphaF() * localT;
            
            return QColor::fromRgbF(r, g, b, a);
        }
    }

    // Handle out of bounds (clamping)
    if (t < m_stops.first().position) return m_stops.first().color;
    if (t > m_stops.last().position) return m_stops.last().color;

    return Qt::black; // Should not reach here
}

QVariant ColorRampNode::compute(const QVector3D& pos, NodeSocket* socket) {
    double fac = 0.5;
    if (m_facInput->isConnected()) {
        QVariant v = m_facInput->getValue(pos);
        if (v.canConvert<QColor>()) {
            // Use luminance if color
            QColor c = v.value<QColor>();
            fac = 0.299 * c.redF() + 0.587 * c.greenF() + 0.114 * c.blueF();
        } else {
            fac = v.toDouble();
        }
    } else {
        fac = m_facInput->value().toDouble();
    }

    QColor resultColor = evaluateRamp(fac);

    if (socket == m_colorOutput) {
        return resultColor;
    } else if (socket == m_alphaOutput) {
        return resultColor.alphaF();
    }

    return QVariant();
}

QJsonObject ColorRampNode::save() const {
    QJsonObject json = Node::save();
    QJsonArray stopsArray;
    for (const auto& stop : m_stops) {
        QJsonObject stopObj;
        stopObj["position"] = stop.position;
        stopObj["color"] = stop.color.name(QColor::HexArgb);
        stopsArray.append(stopObj);
    }
    json["stops"] = stopsArray;
    return json;
}

void ColorRampNode::restore(const QJsonObject& json) {
    Node::restore(json);
    if (json.contains("stops")) {
        m_stops.clear();
        QJsonArray stopsArray = json["stops"].toArray();
        for (const auto& val : stopsArray) {
            QJsonObject stopObj = val.toObject();
            double pos = stopObj["position"].toDouble();
            QColor color(stopObj["color"].toString());
            m_stops.append({pos, color});
        }
        // Ensure sorted
        std::sort(m_stops.begin(), m_stops.end(), [](const Stop& a, const Stop& b) {
            return a.position < b.position;
        });
    }
}
