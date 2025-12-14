#ifndef NODE_H
#define NODE_H

#include <QString>
#include <QVariant>
#include <QVector>
#include <QVector3D>
#include <QColor>
#include <QPointF>
#include <memory>
#include <QJsonObject>
#include <functional>

class Node;
class QPainter;

// ソケットの型
enum class SocketType {
    Float,
    Vector,
    Color,
    Integer,
    Shader
};

// ソケットの方向
enum class SocketDirection {
    Input,
    Output
};

// ノードソケット - 入力/出力の接続点
class NodeSocket {
public:
    NodeSocket(const QString& name, SocketType type, SocketDirection direction, Node* parentNode);
    ~NodeSocket();
    
    QString name() const { return m_name; }
    SocketType type() const { return m_type; }
    SocketDirection direction() const { return m_direction; }
    Node* parentNode() const { return m_parentNode; }
    
    // 値の取得/設定
    QVariant value() const;
    QVariant getValue(const QVector3D& pos) const; // 座標ベースの値取得
    void setValue(const QVariant& value) { m_value = value; }
    void setType(SocketType type) { m_type = type; }
    
    // 接続管理
    void addConnection(NodeSocket* other);
    void removeConnection(NodeSocket* other);
    QVector<NodeSocket*> connections() const { return m_connections; }
    bool isConnected() const { return !m_connections.isEmpty(); }
    
    // デフォルト値
    void setDefaultValue(const QVariant& value) { m_defaultValue = value; }
    QVariant defaultValue() const { return m_defaultValue; }

    // Serialization
    QJsonObject save() const;
    void restore(const QJsonObject& json);

    // Visibility
    void setLabelVisible(bool visible) { m_labelVisible = visible; }
    bool isLabelVisible() const { return m_labelVisible; }
    
    void setVisible(bool visible) { m_visible = visible; }
    bool isVisible() const { return m_visible; }

private:
    QString m_name;
    SocketType m_type;
    SocketDirection m_direction;
    Node* m_parentNode;
    QVariant m_value;
    QVariant m_defaultValue;
    QVector<NodeSocket*> m_connections;
    bool m_labelVisible;
    bool m_visible;
};

// ノード接続
class NodeConnection {
public:
    NodeConnection(NodeSocket* from, NodeSocket* to);
    
    NodeSocket* from() const { return m_from; }
    NodeSocket* to() const { return m_to; }
    
    bool isValid() const;
    static bool isValid(NodeSocket* from, NodeSocket* to);

private:
    NodeSocket* m_from;
    NodeSocket* m_to;
};

// ベースノードクラス
class Node {
public:
    Node(const QString& name);
    virtual ~Node();
    
    QString name() const { return m_name; }
    void setName(const QString& name) { m_name = name; }
    
    // 位置管理
    QPointF position() const { return m_position; }
    void setPosition(const QPointF& pos) { m_position = pos; }
    
    // ソケット管理
    void addInputSocket(NodeSocket* socket);
    void addOutputSocket(NodeSocket* socket);
    
    QVector<NodeSocket*> inputSockets() const { return m_inputSockets; }
    QVector<NodeSocket*> outputSockets() const { return m_outputSockets; }
    
    NodeSocket* findInputSocket(const QString& name) const;
    NodeSocket* findOutputSocket(const QString& name) const;
    
    // ノード評価 - 派生クラスでオーバーライド
    virtual void evaluate() = 0;
    
    // 座標ベースの計算 - デフォルトはevaluateの結果を返す
    virtual QVariant compute(const QVector3D& pos, NodeSocket* socket);
    
    // ダーティフラグ - 再計算が必要かどうか
    bool isDirty() const { return m_dirty; }
    virtual void setDirty(bool dirty);

    // Structure Change Callback
    void setStructureChangedCallback(std::function<void()> callback) { m_structureChangedCallback = callback; }
    void notifyStructureChanged() { if (m_structureChangedCallback) m_structureChangedCallback(); }

    // Dirty Callback (for UI update)
    void setDirtyCallback(std::function<void()> callback) { m_dirtyCallback = callback; }
    void notifyDirty() { if (m_dirtyCallback) m_dirtyCallback(); }

    // Serialization
    virtual QJsonObject save() const;
    virtual void restore(const QJsonObject& json);

    // Parameter definition for UI generation
    // Parameter definition for UI generation
    struct ParameterInfo {
        enum Type { Float, Vector, Color, Enum, Int, Bool, File, Combo };
        Type type;
        QString name;           // Must match input socket name OR be a custom property name
        double min;
        double max;
        QVariant defaultValue;    // Optional, if different from socket default
        double step;            // For slider/spinbox
        QString tooltip;        // Optional tooltip
        QStringList enumNames;  // For Enum
        QStringList options;    // For Combo (Alias for enumNames mostly, but distinct for clarity)
        std::function<void(const QVariant&)> setter; // Callback for value change
        
        // Constructor for convenience (Float)
        ParameterInfo() : type(Float), min(0), max(0), defaultValue(0), step(0.1) {}
        
        // Double constructor
        ParameterInfo(const QString& n, double mn, double mx, double def, double s = 0.1, const QString& t = "", std::function<void(const QVariant&)> set = nullptr)
            : type(Float), name(n), min(mn), max(mx), defaultValue(def), step(s), tooltip(t), setter(set) {}

        // Vector3D constructor
        ParameterInfo(const QString& n, double mn, double mx, const QVector3D& def, double s = 0.1, const QString& t = "", std::function<void(const QVariant&)> set = nullptr)
            : type(Vector), name(n), min(mn), max(mx), defaultValue(def), step(s), tooltip(t), setter(set) {}

        // Color constructor
        ParameterInfo(const QString& n, double mn, double mx, const QColor& def, double s = 0.1, const QString& t = "", std::function<void(const QVariant&)> set = nullptr)
            : type(Color), name(n), min(mn), max(mx), defaultValue(def), step(s), tooltip(t), setter(set) {}

        // Enum constructor
        ParameterInfo(const QString& n, const QStringList& items, const QVariant& def, 
                      std::function<void(const QVariant&)> set = nullptr, const QString& t = "")
            : type(Enum), name(n), min(0), max(items.size()-1), defaultValue(def), step(1), tooltip(t), enumNames(items), setter(set) {}
            
        // Bool constructor
        ParameterInfo(const QString& n, bool def, std::function<void(const QVariant&)> set = nullptr, const QString& t = "")
            : type(Bool), name(n), min(0), max(1), defaultValue(def), step(1), tooltip(t), setter(set) {}

        // File constructor
        ParameterInfo(const QString& n, const QString& defPath, std::function<void(const QVariant&)> set = nullptr, const QString& t = "")
            : type(File), name(n), min(0), max(0), defaultValue(defPath), step(0), tooltip(t), setter(set) {}
    };

    // Override this to define parameters for automatic UI generation
    virtual QVector<ParameterInfo> parameters() const { return QVector<ParameterInfo>(); }

    // Muted (bypass) state
    bool isMuted() const { return m_muted; }
    void setMuted(bool muted) { m_muted = muted; setDirty(true); }

protected:


    QString m_name;
    QPointF m_position;
    QVector<NodeSocket*> m_inputSockets;
    QVector<NodeSocket*> m_outputSockets;
    bool m_dirty;
    bool m_muted = false;
    std::function<void()> m_structureChangedCallback;
    std::function<void()> m_dirtyCallback;
};

#endif // NODE_H
