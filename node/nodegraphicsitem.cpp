#include "nodegraphicsitem.h"
#include "node.h"
#include "noisetexturenode.h"
#include "rivernode.h"
#include "outputnode.h"
#include "watersourcenode.h"
#include "invertnode.h"
#include "voronoinode.h"
#include "imagetexturenode.h"
#include "mappingnode.h"
#include "maprangenode.h"
#include "mixnode.h"
#include "bumpnode.h"
#include "separatexyznode.h"
#include "combinexyznode.h"
#include "clampnode.h"
#include "wavetexturenode.h"
#include "bricktexturenode.h"
#include "radialtilingnode.h"
#include "calculusnode.h"
#include "texturecoordinatenode.h"
#include <QTimer>
#include "colorrampnode.h"
#include "colorrampwidget.h"
#include "watersourcerampwidget.h"
#include "sliderspinbox.h"
#include "connectiongraphicsitem.h"
#include "appsettings.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>
#include <QAction>
#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QFileDialog>
#include <QGraphicsProxyWidget>
#include <QWheelEvent>
#include <QWidget>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <QVector3D>
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsProxyWidget>
#include <QDoubleSpinBox>
#include <QGraphicsScene>
#include "mathnode.h"
#include "vectormathnode.h"
#include <QGraphicsView>
#include <QEvent>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>
#include <QWheelEvent>
#include <utility>
#include <functional>
#include <QColorDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QFileDialog>
#include <QAbstractItemView>
#include <QStyleFactory>

#include <QLineEdit>
#include "uicomponents.h"

NodeGraphicsItem::NodeGraphicsItem(Node* node, QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , m_node(node)
    , m_width(150)
    , m_height(100)
    , m_titleHeight(24)
    , m_socketSpacing(20)
    , m_previewSize(100)
{
    setFlag(ItemIsMovable);
    setFlag(ItemIsSelectable);
    setFlag(ItemSendsGeometryChanges);
    
    m_titleItem = new QGraphicsTextItem(AppSettings::instance().translate(node->name()), this);
    m_titleItem->setDefaultTextColor(Qt::white);
    m_titleItem->setFont(QFont("Segoe UI", 10, QFont::Bold));
    m_titleItem->setPos(5, 2);
    
    updateLayout();
    
    // Register callback for structure changes
    m_node->setStructureChangedCallback([this]() {
        // Defer layout update to avoid deleting widgets/sockets during event handling
        QTimer::singleShot(0, this, [this]() {
            updateLayout();
        });
    });

    // Register callback for dirty/data changes
    m_node->setDirtyCallback([this]() {
        // Rate limiting: use a short delay to coalesce rapid changes
        // This prevents infinite loops from accumulated updates
        if (!m_updatePending) {
            m_updatePending = true;
            // Use 50ms delay instead of 0 to allow changes to settle
            QTimer::singleShot(50, this, [this]() {
                m_updatePending = false;
                updatePreview();
            });
        }
    });

    // Connect language change
    connect(&AppSettings::instance(), &AppSettings::languageChanged, this, [this](AppSettings::Language) {
        updateLayout();
        update(); // For sockets
    });
}

NodeGraphicsItem::~NodeGraphicsItem() {
    // Disconnect all widgets to prevent callbacks during destruction (e.g. popup closed triggers setZValue)
    for (auto* proxy : m_parameterWidgets) {
        if (proxy && proxy->widget()) {
            QWidget* container = proxy->widget();
            container->disconnect(this);
            
            // The widget is often a container, so we must disconnect its children (like QComboBox)
            // which are the actual signal senders.
            const QList<QWidget*> children = container->findChildren<QWidget*>();
            for (auto* child : children) {
                child->disconnect(this);
            }
        }
    }

    if (m_node) {
        // Unregister callbacks to prevent use-after-free if Node outlives GraphicsItem
        m_node->setStructureChangedCallback(nullptr);
        m_node->setDirtyCallback(nullptr);
    }
}

bool NodeGraphicsItem::sceneEvent(QEvent *event) {
    if (event->type() == QEvent::GraphicsSceneMousePress) {
        QGraphicsSceneMouseEvent *me = static_cast<QGraphicsSceneMouseEvent*>(event);
        qDebug() << "NodeGraphicsItem::sceneEvent MousePress at" << me->pos() << "ScenePos:" << me->scenePos();
        
        // Check if we hit a proxy widget
        for (auto proxy : m_parameterWidgets) {
            if (proxy->geometry().contains(me->pos())) {
                qDebug() << "  -> Hit proxy widget!" << proxy->widget()->metaObject()->className();
                qDebug() << "     Proxy Z:" << proxy->zValue() << "Item Z:" << zValue();
                qDebug() << "     Proxy Visible:" << proxy->isVisible() << "Enabled:" << proxy->isEnabled();
            }
        }
    }
    return QGraphicsObject::sceneEvent(event);
}

QRectF NodeGraphicsItem::boundingRect() const {
    return QRectF(0, 0, m_width, m_height);
}

void NodeGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    // Apply mute opacity
    bool isMuted = m_node->isMuted();
    if (isMuted) {
        painter->setOpacity(0.4);
    }
    
    // Static resources to avoid reallocation (Pens are fine to be static if color doesn't change much, or make them dynamic too)
    static const QPen PEN_SELECTED(QColor(255, 150, 50), 2);
    static const QPen PEN_NORMAL(QColor(0, 0, 0), 1);
    
    // Determine colors based on Theme
    QColor bgColor, titleColor;
    auto theme = AppSettings::instance().theme();
    if (theme == AppSettings::Theme::Light) {
        bgColor = QColor(220, 220, 220);
        titleColor = QColor(200, 200, 200);
    } else if (theme == AppSettings::Theme::Colorful) {
        bgColor = QColor(50, 50, 70);
        titleColor = QColor(70, 70, 100);
    } else { // Dark
        bgColor = QColor(60, 60, 60);
        titleColor = QColor(80, 80, 80);
    }
    
    QBrush brushBackground(bgColor);
    QBrush brushTitle(titleColor);
    
    // ノードの背景
    QRectF rect = boundingRect();
    QPainterPath path;
    path.addRoundedRect(rect, 5, 5);
    
    // 選択状態で色を変える
    if (isSelected()) {
        painter->setPen(PEN_SELECTED);
    } else {
        painter->setPen(PEN_NORMAL);
    }
    
    painter->setBrush(brushBackground);
    painter->drawPath(path);
    
    // タイトルバー
    QPainterPath titlePath;
    titlePath.addRoundedRect(QRectF(0, 0, m_width, m_titleHeight), 5, 5);
    QPainterPath titleRect;
    titleRect.addRect(QRectF(0, m_titleHeight / 2, m_width, m_titleHeight / 2));
    titlePath = titlePath.united(titleRect);
    
    painter->setBrush(brushTitle);
    painter->setPen(Qt::NoPen);
    painter->drawPath(titlePath);
    
    // プレビュー描画
    if (!m_previewPixmap.isNull()) {
        qreal previewY = m_titleHeight + 10;
        painter->drawPixmap(QRectF(10, previewY, m_previewSize, m_previewSize), 
                          m_previewPixmap, 
                          m_previewPixmap.rect());
    }
    
    // Draw mute indicator (X mark)
    if (isMuted) {
        painter->setOpacity(1.0);  // Reset opacity for X
        painter->setPen(QPen(QColor(255, 100, 100), 3));
        QRectF xRect = rect.adjusted(10, 10, -10, -10);
        painter->drawLine(xRect.topLeft(), xRect.bottomRight());
        painter->drawLine(xRect.topRight(), xRect.bottomLeft());
    }
}

