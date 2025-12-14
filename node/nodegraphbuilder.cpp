#include "nodegraphbuilder.h"
#include "nodeeditorwidget.h"
#include "node.h"
#include "noderegistry.h"
#include "colorrampnode.h"
#include "mixnode.h"
#include "maprangenode.h"
#include "noisetexturenode.h"
#include <QDebug>

NodeGraphBuilder::NodeGraphBuilder(NodeEditorWidget* editor, QObject* parent)
    : QObject(parent), m_editor(editor) {}

Node* NodeGraphBuilder::createNode(const QString& type, const QString& name, double x, double y) {
    Node* node = NodeRegistry::instance().createNode(type);
    if (node) {
        node->setName(name);
        node->setPosition(QPointF(x, y));
        m_editor->addNode(node);
        m_nodes[name] = node;
    } else {
        qWarning() << "Failed to create node type:" << type;
    }
    return node;
}

void NodeGraphBuilder::setSocketValue(const QString& nodeName, const QString& socketName, const QVariant& value) {
    if (m_nodes.contains(nodeName)) {
        Node* node = m_nodes[nodeName];
        qDebug() << "NodeGraphBuilder::setSocketValue" << nodeName << socketName << "node:" << node;
        if (!node) {
            qWarning() << "Node pointer is null for" << nodeName;
            return;
        }
        NodeSocket* socket = node->findInputSocket(socketName);
        if (socket) {
            socket->setDefaultValue(value);
        } else {
            qWarning() << "Socket not found:" << socketName << "in node" << nodeName;
        }
    } else {
        qWarning() << "Node not found:" << nodeName;
    }
}

void NodeGraphBuilder::connectNodes(const QString& fromNode, const QString& fromSocket, const QString& toNode, const QString& toSocket) {
    if (m_nodes.contains(fromNode) && m_nodes.contains(toNode)) {
        Node* src = m_nodes[fromNode];
        Node* dst = m_nodes[toNode];
        
        NodeSocket* srcSock = src->findOutputSocket(fromSocket);
        NodeSocket* dstSock = dst->findInputSocket(toSocket);
        
        if (srcSock && dstSock) {
            m_editor->createConnection(srcSock, dstSock);
        } else {
            qWarning() << "Failed to connect" << fromNode << ":" << fromSocket << "to" << toNode << ":" << toSocket;
        }
    }
}

