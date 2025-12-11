#include "materialmanager.h"
#include "nodeeditorwidget.h"
#include <QJsonDocument>

int MaterialManager::createMaterial(const QString& name) {
    int id = m_nextId++;
    m_materials[id] = Material(name);
    
    if (m_currentId < 0) {
        m_currentId = id;
    }
    
    emit materialAdded(id);
    emit materialsChanged();
    return id;
}

bool MaterialManager::deleteMaterial(int id) {
    if (!m_materials.contains(id)) return false;
    if (m_materials.size() <= 1) return false;  // Must have at least one material
    
    m_materials.remove(id);
    
    if (m_currentId == id) {
        m_currentId = m_materials.keys().first();
        emit currentMaterialChanged(m_currentId);
    }
    
    emit materialRemoved(id);
    emit materialsChanged();
    return true;
}

bool MaterialManager::renameMaterial(int id, const QString& newName) {
    if (!m_materials.contains(id)) return false;
    
    m_materials[id].name = newName;
    emit materialRenamed(id, newName);
    emit materialsChanged();
    return true;
}

void MaterialManager::setCurrentMaterial(int id) {
    if (!m_materials.contains(id)) return;
    if (m_currentId == id) return;
    
    m_currentId = id;
    emit currentMaterialChanged(id);
}

QString MaterialManager::materialName(int id) const {
    if (m_materials.contains(id)) {
        return m_materials[id].name;
    }
    return QString();
}

void MaterialManager::saveCurrentMaterial(NodeEditorWidget* editor) {
    if (!editor || m_currentId < 0) return;
    
    // Save node graph to temporary file and read JSON
    QString tempPath = "temp_material_save.json";
    editor->saveToFile(tempPath);
    
    QFile file(tempPath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        m_materials[m_currentId].nodeData = doc.object();
        file.close();
        QFile::remove(tempPath);
    }
}

void MaterialManager::loadCurrentMaterial(NodeEditorWidget* editor) {
    if (!editor || m_currentId < 0) return;
    if (!m_materials.contains(m_currentId)) return;
    
    QJsonObject nodeData = m_materials[m_currentId].nodeData;
    if (nodeData.isEmpty()) {
        editor->clear();
        return;
    }
    
    // Write to temp file and load
    QString tempPath = "temp_material_load.json";
    QFile file(tempPath);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(nodeData);
        file.write(doc.toJson());
        file.close();
        
        editor->loadFromFile(tempPath);
        QFile::remove(tempPath);
    }
}

QJsonObject MaterialManager::saveAll() const {
    QJsonObject json;
    QJsonArray materialsArray;
    
    for (auto it = m_materials.begin(); it != m_materials.end(); ++it) {
        QJsonObject mat;
        mat["id"] = it.key();
        mat["name"] = it.value().name;
        mat["nodeData"] = it.value().nodeData;
        materialsArray.append(mat);
    }
    
    json["materials"] = materialsArray;
    json["currentId"] = m_currentId;
    return json;
}

void MaterialManager::restoreAll(const QJsonObject& json) {
    m_materials.clear();
    
    QJsonArray materialsArray = json["materials"].toArray();
    for (const QJsonValue& val : materialsArray) {
        QJsonObject mat = val.toObject();
        int id = mat["id"].toInt();
        Material material(mat["name"].toString());
        material.nodeData = mat["nodeData"].toObject();
        m_materials[id] = material;
        
        if (id >= m_nextId) {
            m_nextId = id + 1;
        }
    }
    
    m_currentId = json["currentId"].toInt();
    
    if (m_materials.isEmpty()) {
        createMaterial("Material");
    }
    
    emit materialsChanged();
}