void NodeGraphicsItem::updateLayout() {
    prepareGeometryChange(); // CRITICAL: Notify scene about geometry change to prevent artifacts
    
    // 1. Clear Widgets SAFELY
    // We must clear the list BEFORE deleting items, because deleting a widget (like ComboBox)
    // might trigger signals (like popupClosed) which try to access m_parameterWidgets.
    auto oldWidgets = m_parameterWidgets;
    m_parameterWidgets.clear();
    qDeleteAll(oldWidgets);

    // Update Title
    m_titleItem->setPlainText(AppSettings::instance().translate(m_node->name()));

    // Set default width (wider to accommodate widgets)
    m_width = 220;
    
    // Determine if this node type uses the standard preview
    bool hasPreview = false;
    if (dynamic_cast<NoiseTextureNode*>(m_node) || 
        dynamic_cast<InvertNode*>(m_node) ||
        dynamic_cast<VoronoiNode*>(m_node)) {
        hasPreview = true;
    }

    qreal yPos = m_titleHeight + 20;
    if (hasPreview) {
        yPos += m_previewSize;
    }
    
    // 2. Global Parameters (Top)
    MathNode* mathNode = dynamic_cast<MathNode*>(m_node);
    VectorMathNode* vecMathNode = dynamic_cast<VectorMathNode*>(m_node);
    NoiseTextureNode* noiseNode = dynamic_cast<NoiseTextureNode*>(m_node);
    RiverNode* riverNode = dynamic_cast<RiverNode*>(m_node);
    ColorRampNode* colorRampNode = dynamic_cast<ColorRampNode*>(m_node);
    ImageTextureNode* imageNode = dynamic_cast<ImageTextureNode*>(m_node);

    // ImageTextureNode UI - (Migrated to Generic UI)
    // if (imageNode) { ... } removed


    


    // TextureCoordinateNode UI - (Migrated to Generic UI)
    // if (texCoordNode) { ... } removed







    // Generic API Parameters (Global/Options)
    // Checks for parameters that don't match input sockets and creates widgets for them
    QVector<Node::ParameterInfo> allParams = m_node->parameters();
    qDebug() << "NodeGraphicsItem::updateLayout - Node:" << m_node->name() << "has" << allParams.size() << "parameters";
    
    for (const auto& param : allParams) {
        qDebug() << "  Processing param:" << param.name << "type:" << param.type << "enumNames count:" << param.enumNames.size();
        
        // Skip if this parameter controls an input socket (handled later)
        // Skip Float/Int params - they are rendered in input socket section
        // Only process Enum, Bool, File, Color here (global options)
        if (param.type == Node::ParameterInfo::Float || 
            param.type == Node::ParameterInfo::Int) {
            qDebug() << "    SKIPPED (Float/Int - rendered in socket section)";
            continue;
        }
        
        if (param.type != Node::ParameterInfo::Enum && 
            param.type != Node::ParameterInfo::Bool &&
            param.type != Node::ParameterInfo::File &&
            param.type != Node::ParameterInfo::Color &&
            param.type != Node::ParameterInfo::Combo && 
            m_node->findInputSocket(param.name)) {
            qDebug() << "    SKIPPED (socket match)";
            continue; 
        }

        if (param.type == Node::ParameterInfo::Enum || param.type == Node::ParameterInfo::Combo) {
            qDebug() << "    Creating Enum/Combo widget for:" << param.name << "with items:" << param.enumNames;
            QWidget* container = new QWidget();
            container->setFixedWidth(180);
            QVBoxLayout* layout = new QVBoxLayout(container);
            layout->setContentsMargins(5, 2, 5, 2);
            layout->setSpacing(2);
            
            QLabel* label = new QLabel(AppSettings::instance().translate(param.name));
            label->setStyleSheet("color: #aaaaaa; font-size: 8pt;");
            
            PopupAwareComboBox* combo = new PopupAwareComboBox();
            
            // Apply Fusion style for reliable cross-platform rendering
            combo->setStyle(QStyleFactory::create("Fusion"));
            
            // Use palette for reliable dropdown colors
            QPalette pal = combo->palette();
            pal.setColor(QPalette::Base, QColor(0x2d, 0x2d, 0x2d));
            pal.setColor(QPalette::Text, Qt::white);
            pal.setColor(QPalette::Button, QColor(0x38, 0x38, 0x38));
            pal.setColor(QPalette::ButtonText, Qt::white);
            pal.setColor(QPalette::Highlight, QColor(0x4a, 0x90, 0xd9));
            pal.setColor(QPalette::HighlightedText, Qt::white);
            pal.setColor(QPalette::Window, QColor(0x2d, 0x2d, 0x2d));
            pal.setColor(QPalette::WindowText, Qt::white);
            combo->setPalette(pal);
            
            // Ensure view uses the same palette and style
            combo->view()->setStyle(QStyleFactory::create("Fusion"));
            combo->view()->setPalette(pal);
            
            // Minimal stylesheet - let Fusion handle most rendering
            combo->setStyleSheet(
                "QComboBox { background-color: #383838; color: white; border: 1px solid #555; border-radius: 3px; padding: 3px 5px; min-height: 18px; }"
            );
            
            for (const QString& item : param.enumNames) {
                combo->addItem(AppSettings::instance().translate(item));
            }
            
            // Set current value
            int currentIndex = param.defaultValue.toInt();
            if (currentIndex >= 0 && currentIndex < combo->count()) {
                combo->setCurrentIndex(currentIndex);
            }
            
            // Connect
            auto setter = param.setter; // Copy std::function
            // Use QComboBox::currentIndexChanged(int)
            // Note: In Qt 6, void currentIndexChanged(int) is the standard signal
            connect(combo, &QComboBox::currentIndexChanged, 
                    this, [setter, this](int index) {
                if (index < 0) return;
                if (setter) {
                    setter(index);
                    // Defer layout update
                    QTimer::singleShot(0, this, [this]() { updateLayout(); });
                    updatePreview();
                }
            });

            layout->addWidget(label);
            layout->addWidget(combo);
            
            if (!param.tooltip.isEmpty()) {
                container->setToolTip(param.tooltip);
            }
            
            container->setAttribute(Qt::WA_TranslucentBackground);
            container->resize(container->sizeHint());
            
            QGraphicsProxyWidget* proxy = new QGraphicsProxyWidget(this);
            proxy->setWidget(container);
            proxy->setPos(10, yPos);
            proxy->setZValue(100);
            m_parameterWidgets.append(proxy);
            
            // Manage Z-Order when popup opens/closes
            auto restoreZOrder = [this]() {
                for (auto* p : m_parameterWidgets) {
                    p->setZValue(100); // Restore default Z
                }
            };
            
            // Open: Lower others, raise this
            connect(combo, &PopupAwareComboBox::popupOpened, this, [proxy, this]() {
                 for (auto* p : m_parameterWidgets) {
                    if (p != proxy) {
                        p->setZValue(-100); // Background others
                    }
                }
                proxy->setZValue(10000); // Foreground this
            });
            
            // Close: Restore
            connect(combo, &PopupAwareComboBox::popupClosed, this, [restoreZOrder]() {
                restoreZOrder();
            });
            
            // Also connect to the combobox's hiding signal if possible, but standard QComboBox doesn't have a specific "popupHidden" signal easily accessible without subclassing.
            // But "activated" covers selection. What if they click away?
            // The popup closing usually triggers loss of focus or similar.
            // Let's rely on the fact that if they click another widget, that widget will receive focus?
            // Actually, for a robust reset, it's better to reset all to 100 at the start of any interaction?
            // No, that's complex. Let's start with this.
            
            yPos += container->sizeHint().height() + 5;
        } else if (param.type == Node::ParameterInfo::Combo) {
            // Same as Enum but with options from 'options' list
            QWidget* container = new QWidget();
            container->setFixedWidth(200);
            QHBoxLayout* layout = new QHBoxLayout(container);
            layout->setContentsMargins(5, 2, 5, 2);
            layout->setSpacing(5);
            
            QLabel* label = new QLabel(AppSettings::instance().translate(param.name));
            label->setStyleSheet("color: #aaaaaa; font-size: 8pt;");
            
            PopupAwareComboBox* combo = new PopupAwareComboBox();
            combo->setStyle(QStyleFactory::create("Fusion")); 
            combo->setFixedWidth(120);
            
            // Filter wheel events on the combo itself to prevent scene zooming and handle value change
            combo->installEventFilter(new WheelEventFilter([combo](int delta) {
                int count = combo->count();
                if (count == 0) return;
                int index = std::clamp(combo->currentIndex() + delta, 0, count - 1);
                combo->setCurrentIndex(index);
            }, combo));
            
            // Also install on view to prevent bubbling if necessary, though view usually handles it.
            // Actually, keep view default behavior (scrolling list) is better when popup is open.
            // So ONLY install on combo.
            
            QPalette pal = combo->palette();
            pal.setColor(QPalette::Base, QColor(0x2d, 0x2d, 0x2d));
            pal.setColor(QPalette::Text, Qt::white);
            pal.setColor(QPalette::Button, QColor(0x38, 0x38, 0x38)); // Darker button
            pal.setColor(QPalette::ButtonText, Qt::white);
            combo->setPalette(pal);
            // Ensure view uses the same palette
            combo->view()->setPalette(pal);

            combo->setStyleSheet(
                "QComboBox { background-color: #383838; color: white; border: 1px solid #555; border-radius: 3px; padding: 3px 5px; min-height: 18px; }"
                "QComboBox QAbstractItemView { background-color: #383838; color: white; selection-background-color: #505050; selection-color: white; border: 1px solid #555; outline: none; }"
                "QComboBox::item { color: white; }"
                "QComboBox::item:selected { background-color: #505050; }"
            );
            
            for (const QString& item : param.options) {
                if (item == "-") {
                    combo->insertSeparator(combo->count());
                } else {
                    combo->addItem(AppSettings::instance().translate(item));
                }
            }
            
            int currentIndex = param.defaultValue.toInt();
            if (currentIndex >= 0 && currentIndex < combo->count()) {
                combo->setCurrentIndex(currentIndex);
            }
            
            auto setter = param.setter;
            connect(combo, &QComboBox::currentIndexChanged, this, [setter, this](int index) {
                if (index < 0) return;
                if (setter) {
                    setter(index);
                    QTimer::singleShot(0, this, [this]() { updateLayout(); });
                    updatePreview();
                }
            });
            
            layout->addWidget(label);
            layout->addWidget(combo);
            
            if (!param.tooltip.isEmpty()) container->setToolTip(param.tooltip);
            container->setAttribute(Qt::WA_TranslucentBackground);
            container->resize(container->sizeHint());
            
            QGraphicsProxyWidget* proxy = new QGraphicsProxyWidget(this);
            proxy->setWidget(container);
            proxy->setPos(10, yPos);
            proxy->setZValue(100);
            m_parameterWidgets.append(proxy);
            
            // Manage Z-Order
            connect(combo, &PopupAwareComboBox::popupOpened, this, [proxy, this]() {
                 for (auto* p : m_parameterWidgets) if (p != proxy) p->setZValue(-100);
                 proxy->setZValue(10000);
            });
            connect(combo, &PopupAwareComboBox::popupClosed, this, [this]() {
                for (auto* p : m_parameterWidgets) p->setZValue(100);
            });
            
            yPos += container->sizeHint().height() + 5;
        } else if (param.type == Node::ParameterInfo::Bool) {
            QCheckBox* check = new QCheckBox(AppSettings::instance().translate(param.name));
            check->setStyleSheet("QCheckBox { color: #aaaaaa; font-size: 8pt; }");
            check->setChecked(param.defaultValue.toBool());
            
            auto setter = param.setter;
            connect(check, &QCheckBox::toggled, this, [setter, this](bool checked) {
                if (setter) {
                    setter(checked);
                    // Defer layout update
                    QTimer::singleShot(0, this, [this]() { updateLayout(); });
                    updatePreview();
                }
            });
            
            if (!param.tooltip.isEmpty()) {
                check->setToolTip(param.tooltip);
            }
            
            check->resize(check->sizeHint());
            
            QGraphicsProxyWidget* proxy = new QGraphicsProxyWidget(this);
            proxy->setWidget(check);
            proxy->setPos(10, yPos);
            proxy->setZValue(100);
            m_parameterWidgets.append(proxy);
            
            yPos += check->sizeHint().height() + 2;
            
        } else if (param.type == Node::ParameterInfo::File) {
            QWidget* container = new QWidget();
            container->setFixedWidth(200); // Slightly wider for file paths
            QVBoxLayout* layout = new QVBoxLayout(container);
            layout->setContentsMargins(5, 2, 5, 2);
            layout->setSpacing(2);
            
            // Open Button
            QPushButton* openBtn = new QPushButton(AppSettings::instance().translate("Open " + param.name)); // Or just "Open Image"
            if (param.name == "Image File") openBtn->setText(AppSettings::instance().translate("Open Image"));
            
            openBtn->setStyleSheet(
                "QPushButton { background-color: #383838; color: white; border: 1px solid #555; border-radius: 3px; padding: 4px; }"
                "QPushButton:hover { border: 1px solid #777; }"
            );
            
            // Path Label
            QString currentPath = param.defaultValue.toString();
            QLabel* pathLabel = new QLabel(currentPath.isEmpty() ? AppSettings::instance().translate("No file") : QFileInfo(currentPath).fileName());
            pathLabel->setStyleSheet("color: #aaaaaa; font-size: 8pt;");
            pathLabel->setWordWrap(true);
            
            auto setter = param.setter;
            connect(openBtn, &QPushButton::clicked, this, [setter, pathLabel, this]() {
                QString path = QFileDialog::getOpenFileName(nullptr, "Open File", "", "Images (*.png *.jpg *.jpeg *.bmp *.tga);;All Files (*.*)");
                if (!path.isEmpty()) {
                    if (setter) {
                        setter(path);
                        pathLabel->setText(QFileInfo(path).fileName());
                        updatePreview();
                    }
                }
            });
            
            layout->addWidget(openBtn);
            layout->addWidget(pathLabel);
            
            if (!param.tooltip.isEmpty()) {
                container->setToolTip(param.tooltip);
            }
            
            container->setAttribute(Qt::WA_TranslucentBackground);
            container->resize(container->sizeHint());
            
            QGraphicsProxyWidget* proxy = new QGraphicsProxyWidget(this);
            proxy->setWidget(container);
            proxy->setPos(10, yPos);
            proxy->setZValue(100);
            m_parameterWidgets.append(proxy);
            
            yPos += container->sizeHint().height() + 5;
        } else if (param.type == Node::ParameterInfo::String) {
            QWidget* container = new QWidget();
            container->setFixedWidth(180);
            QVBoxLayout* layout = new QVBoxLayout(container);
            layout->setContentsMargins(5, 2, 5, 2);
            layout->setSpacing(2);
            
            QLabel* label = new QLabel(AppSettings::instance().translate(param.name));
            label->setStyleSheet("color: #aaaaaa; font-size: 8pt;");
            
            QLineEdit* edit = new QLineEdit(param.defaultValue.toString());
            edit->setStyleSheet(
                "QLineEdit { background-color: #383838; color: white; border: 1px solid #555; border-radius: 3px; padding: 3px; }"
                "QLineEdit:focus { border: 1px solid #4a90d9; }"
            );
            
            auto setter = param.setter;
            connect(edit, &QLineEdit::textChanged, this, [setter, this](const QString& text) {
                if (setter) {
                    setter(text);
                    // Defer layout update if necessary, but for text usually not unless size changes?
                    // TextNode rerenders on change, so updatePreview is needed.
                    updatePreview();
                }
            });
            
            layout->addWidget(label);
            layout->addWidget(edit);
            
            if (!param.tooltip.isEmpty()) container->setToolTip(param.tooltip);
            container->setAttribute(Qt::WA_TranslucentBackground);
            container->resize(container->sizeHint());
            
            QGraphicsProxyWidget* proxy = new QGraphicsProxyWidget(this);
            proxy->setWidget(container);
            proxy->setPos(10, yPos);
            proxy->setZValue(100);
            m_parameterWidgets.append(proxy);
            
            yPos += container->sizeHint().height() + 5;
            
        } else if (param.type == Node::ParameterInfo::Color) {
            // Color picker button
            QWidget* container = new QWidget();
            container->setFixedWidth(220);
            QHBoxLayout* layout = new QHBoxLayout(container);
            layout->setContentsMargins(5, 2, 5, 2);
            layout->setSpacing(5);
            
            QLabel* label = new QLabel(AppSettings::instance().translate(param.name));
            label->setStyleSheet("color: #aaaaaa; font-size: 9pt;");
            
            QPushButton* colorBtn = new QPushButton();
            colorBtn->setFixedSize(60, 24);
            QColor initialColor = param.defaultValue.value<QColor>();
            colorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid #555; border-radius: 3px;").arg(initialColor.name()));
            
            auto setter = param.setter;
            connect(colorBtn, &QPushButton::clicked, this, [setter, colorBtn, this]() {
                QColor current;
                // Fix for deprecated setNamedColor: Use QColor::fromString if 6.4+, or explicit constructor
                #if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
                    current = QColor::fromString(colorBtn->styleSheet().section("background-color: ", 1, 1).section(";", 0, 0));
                #else
                    current = QColor(colorBtn->styleSheet().section("background-color: ", 1, 1).section(";", 0, 0)); // String constructor
                #endif
                QColor newColor = QColorDialog::getColor(current, nullptr, "Select Color", QColorDialog::DontUseNativeDialog);
                if (newColor.isValid() && setter) {
                    setter(newColor);
                    colorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid #555; border-radius: 3px;").arg(newColor.name()));
                    m_node->setDirty(true);
                    updatePreview();
                }
            });
            
            layout->addWidget(label);
            layout->addWidget(colorBtn);
            layout->addStretch();
            
            if (!param.tooltip.isEmpty()) {
                container->setToolTip(param.tooltip);
            }
            
            container->setAttribute(Qt::WA_TranslucentBackground);
            container->resize(container->sizeHint());
            
            QGraphicsProxyWidget* proxy = new QGraphicsProxyWidget(this);
            proxy->setWidget(container);
            proxy->setPos(10, yPos);
            proxy->setZValue(100);
            m_parameterWidgets.append(proxy);
            
            yPos += container->sizeHint().height() + 2;
        }
    }

    // REMOVED HARDCODED UI BLOCKS FOR:
    // - MixNode (Already removed)
    // - WaveTextureNode
    // - MapRangeNode (Partially migration check?)
    // - ImageTextureNode
    // - TextureCoordinateNode
    // - BrickTextureNode (was implicit/missing?? Check if I missed deleting something)
    
    // Note: MapRangeNode had a hardcoded block in my previous view, 
    // but I see lines 697-722 in current file view contain MapRangeNode UI.
    // I should remove that too since it's migrated.
    
    // Remaining hardcoded specific logic (e.g. ColorRamp) stays below.

    // ColorRampNode UI
    if (ColorRampNode* rampNode = dynamic_cast<ColorRampNode*>(m_node)) {
        ColorRampWidget* rampWidget = new ColorRampWidget(rampNode);
        
        connect(rampWidget, &ColorRampWidget::rampChanged, this, [this]() {
            m_node->setDirty(true);
            updatePreview();
        });
        
        // Let's wrap it in a container
        QWidget* container = new QWidget();
        container->setFixedWidth(190);
        QVBoxLayout* layout = new QVBoxLayout(container);
        layout->setContentsMargins(5, 2, 5, 2);
        layout->addWidget(rampWidget);
        
        container->setAttribute(Qt::WA_TranslucentBackground);
        
        QGraphicsProxyWidget* proxy = new QGraphicsProxyWidget(this);
        proxy->setWidget(container);
        proxy->setPos(10, yPos);
        proxy->setZValue(100);
        m_parameterWidgets.append(proxy);
        
        yPos += 100; // Height of widget (90) + margins
    }

    // WaterSourceNode Color Ramp UI
    if (WaterSourceNode* waterNode = dynamic_cast<WaterSourceNode*>(m_node)) {
        WaterSourceRampWidget* rampWidget = new WaterSourceRampWidget(waterNode);
        
        connect(rampWidget, &WaterSourceRampWidget::rampChanged, this, [this]() {
            m_node->setDirty(true);
            updatePreview();
        });
        
        QWidget* container = new QWidget();
        container->setFixedWidth(190);
        QVBoxLayout* layout = new QVBoxLayout(container);
        layout->setContentsMargins(5, 2, 5, 2);
        layout->addWidget(rampWidget);
        
        container->setAttribute(Qt::WA_TranslucentBackground);
        
        QGraphicsProxyWidget* proxy = new QGraphicsProxyWidget(this);
        proxy->setWidget(container);
        proxy->setPos(10, yPos);
        proxy->setZValue(100);
        m_parameterWidgets.append(proxy);
        
        yPos += 95;
    }

    // 3. Input Sockets (Interleaved with Widgets)
    QVector<NodeGraphicsSocket*> newInputItems;
    QVector<NodeSocket*> inputSockets = m_node->inputSockets(); // Store in local variable to avoid temporary
    
    for (NodeSocket* socket : inputSockets) {
        if (!socket->isVisible()) continue;

        // Reuse existing socket item if possible
        NodeGraphicsSocket* socketItem = nullptr;
        for (auto item : std::as_const(m_inputSocketItems)) {
            if (item->socket() == socket) {
                socketItem = item;
                break;
            }
        }
        
        if (!socketItem) {
            socketItem = new NodeGraphicsSocket(socket, this);
        }
        
        socketItem->setPos(0, yPos);
        newInputItems.append(socketItem);
        
        // Move yPos down for the socket label (socket dot is rendered at this Y)
        yPos += 20; // Height for socket label line
        
        // Check if we need a widget for this socket
        bool hasWidget = false;
        Node::ParameterInfo paramInfo;
        
        // Dynamic Parameter Lookup
        QVector<Node::ParameterInfo> params = m_node->parameters();
        for (const auto& p : params) {
            if (p.name == socket->name()) {
                hasWidget = true;
                paramInfo = p;
                break;
            }
        }
        
        // Override for connected sockets: usually no widget if connected
        if (socket->isConnected()) {
            hasWidget = false;
        }

        if (hasWidget) {
            // Widget is placed directly below socket label
            QWidget* container = new QWidget();
            container->setFixedWidth(m_width - 20); // Full width minus margins
            QVBoxLayout* layout = new QVBoxLayout(container);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(0);
            
            if (socket->type() == SocketType::Float || socket->type() == SocketType::Integer) {
                SliderSpinBox* spinBox = new SliderSpinBox();
                
                spinBox->setSoftRange(paramInfo.min, paramInfo.max);
                spinBox->setSpinBoxRange(-100000.0, 100000.0);
                
                if (socket->type() == SocketType::Integer) {
                    spinBox->setSingleStep(std::max(1.0, paramInfo.step));
                    spinBox->setDecimals(0);
                } else {
                    spinBox->setSingleStep(paramInfo.step > 0 ? paramInfo.step : 0.1);
                    spinBox->setDecimals(3); 
                }
                
                spinBox->setValue(socket->defaultValue().toDouble());
                
                connect(spinBox, &SliderSpinBox::valueChanged, this, [socket, this](double val) {
                    if (!qFuzzyCompare(socket->defaultValue().toDouble(), val)) {
                        qDebug() << "UI: SliderSpinBox Changed" << socket->name() << val;
                        socket->setDefaultValue(val);
                        m_node->setDirty(true);
                        updatePreview();
                    }
                });
                
                layout->addWidget(spinBox);
            } 
            else if (socket->type() == SocketType::Vector) {
                 QHBoxLayout* vectorLayout = new QHBoxLayout();
                 vectorLayout->setSpacing(2);
                 vectorLayout->setContentsMargins(0,0,0,0);
                 QVector3D currentVal = socket->defaultValue().value<QVector3D>();
                 
                 auto createSpinBox = [&](double val, std::function<void(double)> setter) {
                    QDoubleSpinBox* sb = new QDoubleSpinBox();
                    sb->setRange(paramInfo.min, paramInfo.max); 
                    sb->setValue(val);
                    sb->setSingleStep(paramInfo.step > 0 ? paramInfo.step : 0.1);
                    sb->setButtonSymbols(QAbstractSpinBox::NoButtons);
                    sb->setStyleSheet("background-color: #404040; color: white; border: 1px solid #555;");
                    sb->setFixedWidth(50);
                    connect(sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [setter, this](double v) {
                        setter(v);
                        updatePreview();
                    });
                    return sb;
                 };
                 
                 vectorLayout->addWidget(createSpinBox(currentVal.x(), [socket, this](double v){
                     QVector3D vec = socket->defaultValue().value<QVector3D>();
                     if (!qFuzzyCompare(static_cast<double>(vec.x()), v)) {
                         qDebug() << "UI: Vector X Changed" << socket->name() << v;
                         vec.setX(v); socket->setDefaultValue(vec); m_node->setDirty(true);
                     }
                 }));
                 vectorLayout->addWidget(createSpinBox(currentVal.y(), [socket, this](double v){
                     QVector3D vec = socket->defaultValue().value<QVector3D>();
                     if (!qFuzzyCompare(static_cast<double>(vec.y()), v)) {
                         qDebug() << "UI: Vector Y Changed" << socket->name() << v;
                         vec.setY(v); socket->setDefaultValue(vec); m_node->setDirty(true);
                     }
                 }));
                 vectorLayout->addWidget(createSpinBox(currentVal.z(), [socket, this](double v){
                     QVector3D vec = socket->defaultValue().value<QVector3D>();
                     if (!qFuzzyCompare(static_cast<double>(vec.z()), v)) {
                         qDebug() << "UI: Vector Z Changed" << socket->name() << v;
                         vec.setZ(v); socket->setDefaultValue(vec); m_node->setDirty(true);
                     }
                 }));
                 layout->addLayout(vectorLayout);
            }
            else if (socket->type() == SocketType::Color) {
                QPushButton* colorBtn = new QPushButton();
                colorBtn->setFixedHeight(20);
                
                auto updateBtnColor = [colorBtn, socket]() {
                    QColor c = socket->defaultValue().value<QColor>();
                    QString style = QString("background-color: %1; border: 1px solid #555; border-radius: 3px;").arg(c.name());
                    colorBtn->setStyleSheet(style);
                };
                updateBtnColor();
                
                connect(colorBtn, &QPushButton::clicked, this, [colorBtn, socket, updateBtnColor, this]() {
                    QColor c = socket->defaultValue().value<QColor>();
                    QColor newColor = QColorDialog::getColor(c, nullptr, "Select Color");
                    if (newColor.isValid() && newColor != c) {
                        qDebug() << "UI: Color Changed" << socket->name() << newColor;
                        socket->setDefaultValue(newColor);
                        updateBtnColor();
                        m_node->setDirty(true);
                        updatePreview();
                    }
                });
                
                layout->addWidget(colorBtn);
            }
            
            QGraphicsProxyWidget* proxy = new QGraphicsProxyWidget(this);
            proxy->setWidget(container);
            proxy->setPos(10, yPos); // Left aligned with node body
            proxy->setFlag(QGraphicsItem::ItemIsSelectable, false);
            proxy->setFlag(QGraphicsItem::ItemIsFocusable, true);
            proxy->setZValue(100);
            m_parameterWidgets.append(proxy);
            
            yPos += 30; // Widget height
            if (socket->type() == SocketType::Vector) yPos += 30; // Extra for vector
        } else {
            // No widget, just add regular socket spacing
        }
        
        yPos += m_socketSpacing - 20; // Subtract the 20 we already added for label
    }
    
    // Cleanup unused input sockets
    for (auto item : std::as_const(m_inputSocketItems)) {
        if (!newInputItems.contains(item)) {
            delete item;
        }
    }
    m_inputSocketItems = newInputItems;

    // 4. Float/Int Parameters (rendered like input sockets, but without socket dot)
    // These are parameters that don't have corresponding input sockets
    QVector<Node::ParameterInfo> floatIntParams = m_node->parameters();
    for (const auto& param : floatIntParams) {
        if (param.type != Node::ParameterInfo::Float && param.type != Node::ParameterInfo::Int) {
            continue; // Only process Float/Int here
        }
        
        // Skip if there's a matching input socket (already handled above)
        if (m_node->findInputSocket(param.name)) {
            continue;
        }
        
        // Draw parameter label (like a socket label but without the dot)
        QGraphicsTextItem* label = new QGraphicsTextItem(AppSettings::instance().translate(param.name), this);
        label->setDefaultTextColor(QColor(170, 170, 170));
        label->setFont(QFont("Arial", 9));
        label->setPos(15, yPos);
        label->setZValue(50);
        
        yPos += 20; // Label height
        
        // Create widget below label
        QWidget* container = new QWidget();
        container->setFixedWidth(m_width - 20);
        QVBoxLayout* layout = new QVBoxLayout(container);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        
        SliderSpinBox* spinBox = new SliderSpinBox();
        spinBox->setSoftRange(param.min, param.max);
        spinBox->setSpinBoxRange(-100000.0, 100000.0);
        
        if (param.type == Node::ParameterInfo::Int) {
            spinBox->setSingleStep(std::max(1.0, param.step));
            spinBox->setDecimals(0);
        } else {
            spinBox->setSingleStep(param.step > 0 ? param.step : 0.1);
            spinBox->setDecimals(3);
        }
        
        spinBox->setValue(param.defaultValue.toDouble());
        
        auto setter = param.setter;
        connect(spinBox, &SliderSpinBox::valueChanged, this, [setter, this](double val) {
            if (setter) {
                setter(val);
                m_node->setDirty(true);
                updatePreview();
            }
        });
        
        layout->addWidget(spinBox);
        
        if (!param.tooltip.isEmpty()) container->setToolTip(param.tooltip);
        
        QGraphicsProxyWidget* proxy = new QGraphicsProxyWidget(this);
        proxy->setWidget(container);
        proxy->setPos(10, yPos);
        proxy->setZValue(100);
        m_parameterWidgets.append(proxy);
        
        yPos += 32; // Widget height + spacing
    }

    // River Node Edge Connection Checkbox
    if (RiverNode* riverNode = dynamic_cast<RiverNode*>(m_node)) {
        QCheckBox* edgeCheck = new QCheckBox("Edge Connection");
        edgeCheck->setChecked(riverNode->edgeConnection());
        edgeCheck->setStyleSheet("color: #e0e0e0;");
        
        connect(edgeCheck, &QCheckBox::toggled, this, [riverNode, this](bool checked) {
            // Defer update to avoid crash (rebuilds UI to update button text)
            QTimer::singleShot(0, this, [riverNode, this, checked]() {
                riverNode->setEdgeConnection(checked);
                riverNode->setDirty(true);
                updatePreview();
            });
        });
        
        QGraphicsProxyWidget* proxy = new QGraphicsProxyWidget(this);
        proxy->setWidget(edgeCheck);
        proxy->setPos(10, yPos);
        yPos += 25;
        m_parameterWidgets.append(proxy);
    }

    // Output Node Auto Update Checkbox
    if (OutputNode* outputNode = dynamic_cast<OutputNode*>(m_node)) {
        QCheckBox* autoUpdateCheck = new QCheckBox("Auto Update");
        autoUpdateCheck->setChecked(outputNode->autoUpdate());
        autoUpdateCheck->setStyleSheet("color: #e0e0e0;");
        
        connect(autoUpdateCheck, &QCheckBox::toggled, this, [outputNode, this](bool checked) {
            outputNode->setAutoUpdate(checked);
            // No need to trigger update immediately, just set the flag
            // But if enabled, maybe we should trigger one?
            if (checked) updatePreview();
        });
        
        QGraphicsProxyWidget* proxy = new QGraphicsProxyWidget(this);
        proxy->setWidget(autoUpdateCheck);
        proxy->setPos(10, yPos);
        yPos += 25;
        m_parameterWidgets.append(proxy);
    }

    // 4. Output Sockets
    yPos += 10;
    QVector<NodeGraphicsSocket*> newOutputItems;
    QVector<NodeSocket*> outputSockets = m_node->outputSockets();
    
    for (NodeSocket* socket : outputSockets) {
        NodeGraphicsSocket* socketItem = nullptr;
        for (auto item : std::as_const(m_outputSocketItems)) {
            if (item->socket() == socket) {
                socketItem = item;
                break;
            }
        }
        if (!socketItem) {
            socketItem = new NodeGraphicsSocket(socket, this);
        }
        socketItem->setPos(m_width, yPos);
        newOutputItems.append(socketItem);
        yPos += m_socketSpacing;
    }
    
    // Cleanup unused output sockets
    for (auto item : std::as_const(m_outputSocketItems)) {
        if (!newOutputItems.contains(item)) {
            delete item;
        }
    }
    m_outputSocketItems = newOutputItems;
    
    m_height = yPos + 10;
    update();
}

