#include "mappingnode.h"

MappingNode::MappingNode() : Node("Mapping") {
    m_vectorInput = new NodeSocket("Vector", SocketType::Vector, SocketDirection::Input, this);
    addInputSocket(m_vectorInput);

    m_locationInput = new NodeSocket("Location", SocketType::Vector, SocketDirection::Input, this);
    m_locationInput->setDefaultValue(QVector3D(0, 0, 0));
    addInputSocket(m_locationInput);

    m_rotationInput = new NodeSocket("Rotation", SocketType::Vector, SocketDirection::Input, this);
    m_rotationInput->setDefaultValue(QVector3D(0, 0, 0));
    addInputSocket(m_rotationInput);
    
    m_scaleInput = new NodeSocket("Scale", SocketType::Vector, SocketDirection::Input, this);
    m_scaleInput->setDefaultValue(QVector3D(1, 1, 1));
    addInputSocket(m_scaleInput);

    m_vectorOutput = new NodeSocket("Vector", SocketType::Vector, SocketDirection::Output, this);
    addOutputSocket(m_vectorOutput);
}

MappingNode::~MappingNode() {}

QVector<Node::ParameterInfo> MappingNode::parameters() const {
    return {
        ParameterInfo("Location", -100.0, 100.0, QVector3D(0, 0, 0)),
        ParameterInfo("Rotation", -360.0, 360.0, QVector3D(0, 0, 0)),
        ParameterInfo("Scale", 0.0, 100.0, QVector3D(1, 1, 1))
    };
}

void MappingNode::evaluate() {
    // Stateless
}

QVariant MappingNode::compute(const QVector3D& pos, NodeSocket* socket) {
    // 入力ベクトルを取得（接続がなければ pos を使用）
    QVector3D vec = m_vectorInput->isConnected() 
        ? m_vectorInput->getValue(pos).value<QVector3D>()
        : pos;
        
    // 接続がある場合は動的に計算、なければデフォルト値
    QVector3D loc = m_locationInput->isConnected() 
        ? m_locationInput->getValue(pos).value<QVector3D>() 
        : location();
        
    QVector3D rot = m_rotationInput->isConnected() 
        ? m_rotationInput->getValue(pos).value<QVector3D>() 
        : rotation();
        
    QVector3D scl = m_scaleInput->isConnected() 
        ? m_scaleInput->getValue(pos).value<QVector3D>() 
        : scale();
    
    // 変換行列の作成
    QMatrix4x4 mat;
    mat.translate(loc);
    mat.rotate(rot.x(), 1, 0, 0);
    mat.rotate(rot.y(), 0, 1, 0);
    mat.rotate(rot.z(), 0, 0, 1);
    mat.scale(scl);
    
    // 変換適用
    return mat.map(vec);
}

QVector3D MappingNode::location() const {
    return m_locationInput->defaultValue().value<QVector3D>();
}

QVector3D MappingNode::rotation() const {
    return m_rotationInput->defaultValue().value<QVector3D>();
}

QVector3D MappingNode::scale() const {
    return m_scaleInput->defaultValue().value<QVector3D>();
}

void MappingNode::setLocation(const QVector3D& loc) {
    m_locationInput->setDefaultValue(loc);
    setDirty(true);
}

void MappingNode::setRotation(const QVector3D& rot) {
    m_rotationInput->setDefaultValue(rot);
    setDirty(true);
}

void MappingNode::setScale(const QVector3D& scale) {
    m_scaleInput->setDefaultValue(scale);
    setDirty(true);
}
