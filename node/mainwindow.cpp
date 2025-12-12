#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "nodeeditorwidget.h"
#include "noisetexturenode.h"
#include "rivernode.h"
#include "mappingnode.h"
#include "outputnode.h"
#include "texturecoordinatenode.h"
#include "imagetexturenode.h"
#include "nodegraphbuilder.h"
#include "appsettings.h"
#include "outputviewerwidget.h"

#include <QSplitter>
#include <QTabWidget>
#include <QToolBar>
#include <QAction>
#include <QLabel>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QMessageBox>
#include <QFileDialog>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QTimer>
#include <QElapsedTimer>
#include <QTimer>
#include <QCheckBox>
#include <QCheckBox>
#include <QComboBox>
#include <QMenuBar>
#include <QPushButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    
    // ウィンドウタイトル設定
    setWindowTitle("Node Editor - Noise Texture");
    
    // QTabWidget for tabs
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setMovable(true); // Allow tabs to be moved
    setCentralWidget(m_tabWidget);
    
    // --- Tab 1: Editor ---
    QWidget* editorTab = new QWidget();
    QVBoxLayout* editorLayout = new QVBoxLayout(editorTab);
    editorLayout->setContentsMargins(0, 0, 0, 0);
    
    // QSplitter for split view (Blender-style)
    QSplitter* splitter = new QSplitter(Qt::Horizontal, editorTab);
    editorLayout->addWidget(splitter);
    
    // Material toolbar (left of node editor)
    QWidget* materialToolbar = new QWidget(this);
    QHBoxLayout* matLayout = new QHBoxLayout(materialToolbar);
    matLayout->setContentsMargins(5, 2, 5, 2);
    matLayout->setSpacing(5);
    
    QLabel* matLabel = new QLabel("Material:", materialToolbar);
    m_materialCombo = new QComboBox(materialToolbar);
    m_materialCombo->setMinimumWidth(150);
    m_materialCombo->addItem("Material");
    m_lastMaterialName = "Material";

    // Ensure materials directory exists
    QDir dir(QCoreApplication::applicationDirPath());
    if (!dir.exists("materials")) {
        dir.mkdir("materials");
    }
    
    QPushButton* addMatBtn = new QPushButton("+", materialToolbar);
    addMatBtn->setFixedWidth(30);
    addMatBtn->setToolTip("Add new material");
    
    QPushButton* delMatBtn = new QPushButton("-", materialToolbar);
    delMatBtn->setFixedWidth(30);
    delMatBtn->setToolTip("Delete current material");
    
    matLayout->addWidget(matLabel);
    matLayout->addWidget(m_materialCombo, 1);
    matLayout->addWidget(addMatBtn);
    matLayout->addWidget(delMatBtn);
    matLayout->addStretch();
    
    connect(addMatBtn, &QPushButton::clicked, this, &MainWindow::onAddMaterial);
    connect(delMatBtn, &QPushButton::clicked, this, &MainWindow::onDeleteMaterial);
    connect(m_materialCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::onMaterialChanged);
    
    editorLayout->insertWidget(0, materialToolbar);  // Add at top
    
    // Node editor (left side)
    m_nodeEditor = new NodeEditorWidget(this);
    splitter->addWidget(m_nodeEditor);
    
    // 結果表示エリア（右側）
    QWidget* rightWidget = new QWidget(this);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    
    // Output Viewer (with drag-to-adjust viewport)
    m_outputViewer = new OutputViewerWidget(this);
    m_outputViewer->setMinimumSize(256, 256);
    
    // Connect viewport changes to re-render
    connect(m_outputViewer, &OutputViewerWidget::viewportChanged, this, &MainWindow::onRunClicked);
    
    rightLayout->addWidget(m_outputViewer, 1); // Stretch factor 1
    
    // Keep m_resultLabel as nullptr (for backward compat)
    m_resultLabel = nullptr;
    
    // Control Panel (Bottom of right side)
    QGroupBox* controlPanel = new QGroupBox("Render Settings", rightWidget);
    QVBoxLayout* controlLayout = new QVBoxLayout(controlPanel);
    
    // 1. Render Resolution
    QHBoxLayout* resLayout = new QHBoxLayout();
    QLabel* resLabel = new QLabel("Resolution:", controlPanel);
    resLabel->setToolTip("Output image size in pixels.\nHigher values = more detail but slower.\nLower values = faster preview.");
    
    QSpinBox* widthSpin = new QSpinBox(controlPanel);
    widthSpin->setRange(64, 4096);
    widthSpin->setValue(AppSettings::instance().renderWidth());
    widthSpin->setSuffix(" px");
    widthSpin->setToolTip("Image Width");
    
    QLabel* xLabel = new QLabel(" x ", controlPanel);
    QSpinBox* heightSpin = new QSpinBox(controlPanel);
    heightSpin->setRange(64, 4096);
    heightSpin->setValue(AppSettings::instance().renderHeight());
    heightSpin->setSuffix(" px");
    heightSpin->setToolTip("Image Height");
    
    connect(widthSpin, QOverload<int>::of(&QSpinBox::valueChanged), [](int value) {
        AppSettings::instance().setRenderWidth(value);
    });
    connect(heightSpin, QOverload<int>::of(&QSpinBox::valueChanged), [](int value) {
        AppSettings::instance().setRenderHeight(value);
    });
    
    // Sync spinboxes when render resolution changes from code (e.g., aspect ratio lock)
    connect(&AppSettings::instance(), &AppSettings::renderResolutionChanged, this, [widthSpin, heightSpin](int w, int h) {
        widthSpin->blockSignals(true);
        heightSpin->blockSignals(true);
        widthSpin->setValue(w);
        heightSpin->setValue(h);
        widthSpin->blockSignals(false);
        heightSpin->blockSignals(false);
    });
    
    resLayout->addWidget(resLabel);
    resLayout->addWidget(widthSpin);
    resLayout->addWidget(xLabel);
    resLayout->addWidget(heightSpin);
    resLayout->addStretch();
    controlLayout->addLayout(resLayout);
    
    // 2. Viewport Range
    QGroupBox* viewportGroup = new QGroupBox("Viewport Range (UV Space)", controlPanel);
    viewportGroup->setToolTip("Defines the visible area of the UV coordinate space.\n"
                              "Standard range is 0.0 to 1.0.\n"
                              "Increase range (e.g., 0 to 2) to zoom out/tile.\n"
                              "Decrease range (e.g., 0.4 to 0.6) to zoom in.");
    QVBoxLayout* viewportLayout = new QVBoxLayout(viewportGroup);
    
    // Reset button
    QPushButton* resetBtn = new QPushButton("リセット (0-1)", viewportGroup);
    resetBtn->setToolTip("Viewport範囲を0-1にリセット");
    viewportLayout->addWidget(resetBtn);
    
    // Link U/V Checkbox
    QCheckBox* linkUVCheck = new QCheckBox("Link U/V", viewportGroup);
    linkUVCheck->setChecked(true);
    linkUVCheck->setToolTip("When enabled, changing U range also changes V range.");
    viewportLayout->addWidget(linkUVCheck);
    
    // Helper to create Slider + SpinBox row
    auto createSliderRow = [this](const QString& label, double initialVal, std::function<void(double)> setter, QDoubleSpinBox*& spinBox) -> QWidget* {
        QWidget* rowWidget = new QWidget();
        QHBoxLayout* layout = new QHBoxLayout(rowWidget);
        layout->setContentsMargins(0, 0, 0, 0);
        
        layout->addWidget(new QLabel(label));
        
        spinBox = new QDoubleSpinBox();
        spinBox->setRange(-10.0, 10.0);
        spinBox->setValue(initialVal);
        spinBox->setSingleStep(0.1);
        
        QSlider* slider = new QSlider(Qt::Horizontal);
        slider->setRange(-100, 100); // -10.0 to 10.0 mapped to int
        slider->setValue(static_cast<int>(initialVal * 10.0));
        
        // Connect SpinBox -> Slider & Setter
        connect(spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [slider, setter](double val){
            slider->blockSignals(true);
            slider->setValue(static_cast<int>(val * 10.0));
            slider->blockSignals(false);
            setter(val);
        });
        
        // Connect Slider -> SpinBox
        connect(slider, &QSlider::valueChanged, [spinBox](int val){
            spinBox->setValue(val / 10.0);
        });
        
        layout->addWidget(slider);
        layout->addWidget(spinBox);
        return rowWidget;
    };
    
    QDoubleSpinBox *minUSpin, *minVSpin, *maxUSpin, *maxVSpin;
    
    // Min U
    viewportLayout->addWidget(createSliderRow("Min U:", AppSettings::instance().viewportMinU(), [](double v){ AppSettings::instance().setViewportMinU(v); }, minUSpin));
    // Min V
    QWidget* minVRow = createSliderRow("Min V:", AppSettings::instance().viewportMinV(), [](double v){ AppSettings::instance().setViewportMinV(v); }, minVSpin);
    viewportLayout->addWidget(minVRow);
    
    // Max U
    viewportLayout->addWidget(createSliderRow("Max U:", AppSettings::instance().viewportMaxU(), [](double v){ AppSettings::instance().setViewportMaxU(v); }, maxUSpin));
    // Max V
    QWidget* maxVRow = createSliderRow("Max V:", AppSettings::instance().viewportMaxV(), [](double v){ AppSettings::instance().setViewportMaxV(v); }, maxVSpin);
    viewportLayout->addWidget(maxVRow);
    
    // Link Logic
    auto syncUV = [=](double val, QDoubleSpinBox* targetSpin) {
        if (linkUVCheck->isChecked()) {
            targetSpin->setValue(val);
        }
    };
    
    connect(minUSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double v){ syncUV(v, minVSpin); });
    connect(maxUSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double v){ syncUV(v, maxVSpin); });
    
    // Toggle Visibility based on Link
    connect(linkUVCheck, &QCheckBox::toggled, [minVRow, maxVRow](bool checked){
        minVRow->setVisible(!checked);
        maxVRow->setVisible(!checked);
    });
    
    // Initial Sync and Visibility
    if (linkUVCheck->isChecked()) {
        minVSpin->setValue(minUSpin->value());
        maxVSpin->setValue(maxUSpin->value());
        minVRow->setVisible(false);
        maxVRow->setVisible(false);
    }
    
    // Reset button connection
    connect(resetBtn, &QPushButton::clicked, [minUSpin, minVSpin, maxUSpin, maxVSpin, this]() {
        minUSpin->setValue(0.0);
        minVSpin->setValue(0.0);
        maxUSpin->setValue(1.0);
        maxVSpin->setValue(1.0);
        AppSettings::instance().setViewportMinU(0.0);
        AppSettings::instance().setViewportMinV(0.0);
        AppSettings::instance().setViewportMaxU(1.0);
        AppSettings::instance().setViewportMaxV(1.0);
        onRunClicked();  // Re-render
    });
    
    // Sync spinboxes when viewport changes (e.g., from drag operations)
    connect(&AppSettings::instance(), &AppSettings::viewportRangeChanged, this, [minUSpin, minVSpin, maxUSpin, maxVSpin]() {
        // Block signals to prevent infinite loop
        minUSpin->blockSignals(true);
        minVSpin->blockSignals(true);
        maxUSpin->blockSignals(true);
        maxVSpin->blockSignals(true);
        
        minUSpin->setValue(AppSettings::instance().viewportMinU());
        minVSpin->setValue(AppSettings::instance().viewportMinV());
        maxUSpin->setValue(AppSettings::instance().viewportMaxU());
        maxVSpin->setValue(AppSettings::instance().viewportMaxV());
        
        minUSpin->blockSignals(false);
        minVSpin->blockSignals(false);
        maxUSpin->blockSignals(false);
        maxVSpin->blockSignals(false);
    });
    
    // 3. Auto Update & FPS
    QHBoxLayout* updateLayout = new QHBoxLayout();
    
    m_autoUpdateCheckbox = new QCheckBox("Auto Update", controlPanel);
    m_autoUpdateCheckbox->setChecked(true);
    m_autoUpdateCheckbox->setToolTip("Automatically re-render when parameters change.");
    m_autoUpdateEnabled = true;
    
    QCheckBox* showFpsCheck = new QCheckBox("Show FPS", controlPanel);
    showFpsCheck->setChecked(AppSettings::instance().showFPS());
    connect(showFpsCheck, &QCheckBox::toggled, [](bool checked){
        AppSettings::instance().setShowFPS(checked);
    });
    
    m_autoUpdateTimer = new QTimer(this);
    m_autoUpdateTimer->setInterval(200); // 200ms debounce
    m_autoUpdateTimer->setSingleShot(true);
    connect(m_autoUpdateTimer, &QTimer::timeout, this, &MainWindow::onRunClicked);
    
    connect(m_autoUpdateCheckbox, &QCheckBox::toggled, [this](bool checked){
        m_autoUpdateEnabled = checked;
        if (checked) m_autoUpdateTimer->start();
        
        // Update OutputNode's autoUpdate property if it exists
        for (Node* node : m_nodeEditor->nodes()) {
            if (OutputNode* outNode = dynamic_cast<OutputNode*>(node)) {
                outNode->setAutoUpdate(checked);
            }
        }
    });
    
    m_fpsLabel = new QLabel("FPS: --", controlPanel);
    m_fpsLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_fpsLabel->setToolTip("Frames Per Second (Render Performance)");
    
    updateLayout->addWidget(m_autoUpdateCheckbox);
    updateLayout->addWidget(showFpsCheck);
    updateLayout->addStretch();
    updateLayout->addWidget(m_fpsLabel);
    controlLayout->addLayout(updateLayout);
    
    controlLayout->addWidget(viewportGroup);
    rightLayout->addWidget(controlPanel);
    
    splitter->addWidget(rightWidget);
    
    // Connect AppSettings signals to auto-update with debounce
    connect(&AppSettings::instance(), &AppSettings::renderResolutionChanged, this, [this](int, int){
        if (m_autoUpdateCheckbox->isChecked()) {
            m_autoUpdateTimer->start();
        }
    });
    
    connect(&AppSettings::instance(), &AppSettings::viewportRangeChanged, this, [this](){
        if (m_autoUpdateCheckbox->isChecked()) {
            m_autoUpdateTimer->start();
        }
    });
    
    // Set splitter ratios: Node Editor 70%, Result 30%
    splitter->setStretchFactor(0, 7);
    splitter->setStretchFactor(1, 3);
    
    m_tabWidget->addTab(editorTab, "Editor");

    // --- Tab 2: Settings ---
    QWidget* settingsTab = new QWidget();
    QVBoxLayout* settingsLayout = new QVBoxLayout(settingsTab);
    settingsLayout->setAlignment(Qt::AlignTop);
    settingsLayout->setSpacing(20); // Add spacing between items
    settingsLayout->setContentsMargins(20, 20, 20, 20); // Add margins
    
    // CPU Usage (Threads)
    QHBoxLayout* cpuLayout = new QHBoxLayout();
    m_cpuLabel = new QLabel("CPU Usage (Threads):", settingsTab);
    QSpinBox* cpuSpinBox = new QSpinBox(settingsTab);
    cpuSpinBox->setRange(1, 32);
    cpuSpinBox->setValue(AppSettings::instance().maxThreads());
    connect(cpuSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [](int val){
        AppSettings::instance().setMaxThreads(val);
    });
    cpuLayout->addWidget(m_cpuLabel);
    cpuLayout->addWidget(cpuSpinBox);
    // Show FPS
    m_fpsCheckBox = new QCheckBox("Show FPS", settingsTab);
    m_fpsCheckBox->setChecked(AppSettings::instance().showFPS());
    connect(m_fpsCheckBox, &QCheckBox::toggled, [](bool checked){
        AppSettings::instance().setShowFPS(checked);
    });
    settingsLayout->addWidget(m_fpsCheckBox);
    
    // Language
    QHBoxLayout* langLayout = new QHBoxLayout();
    m_langLabel = new QLabel("Language:", settingsTab);
    QComboBox* langCombo = new QComboBox(settingsTab);
    langCombo->addItems({"English", "日本語", "中文"});
    langCombo->setCurrentIndex(static_cast<int>(AppSettings::instance().language()));
    connect(langCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index){
        AppSettings::instance().setLanguage(static_cast<AppSettings::Language>(index));
    });
    settingsLayout->addLayout(langLayout);
    
    // Theme
    QHBoxLayout* themeLayout = new QHBoxLayout();
    m_themeLabel = new QLabel("Theme:", settingsTab);
    QComboBox* themeCombo = new QComboBox(settingsTab);
    themeCombo->addItems({"Dark", "Light", "Colorful"});
    themeCombo->setCurrentIndex(static_cast<int>(AppSettings::instance().theme()));
    connect(themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index){
        AppSettings::instance().setTheme(static_cast<AppSettings::Theme>(index));
        applyTheme();
    });
    themeLayout->addWidget(m_themeLabel);
    themeLayout->addWidget(themeCombo);
    themeLayout->addStretch();
    settingsLayout->addLayout(themeLayout);

    settingsLayout->addStretch();
    
    m_tabWidget->addTab(settingsTab, AppSettings::instance().translate("Settings"));
    // QAction* runAction = toolbar->addAction("Run");
    // connect(runAction, &QAction::triggered, this, &MainWindow::onRunClicked);
    
    // UIからのアクションを接続
    // Note: 'actionecport' seems to be a typo in the .ui file (intended 'actionExport'?)
    if (ui->actionecport) {
        connect(ui->actionecport, &QAction::triggered, this, &MainWindow::onExportClicked);
    }
    
    // Run Action (from UI if available, otherwise manual)
    if (ui->actionactionRun) {
        connect(ui->actionactionRun, &QAction::triggered, this, &MainWindow::onRunClicked);
        // If UI has run action, we might want to hide the manual toolbar button or keep both
        // For now, keeping both is fine or we can remove the manual one if we are sure
    }

    // Save Action
    if (ui->actionsave) {
        connect(ui->actionsave, &QAction::triggered, this, &MainWindow::onSaveClicked);
    }
    // "Save Nodes" action (duplicate?)
    if (ui->action) {
        connect(ui->action, &QAction::triggered, this, &MainWindow::onSaveClicked);
    }

    // Load Action
    if (ui->action_2) {
        connect(ui->action_2, &QAction::triggered, this, &MainWindow::onLoadClicked);
    }
    
    // Settings Action
    if (ui->action_3) {
        connect(ui->action_3, &QAction::triggered, [this](){
            m_tabWidget->setCurrentIndex(1); // Switch to Settings tab
        });
    }

    // Load Demo Graph Action
    QAction* loadDemoAction = new QAction("Load Demo Graph", this);
    ui->menufile->addAction(loadDemoAction);
    connect(loadDemoAction, &QAction::triggered, this, &MainWindow::onLoadDemoClicked);

    // Exit Action
    if (ui->actionexit) {
        connect(ui->actionexit, &QAction::triggered, this, &MainWindow::close);
    }
    
    // Escape Action (Close?)
    if (ui->actionescape) {
        connect(ui->actionescape, &QAction::triggered, this, &MainWindow::close);
    }
    
    // Undo/Redo Actions
    QAction* undoAction = m_nodeEditor->undoStack()->createUndoAction(this, tr("&Undo"));
    undoAction->setShortcuts(QKeySequence::Undo);
    
    QAction* redoAction = m_nodeEditor->undoStack()->createRedoAction(this, tr("&Redo"));
    redoAction->setShortcuts(QKeySequence::Redo);
    
    // Add to Edit menu
    QMenu* editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(undoAction);
    editMenu->addAction(redoAction);
    
    // Load default scene if exists
    loadStartupGraph();
    
    qDebug() << "MainWindow: Connections done";
    
    // Connect parameter changes from node editor to auto-update
    connect(m_nodeEditor, &NodeEditorWidget::parameterChanged, 
            this, &MainWindow::onParameterChanged);
    
    // Trigger initial render since Auto Update is enabled by default
    // This fixes the bug where the first update doesn't happen until parameters change
    QTimer::singleShot(100, this, &MainWindow::onRunClicked);
    
    // Initialize language
    qDebug() << "MainWindow: Calling updateLanguage";
    updateLanguage();
    qDebug() << "MainWindow: updateLanguage done";
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onRunClicked() {
    // 出力ノードを探す
    OutputNode* outputNode = nullptr;
    for (Node* node : m_nodeEditor->nodes()) {
        outputNode = dynamic_cast<OutputNode*>(node);
        if (outputNode) break;
    }
    
    if (!outputNode) {
        QMessageBox::warning(this, "Error", "Output Node not found!");
        return;
    }
    
    // 画像生成（TextureCoordinateノードの解像度を使用）
    QElapsedTimer timer;
    timer.start();
    
    QImage result = outputNode->render(m_nodeEditor->nodes());
    
    qint64 elapsed = timer.elapsed();
    if (elapsed > 0) {
        double fps = 1000.0 / elapsed;
        if (AppSettings::instance().showFPS()) {
            m_fpsLabel->setText(QString("FPS: %1 (%2 ms)").arg(fps, 0, 'f', 1).arg(elapsed));
        } else {
            m_fpsLabel->setText("");
        }
    } else {
        if (AppSettings::instance().showFPS()) {
            m_fpsLabel->setText("FPS: >1000 (<1 ms)");
        } else {
            m_fpsLabel->setText("");
        }
    }
    
    // 結果表示
    // Use the new OutputViewerWidget for display
    m_outputViewer->setImage(result);
    // No need to switch tabs - result is already visible in split view
}