void NodeGraphicsItem::updatePreview() {
    emit parameterChanged();
    int size = static_cast<int>(m_previewSize);
    QImage image(size, size, QImage::Format_RGB32);
    
    // NoiseTextureNode の場合
    NoiseTextureNode* noiseNode = dynamic_cast<NoiseTextureNode*>(m_node);
    if (noiseNode) {
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                double nx = static_cast<double>(x) / size;
                double ny = static_cast<double>(y) / size;
                
                QColor color = noiseNode->getColorValue(nx, ny, 0.0);
                image.setPixelColor(x, y, color);
            }
        }
        
        m_previewPixmap = QPixmap::fromImage(image);
        update();
        return;
    }
    
    // InvertNode の場合
    InvertNode* invertNode = dynamic_cast<InvertNode*>(m_node);
    if (invertNode) {
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                QVector3D pos(static_cast<double>(x) / size, static_cast<double>(y) / size, 0.0);
                
                // compute()メソッドを呼び出して色を取得
                QVector<NodeSocket*> outputs = invertNode->outputSockets();
                if (!outputs.isEmpty()) {
                    QVariant result = invertNode->compute(pos, outputs[0]);
                    QColor color = result.value<QColor>();
                    image.setPixelColor(x, y, color);
                }
            }
        }
        
        m_previewPixmap = QPixmap::fromImage(image);
        update();
        return;
    }
    
    // RiverNode preview removed (too heavy and non-functional)
    // River texture will only be visible in the main result view
}