void NodeGraphBuilder::buildDemoGraph() {
    m_nodes.clear();
    m_editor->clear(); // Assuming clear() exists or we implement it

    // Replicating the Blender script
    
    // Node プリンシプルBSDF
    createNode("Principled BSDF", "プリンシプルBSDF", -229.4, 96.5);
    setSocketValue("プリンシプルBSDF", "Metallic", 0.0);
    setSocketValue("プリンシプルBSDF", "IOR", 1.5);
    setSocketValue("プリンシプルBSDF", "Alpha", 1.0);
    setSocketValue("プリンシプルBSDF", "Roughness", 0.4); // From script top level default

    // Node マテリアル出力
    createNode("Output", "マテリアル出力", 1001.6, 136.0);

    // Node マッピング
    createNode("Mapping", "マッピング", -1989.1, -31.4);
    setSocketValue("マッピング", "Scale", QVector3D(1.0, 1.0, 1.0));

    // Node テクスチャ座標
    createNode("Texture Coordinate", "テクスチャ座標", -2169.1, -31.4);

    // Node ノイズテクスチャ
    createNode("Noise Texture", "ノイズテクスチャ", -1305.0, 28.1);
    setSocketValue("ノイズテクスチャ", "Scale", 2.5);
    setSocketValue("ノイズテクスチャ", "Detail", 8.0);
    setSocketValue("ノイズテクスチャ", "Roughness", 0.588);
    setSocketValue("ノイズテクスチャ", "Lacunarity", 2.0);
    setSocketValue("ノイズテクスチャ", "Distortion", 0.0);
    if (auto n = dynamic_cast<NoiseTextureNode*>(m_nodes["ノイズテクスチャ"])) n->setNoiseType(NoiseType::Perlin); // FBM approx

    // Node バンプ
    createNode("Bump", "バンプ", -390.2, -137.8);
    setSocketValue("バンプ", "Strength", 1.0);
    setSocketValue("バンプ", "Distance", 14.3);

    // Node 範囲マッピング
    createNode("Map Range", "範囲マッピング", -626.3, -156.1);
    setSocketValue("範囲マッピング", "From Min", 0.485);
    setSocketValue("範囲マッピング", "From Max", 1.0);
    setSocketValue("範囲マッピング", "To Min", 0.0);
    setSocketValue("範囲マッピング", "To Max", 1.0);
    if (auto n = dynamic_cast<MapRangeNode*>(m_nodes["範囲マッピング"])) n->setClamp(true);

    // Node カラーランプ
    createNode("Color Ramp", "カラーランプ", -792.6, 212.6);
    if (auto n = dynamic_cast<ColorRampNode*>(m_nodes["カラーランプ"])) {
        n->clearStops(); // Clear default
        n->addStop(0.0, QColor::fromRgbF(0.028, 0.026, 0.001, 1.0));
        n->addStop(1.0, QColor::fromRgbF(0.161, 0.161, 0.161, 1.0));
    }
    setSocketValue("カラーランプ", "Fac", 0.5);

    // Node プリンシプルBSDF.001
    createNode("Principled BSDF", "プリンシプルBSDF.001", -151.4, -802.2);
    setSocketValue("プリンシプルBSDF.001", "Base Color", QColor::fromRgbF(0.266, 0.266, 0.266, 1.0));
    setSocketValue("プリンシプルBSDF.001", "Roughness", 0.4);

    // Node マッピング.001
    createNode("Mapping", "マッピング.001", -1155.9, -767.3);

    // Node テクスチャ座標.001
    createNode("Texture Coordinate", "テクスチャ座標.001", -1326.2, -835.3);

    // Node ノイズテクスチャ.001
    createNode("Noise Texture", "ノイズテクスチャ.001", -906.3, -1041.2);
    setSocketValue("ノイズテクスチャ.001", "Scale", 0.3);
    setSocketValue("ノイズテクスチャ.001", "Detail", 8.0);
    setSocketValue("ノイズテクスチャ.001", "Roughness", 0.588);
    setSocketValue("ノイズテクスチャ.001", "Lacunarity", 2.0);

    // Node バンプ.001
    createNode("Bump", "バンプ.001", -341.6, -1040.0);
    setSocketValue("バンプ.001", "Strength", 1.0);
    setSocketValue("バンプ.001", "Distance", 0.701);

    // Node シェーダーミックス
    createNode("Mix Shader", "シェーダーミックス", 697.6, 4.8);
    setSocketValue("シェーダーミックス", "Fac", 0.5);

    // Node ノイズテクスチャ.002
    createNode("Noise Texture", "ノイズテクスチャ.002", -1327.5, -323.8);
    setSocketValue("ノイズテクスチャ.002", "Scale", 6.5);
    setSocketValue("ノイズテクスチャ.002", "Detail", 8.0);
    setSocketValue("ノイズテクスチャ.002", "Roughness", 0.588);
    setSocketValue("ノイズテクスチャ.002", "Lacunarity", 2.0);

    // Node ミックス
    createNode("Mix", "ミックス", -920.2, -27.9);
    setSocketValue("ミックス", "Factor", 0.5);
    if (auto n = dynamic_cast<MixNode*>(m_nodes["ミックス"])) {
        n->setColorBlendMode(MixNode::ColorBlendMode::LinearLight); // LINEAR_LIGHT
        n->setDataType(MixNode::DataType::Color); // RGBA in script
    }

    // Node カラーランプ.001
    createNode("Color Ramp", "カラーランプ.001", -380.2, 382.3);
    if (auto n = dynamic_cast<ColorRampNode*>(m_nodes["カラーランプ.001"])) {
        n->clearStops();
        n->addStop(0.0, QColor::fromRgbF(0.075, 0.075, 0.075, 1.0));
        n->addStop(0.25, QColor::fromRgbF(0.499, 0.499, 0.499, 1.0));
        n->addStop(0.5, QColor::fromRgbF(0.336, 0.336, 0.336, 1.0));
        n->addStop(1.0, QColor::fromRgbF(1.0, 1.0, 1.0, 1.0));
    }

    // Node カラーランプ.002
    createNode("Color Ramp", "カラーランプ.002", -623.1, -777.0);
    if (auto n = dynamic_cast<ColorRampNode*>(m_nodes["カラーランプ.002"])) {
        n->clearStops();
        n->addStop(0.0, QColor::fromRgbF(0.175, 0.175, 0.175, 1.0));
        n->addStop(0.995, QColor::fromRgbF(0.558, 0.558, 0.558, 1.0));
    }

    // Links
    connectNodes("マッピング", "Vector", "ノイズテクスチャ", "Vector");
    connectNodes("バンプ", "Normal", "プリンシプルBSDF", "Normal");
    connectNodes("範囲マッピング", "Result", "バンプ", "Height");
    connectNodes("テクスチャ座標", "Object", "マッピング", "Vector");
    connectNodes("カラーランプ", "Color", "プリンシプルBSDF", "Base Color");
    connectNodes("マッピング.001", "Vector", "ノイズテクスチャ.001", "Vector");
    connectNodes("バンプ.001", "Normal", "プリンシプルBSDF.001", "Normal");
    connectNodes("テクスチャ座標.001", "Object", "マッピング.001", "Vector");
    connectNodes("ノイズテクスチャ.001", "Fac", "バンプ.001", "Height");
    connectNodes("プリンシプルBSDF.001", "BSDF", "シェーダーミックス", "Shader 1"); // Input 1
    connectNodes("マッピング", "Vector", "ノイズテクスチャ.002", "Vector");
    connectNodes("プリンシプルBSDF", "BSDF", "シェーダーミックス", "Shader 2"); // Input 2
    connectNodes("ノイズテクスチャ", "Fac", "ミックス", "A"); // Input 6 -> A (approx)
    connectNodes("ノイズテクスチャ.002", "Fac", "ミックス", "B"); // Input 7 -> B (approx)
    connectNodes("ミックス", "Result", "範囲マッピング", "Value");
    connectNodes("ミックス", "Result", "カラーランプ.001", "Fac");
    connectNodes("カラーランプ.001", "Color", "プリンシプルBSDF", "Roughness");
    connectNodes("ノイズテクスチャ.001", "Fac", "カラーランプ.002", "Fac");
    connectNodes("カラーランプ.002", "Color", "プリンシプルBSDF.001", "Roughness");
    connectNodes("シェーダーミックス", "Shader", "マテリアル出力", "Surface"); // Surface is usually input 0
}