void MainWindow::onExportClicked() {
    QImage image = m_outputViewer->image();
    if (image.isNull()) {
        QMessageBox::warning(this, "Error", "No image to export. Please run the graph first.");
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(this, "Export Image", "", "Images (*.png *.jpg *.bmp)");
    if (!fileName.isEmpty()) {
        if (image.save(fileName)) {
            QMessageBox::information(this, "Success", "Image saved successfully!");
        } else {
            QMessageBox::critical(this, "Error", "Failed to save image.");
        }
    }
}

void MainWindow::onSaveClicked() {
    QString fileName = QFileDialog::getSaveFileName(this, "Save Node Graph", "", "JSON Files (*.json)");
    if (!fileName.isEmpty()) {
        m_nodeEditor->saveToFile(fileName);
        QMessageBox::information(this, "Success", "Graph saved successfully!");
    }
}

void MainWindow::onLoadClicked() {
    QString fileName = QFileDialog::getOpenFileName(this, "Load Node Graph", "", "JSON Files (*.json)");
    if (!fileName.isEmpty()) {
        // Create new material from filename
        QFileInfo fileInfo(fileName);
        QString newMaterialName = fileInfo.baseName();
        
        // Save current first
        QString currentName = m_materialCombo->currentText();
        m_nodeEditor->saveToFile("materials/" + currentName + ".json");
        
        // Add new material
        m_materialCombo->addItem(newMaterialName);
        m_materialCombo->setCurrentIndex(m_materialCombo->count() - 1);
        
        // Load into new material
        m_nodeEditor->loadFromFile(fileName);
        
        // Trigger run to update preview
        onRunClicked();
    }
}

void MainWindow::onLoadDemoClicked() {
    NodeGraphBuilder builder(m_nodeEditor);
    builder.buildDemoGraph();
    onRunClicked();
}

void MainWindow::onParameterChanged() {
    // Check if auto-update is enabled in OutputNode
    bool autoUpdate = true;
    for (Node* node : m_nodeEditor->nodes()) {
        if (OutputNode* outNode = dynamic_cast<OutputNode*>(node)) {
            autoUpdate = outNode->autoUpdate();
            break;
        }
    }

    if (!autoUpdate) {
        return;
    }
    
    // Restart timer to debounce rapid parameter changes
    // Will only trigger render after 300ms of no changes
    m_autoUpdateTimer->start();
}

void MainWindow::updateLanguage() {
    auto& settings = AppSettings::instance();
    setWindowTitle(settings.translate("Node Editor - Noise Texture"));
    m_tabWidget->setTabText(0, settings.translate("Editor"));
    m_tabWidget->setTabText(1, settings.translate("Settings"));
    m_cpuLabel->setText(settings.translate("CPU Usage (Threads):"));
    m_fpsCheckBox->setText(settings.translate("Show FPS"));
    m_langLabel->setText(settings.translate("Language:"));
    m_themeLabel->setText(settings.translate("Theme:"));
    
    // Update Menus
    if (ui->menufile) ui->menufile->setTitle(settings.translate("File"));
    if (ui->menurun) ui->menurun->setTitle(settings.translate("Run"));

    // Update Actions
    if (ui->actionactionRun) ui->actionactionRun->setText(settings.translate("Run"));
    if (ui->actionecport) ui->actionecport->setText(settings.translate("Export"));
    if (ui->actionsave) ui->actionsave->setText(settings.translate("Save"));
    if (ui->action) ui->action->setText(settings.translate("Save Nodes"));
    if (ui->action_2) ui->action_2->setText(settings.translate("Load Nodes"));
    if (ui->action_3) ui->action_3->setText(settings.translate("Settings"));
    if (ui->actionexit) ui->actionexit->setText(settings.translate("Exit"));
}

void MainWindow::applyTheme() {
    auto theme = AppSettings::instance().theme();
    QString style;
    if (theme == AppSettings::Theme::Dark) {
        style = "QMainWindow { background-color: #2b2b2b; color: #ffffff; }"
                "QTabWidget::pane { border: 1px solid #444; }"
                "QTabBar::tab { background: #333; color: #aaa; padding: 5px; }"
                "QTabBar::tab:selected { background: #555; color: #fff; }";
    } else if (theme == AppSettings::Theme::Light) {
        style = "QMainWindow { background-color: #f0f0f0; color: #000000; }"
                "QTabWidget::pane { border: 1px solid #ccc; }"
                "QTabBar::tab { background: #e0e0e0; color: #333; padding: 5px; }"
                "QTabBar::tab:selected { background: #fff; color: #000; }";
    } else { // Colorful
        style = "QMainWindow { background-color: #2b2b3b; color: #ffffff; }"
                "QTabWidget::pane { border: 1px solid #556; }"
                "QTabBar::tab { background: #334; color: #aaa; padding: 5px; }"
                "QTabBar::tab:selected { background: #668; color: #fff; }";
    }
    setStyleSheet(style);
    if (m_nodeEditor) m_nodeEditor->updateTheme(); // Trigger theme update (background, grid, repaint)
}

// Material management
void MainWindow::onAddMaterial() {
    static int matCount = 1;
    QString name = QString("Material.%1").arg(++matCount);
    
    // Save current graph
    // Use m_lastMaterialName to be sure what we are saving
    if (!m_lastMaterialName.isEmpty()) {
        QString savePath = QCoreApplication::applicationDirPath() + "/materials/" + m_lastMaterialName + ".json";
        m_nodeEditor->saveToFile(savePath);
    }
    
    // Add new material
    m_materialCombo->addItem(name);
    // Block signals to prevent double-save logic in onMaterialChanged for this specific transition if desired,
    // but onMaterialChanged logic handles "save previous" which is consistent.
    // However, we just saved "current". 
    // Let's update m_lastMaterialName to the NEW name manually if we want to skip onMaterialChanged save?
    // Actually, onMaterialChanged will see index change.
    
    m_materialCombo->setCurrentIndex(m_materialCombo->count() - 1);
    
    // Note: setCurrentIndex triggers onMaterialChanged. 
    // onMaterialChanged will try to save 'm_lastMaterialName' (which is the old one).
    // This effectively saves it TWICE. One explicit here, one in slot.
    // To be cleaner, we can remove the explicit save here and let onMaterialChanged handle it.
    // But onAddMaterial needs to ensure the NEW material starts clean/default.
    
    // Let's rely on onMaterialChanged for saving the old one.
    // ALL we do here is add item and switch.
    // BUT we need to clear/init the new one.
    // onMaterialChanged loads file. If New material file doesn't exist, it clears.
    // So we just need to detect "File doesn't exist" -> Load Default.
}

void MainWindow::onDeleteMaterial() {
    if (m_materialCombo->count() <= 1) {
        QMessageBox::warning(this, "Cannot Delete", "Must have at least one material.");
        return;
    }
    
    int index = m_materialCombo->currentIndex();
    QString name = m_materialCombo->currentText();
    
    // Remove file
    QString path = QCoreApplication::applicationDirPath() + "/materials/" + name + ".json";
    QFile::remove(path);
    
    // Update state BEFORE removing from combo, because removing might trigger change?
    // Removing item triggers currentIndexChanged if current item is removed.
    // We want the new selection to be handled gracefully.
    
    // If we remove current, combo picks a new one. onMaterialChanged fires.
    // It will try to save "name" (which we just deleted? No, m_lastMaterialName is "name").
    // We should probably prevent saving the deleted material.
    
    m_lastMaterialName = ""; // Prevent saving the material we are about to delete
    
    // Remove from combo
    m_materialCombo->removeItem(index);
}

void MainWindow::onMaterialChanged(int index) {
    if (index < 0 || !m_nodeEditor) return;
    
    // Stop processing unused/old material to save memory/cpu
    if (m_autoUpdateTimer->isActive()) {
        m_autoUpdateTimer->stop();
    }
    
    QString newMaterialName = m_materialCombo->itemText(index);

    // Save previous material
    if (!m_lastMaterialName.isEmpty() && m_lastMaterialName != newMaterialName) {
        QString savePath = QCoreApplication::applicationDirPath() + "/materials/" + m_lastMaterialName + ".json";
        m_nodeEditor->saveToFile(savePath);
        qDebug() << "Saved previous material:" << m_lastMaterialName;
    }
    
    // Load new material
    QString path = QCoreApplication::applicationDirPath() + "/materials/" + newMaterialName + ".json";
    
    if (QFile::exists(path)) {
        m_nodeEditor->loadFromFile(path);
        qDebug() << "Loaded material:" << newMaterialName;
    } else {
        // New material or missing file -> Load Default
        qDebug() << "New/Missing material, loading default:" << newMaterialName;
        // Check if it's a "fresh" start (not deleted file)
        // If we just added it, we want default graph.
        m_nodeEditor->clear(); 
        loadNewMaterialGraph();
    }
    
    m_lastMaterialName = newMaterialName;
}

void MainWindow::updateMaterialList() {
    // Not implemented yet - would scan materials folder
}

// Startup Graph (Image/BSDF) - Used on initial boot
void MainWindow::loadStartupGraph() {
    // Load embedded default scene
    const char* EMBEDDED_JSON = R"({
    "connections": [
        {
            "fromNode": 0,
            "fromSocket": "UV",
            "toNode": 1,
            "toSocket": "Vector"
        },
        {
            "fromNode": 1,
            "fromSocket": "Vector",
            "toNode": 3,
            "toSocket": "Vector"
        },
        {
            "fromNode": 4,
            "fromSocket": "BSDF",
            "toNode": 2,
            "toSocket": "Surface"
        },
        {
            "fromNode": 3,
            "fromSocket": "Color",
            "toNode": 4,
            "toSocket": "Base Color"
        }
    ],
    "nodes": [
        {
            "inputs": [
                {
                    "name": "Type",
                    "value": 2
                }
            ],
            "name": "Texture Coordinate",
            "type": "Texture Coordinate",
            "x": -1041,
            "y": 12
        },
        {
            "inputs": [
                {
                    "name": "Vector",
                    "value": {
                        "x": 0,
                        "y": 0,
                        "z": 0
                    }
                },
                {
                    "name": "Location",
                    "value": {
                        "x": 0,
                        "y": 0,
                        "z": 0
                    }
                },
                {
                    "name": "Rotation",
                    "value": {
                        "x": 0,
                        "y": 0,
                        "z": 0
                    }
                },
                {
                    "name": "Scale",
                    "value": {
                        "x": 1,
                        "y": 1,
                        "z": 1
                    }
                }
            ],
            "name": "Mapping",
            "type": "Mapping",
            "x": -691,
            "y": 12
        },
        {
            "inputs": [
                {
                    "name": "Surface",
                    "value": {
                        "a": 255,
                        "b": 0,
                        "g": 0,
                        "r": 0
                    }
                }
            ],
            "name": "Material Output",
            "type": "Material Output",
            "x": 366.92407474722154,
            "y": 188.32472863942462
        },
        {
            "filePath": "C:/Users/Minxue/Downloads/image_1.jpg",
            "inputs": [
                {
                    "name": "Vector",
                    "value": {
                        "x": 0,
                        "y": 0,
                        "z": 0
                    }
                }
            ],
            "keepAspectRatio": false,
            "name": "Image Texture",
            "repeat": false,
            "scaleX": 1,
            "scaleY": 1,
            "stretchToFit": false,
            "type": "Image Texture",
            "x": -271.4570888468808,
            "y": 131.0142722117203
        },
        {
            "inputs": [
                {
                    "name": "Base Color",
                    "value": {
                        "a": 255,
                        "b": 200,
                        "g": 200,
                        "r": 200
                    }
                },
                {
                    "name": "Metallic",
                    "value": 0
                },
                {
                    "name": "Roughness",
                    "value": 0.5
                },
                {
                    "name": "IOR",
                    "value": 1.45
                },
                {
                    "name": "Alpha",
                    "value": 1
                },
                {
                    "name": "Normal",
                    "value": {
                        "x": 0,
                        "y": 0,
                        "z": 1
                    }
                }
            ],
            "name": "Principled BSDF",
            "type": "Principled BSDF",
            "x": 62.73742911153124,
            "y": 63.53856332703225
        }
    ]
})";
    m_nodeEditor->loadFromData(QByteArray(EMBEDDED_JSON));
}

