#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class NodeEditorWidget;
class QTabWidget;
class QLabel;
class QTimer;
class QCheckBox;
class QScrollArea;
class OutputViewerWidget;
class QComboBox;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void onRunClicked();
    void onExportClicked();
    void onSaveClicked();
    void onLoadClicked();
    void onLoadDemoClicked();
    void onAddMultipleImages();
    void onAddMultipleNodes(); // New slot for bulk node add
    void onParameterChanged(); // Called when node parameters change

private:
    void setupAutoUpdate();
    
    Ui::MainWindow *ui;
    NodeEditorWidget* m_nodeEditor;
    OutputViewerWidget* m_outputViewer;  // Replaces m_resultLabel
    QLabel* m_resultLabel;  // Keep for backward compat
    QScrollArea* m_outputScrollArea;
    
    // Real-time preview (Blender-style)
    QTimer* m_autoUpdateTimer;
    QCheckBox* m_autoUpdateCheckbox;
    bool m_autoUpdateEnabled;
    QLabel* m_fpsLabel;


    void updateLanguage();
    void applyTheme();

    // Settings UI pointers
    QTabWidget* m_tabWidget;
    QLabel* m_cpuLabel;
    QCheckBox* m_fpsCheckBox;
    QLabel* m_langLabel;
    QLabel* m_themeLabel;
    
    // Material system UI
    QComboBox* m_materialCombo;
    QString m_lastMaterialName; // Track active material for saving
    void updateMaterialList();
    void onMaterialChanged(int index);
    void onAddMaterial();
    void onDeleteMaterial();
    
    void loadStartupGraph();
    void loadNewMaterialGraph();
};
#endif // MAINWINDOW_H
