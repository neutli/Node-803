#ifndef MATERIALMANAGER_H
#define MATERIALMANAGER_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>

class NodeEditorWidget;

// Material data structure
struct Material {
    QString name;
    QJsonObject nodeData;  // Serialized node graph
    
    Material(const QString& n = "Material") : name(n) {}
};

// Manager for multiple materials
class MaterialManager : public QObject {
    Q_OBJECT
    
public:
    static MaterialManager& instance() {
        static MaterialManager inst;
        return inst;
    }
    
    // Material management
    int createMaterial(const QString& name = "Material");
    bool deleteMaterial(int id);
    bool renameMaterial(int id, const QString& newName);
    
    // Get/set current material
    int currentMaterialId() const { return m_currentId; }
    void setCurrentMaterial(int id);
    
    // Get material list
    QList<int> materialIds() const { return m_materials.keys(); }
    QString materialName(int id) const;
    
    // Save/restore from NodeEditorWidget
    void saveCurrentMaterial(NodeEditorWidget* editor);
    void loadCurrentMaterial(NodeEditorWidget* editor);
    
    // Serialization
    QJsonObject saveAll() const;
    void restoreAll(const QJsonObject& json);
    
signals:
    void materialAdded(int id);
    void materialRemoved(int id);
    void materialRenamed(int id, const QString& newName);
    void currentMaterialChanged(int id);
    void materialsChanged();
    
private:
    MaterialManager() : m_currentId(-1), m_nextId(0) {
        createMaterial("Material");  // Default material
    }
    
    QMap<int, Material> m_materials;
    int m_currentId;
    int m_nextId;
};

#endif // MATERIALMANAGER_H