QPointF NodeGraphicsItem::getSocketPosition(NodeSocket* socket) const {
    // 入力ソケット
    QVector<NodeSocket*> inputs = m_node->inputSockets();
    for (int i = 0; i < inputs.size(); i++) {
        if (inputs[i] == socket && i < m_inputSocketItems.size()) {
            return mapToScene(m_inputSocketItems[i]->centerPos());
        }
    }
    
    // 出力ソケット
    QVector<NodeSocket*> outputs = m_node->outputSockets();
    for (int i = 0; i < outputs.size(); i++) {
        if (outputs[i] == socket && i < m_outputSocketItems.size()) {
            return mapToScene(m_outputSocketItems[i]->centerPos());
        }
    }
    
    return QPointF();
}



QVariant NodeGraphicsItem::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == ItemPositionChange && scene()) {
        // ノードの位置を更新
        if (m_node) {
            m_node->setPosition(value.toPointF());
        }
        
        // 接続されているワイヤーの表示を更新
        for (NodeGraphicsSocket* socket : std::as_const(m_inputSocketItems)) {
            socket->updateConnectionPositions();
        }
        for (NodeGraphicsSocket* socket : std::as_const(m_outputSocketItems)) {
            socket->updateConnectionPositions();
        }
    }
    return QGraphicsItem::itemChange(change, value);
}