// New Material Graph (River/Water) - Used when adding new material
void MainWindow::loadNewMaterialGraph() {
    const char* EMBEDDED_JSON = R"({
    "connections": [
        {
            "fromNode": 0,
            "fromSocket": "UV",
            "toNode": 1,
            "toSocket": "Vector"
        },
        {
            "fromNode": 1,
            "fromSocket": "Vector",
            "toNode": 4,
            "toSocket": "Vector"
        },
        {
            "fromNode": 4,
            "fromSocket": "Color",
            "toNode": 2,
            "toSocket": "Water Mask"
        },
        {
            "fromNode": 5,
            "fromSocket": "Normal",
            "toNode": 3,
            "toSocket": "Surface"
        },
        {
            "fromNode": 2,
            "fromSocket": "Color",
            "toNode": 5,
            "toSocket": "Height"
        }
    ],
    "nodes": [
        {
            "inputs": [
                {
                    "name": "Type",
                    "value": 1
                }
            ],
            "name": "Texture Coordinate",
            "type": "Texture Coordinate",
            "x": -1041,
            "y": 12
        },
        {
            "inputs": [
                {
                    "name": "Vector",
                    "value": {
                        "x": 0,
                        "y": 0,
                        "z": 0
                    }
                },
                {
                    "name": "Location",
                    "value": {
                        "x": 0,
                        "y": 0,
                        "z": 0
                    }
                },
                {
                    "name": "Rotation",
                    "value": {
                        "x": 0,
                        "y": 0,
                        "z": 0
                    }
                },
                {
                    "name": "Scale",
                    "value": {
                        "x": 1,
                        "y": 1,
                        "z": 1
                    }
                }
            ],
            "name": "Mapping",
            "type": "Mapping",
            "x": -691,
            "y": 12
        },
        {
            "inputs": [
                {
                    "name": "Vector",
                    "value": {
                        "x": 0,
                        "y": 0,
                        "z": 0
                    }
                },
                {
                    "name": "Water Mask",
                    "value": {
                        "a": 255,
                        "b": 0,
                        "g": 0,
                        "r": 0
                    }
                },
                {
                    "name": "Scale",
                    "value": 5
                },
                {
                    "name": "Distortion",
                    "value": 20
                },
                {
                    "name": "Width",
                    "value": 0.02
                },
                {
                    "name": "Width Variation",
                    "value": 0.5
                },
                {
                    "name": "Attenuation",
                    "value": 0
                },
                {
                    "name": "Source Count",
                    "value": 10
                },
                {
                    "name": "Points",
                    "value": 50
                },
                {
                    "name": "Seed",
                    "value": 0
                },
                {
                    "name": "Target Color",
                    "value": {
                        "a": 255,
                        "b": 255,
                        "g": 255,
                        "r": 255
                    }
                },
                {
                    "name": "Tolerance",
                    "value": 0.1
                },
                {
                    "name": "Merge Distance",
                    "value": 0.15
                },
                {
                    "name": "River Color",
                    "value": {
                        "a": 255,
                        "b": 255,
                        "g": 255,
                        "r": 255
                    }
                },
                {
                    "name": "Dest Color",
                    "value": {
                        "a": 255,
                        "b": 0,
                        "g": 0,
                        "r": 0
                    }
                },
                {
                    "name": "Dest Count",
                    "value": 30
                },
                {
                    "name": "Dest Tolerance",
                    "value": 0.515
                },
                {
                    "name": "Dest Merge Dist",
                    "value": 0.15
                },
                {
                    "name": "Map Size",
                    "value": 512
                },
                {
                    "name": "Min Distance",
                    "value": 0.2725
                }
            ],
            "name": "River Texture",
            "type": "River Texture",
            "x": 109,
            "y": -746
        },
        {
            "inputs": [
                {
                    "name": "Surface",
                    "value": {
                        "a": 255,
                        "b": 0,
                        "g": 0,
                        "r": 0
                    }
                }
            ],
            "name": "Material Output",
            "type": "Material Output",
            "x": 1299.0449999999998,
            "y": 76.19500000000005
        },
        {
            "colorRampStops": [
                {
                    "color": "#ff000000",
                    "position": 0
                },
                {
                    "color": "#ffffffff",
                    "position": 0.27058823529411763
                }
            ],
            "inputs": [
                {
                    "name": "Vector",
                    "value": {
                        "x": 0,
                        "y": 0,
                        "z": 0
                    }
                },
                {
                    "name": "Position X",
                    "value": 0
                },
                {
                    "name": "Position Y",
                    "value": 0
                },
                {
                    "name": "Distortion",
                    "value": 0.5
                },
                {
                    "name": "Noise Scale",
                    "value": 1
                },
                {
                    "name": "Detail",
                    "value": 5.8
                },
                {
                    "name": "Roughness",
                    "value": 0.8
                },
                {
                    "name": "Lacunarity",
                    "value": 2
                },
                {
                    "name": "Seed",
                    "value": 803
                }
            ],
            "name": "Water Source",
            "type": "Water Source",
            "x": -331,
            "y": -489.4282467544069
        },
        {
            "inputs": [
                {
                    "name": "Strength",
                    "value": 1
                },
                {
                    "name": "Distance",
                    "value": 27.3
                },
                {
                    "name": "Height",
                    "value": 0
                },
                {
                    "name": "Normal",
                    "value": {
                        "x": 0,
                        "y": 0,
                        "z": 1
                    }
                }
            ],
            "invert": false,
            "name": "Bump",
            "type": "Bump",
            "x": 680.7436428125022,
            "y": 171.41928749999943
        }
    ]
})";
    m_nodeEditor->loadFromData(QByteArray(EMBEDDED_JSON));
}