// NodeGraphicsSocket implementation
NodeGraphicsSocket::NodeGraphicsSocket(NodeSocket* socket, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_socket(socket)
{
}

NodeGraphicsSocket::~NodeGraphicsSocket() {
    // Notify connections that this socket is dead
    auto connections = m_connections;
    for (auto* conn : connections) {
        conn->onSocketDeleted(this);
    }
}

QRectF NodeGraphicsSocket::boundingRect() const {
    return QRectF(-SOCKET_RADIUS, -SOCKET_RADIUS, SOCKET_RADIUS * 2, SOCKET_RADIUS * 2);
}

void NodeGraphicsSocket::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    auto theme = AppSettings::instance().theme();
    bool isLight = (theme == AppSettings::Theme::Light);
    
    // ソケットの色を型に応じて変える
    QColor color;
    switch (m_socket->type()) {
        case SocketType::Float:
            color = isLight ? QColor(100, 100, 100) : QColor(160, 160, 160);
            break;
        case SocketType::Vector:
            color = isLight ? QColor(60, 60, 180) : QColor(100, 100, 200);
            break;
        case SocketType::Color:
            color = isLight ? QColor(180, 180, 50) : QColor(200, 200, 100);
            break;
        case SocketType::Integer:
            color = isLight ? QColor(50, 180, 50) : QColor(100, 200, 100);
            break;
        case SocketType::Shader:
            color = isLight ? QColor(30, 180, 80) : QColor(50, 200, 100);
            break;
    }
    
    painter->setBrush(color);
    painter->setPen(QPen(Qt::black, 1));
    painter->drawEllipse(boundingRect());
    
    // Draw highlight ring if selected for quick-connect
    if (m_highlighted) {
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(QColor(255, 200, 0), 3)); // Yellow ring
        painter->drawEllipse(boundingRect().adjusted(-3, -3, 3, 3));
    }
    
    // ソケット名
    if (m_socket->isLabelVisible()) {
        QColor textColor = isLight ? Qt::black : Qt::white;
        painter->setPen(textColor);
        
        if (m_socket->direction() == SocketDirection::Input) {
            painter->drawText(QPointF(SOCKET_RADIUS + 5, 4), AppSettings::instance().translate(m_socket->name()));
        } else {
            QFontMetrics fm(painter->font());
            QString name = AppSettings::instance().translate(m_socket->name());
            int textWidth = fm.horizontalAdvance(name);
            painter->drawText(QPointF(-SOCKET_RADIUS - textWidth - 5, 4), name);
        }
    }
}



QPointF NodeGraphicsSocket::centerPos() const {
    return pos();
}

void NodeGraphicsSocket::addConnection(ConnectionGraphicsItem* connection) {
    if (!m_connections.contains(connection)) {
        m_connections.append(connection);
        
        // Trigger layout update on parent item
        NodeGraphicsItem* item = dynamic_cast<NodeGraphicsItem*>(parentItem());
        if (item) {
            // Defer update to avoid crash during event processing
            QTimer::singleShot(0, item, [item]() { item->updateLayout(); });
        }
    }
}

void NodeGraphicsSocket::removeConnection(ConnectionGraphicsItem* connection) {
    m_connections.removeAll(connection);
    
    // Trigger layout update on parent item
    NodeGraphicsItem* item = dynamic_cast<NodeGraphicsItem*>(parentItem());
    if (item) {
        // Defer update to avoid crash during event processing
        QTimer::singleShot(0, item, [item]() { item->updateLayout(); });
    }
}

void NodeGraphicsSocket::updateConnectionPositions() {
    for (ConnectionGraphicsItem* connection : m_connections) {
        connection->updatePath();
    }
}


