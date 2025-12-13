#ifndef UICOMPONENTS_H
#define UICOMPONENTS_H

#include <QComboBox>
#include <functional>
#include <QWheelEvent>

// Helper class to detect popup events
class PopupAwareComboBox : public QComboBox {
    Q_OBJECT
public:
    using QComboBox::QComboBox;
    
signals:
    void popupOpened();
    void popupClosed();

protected:
    void showPopup() override {
        emit popupOpened();
        QComboBox::showPopup();
    }
    
    void hidePopup() override {
        QComboBox::hidePopup();
        emit popupClosed();
    }
};

class WheelEventFilter : public QObject {
public:
    WheelEventFilter(std::function<void(int)> callback, QObject* parent = nullptr)
        : QObject(parent), m_callback(callback) {}

protected:
    bool eventFilter(QObject* obj, QEvent* event) override {
        if (event->type() == QEvent::Wheel) {
            QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
            int delta = wheelEvent->angleDelta().y() > 0 ? -1 : 1;
            m_callback(delta);
            return true;
        }
        return QObject::eventFilter(obj, event);
    }

private:
    std::function<void(int)> m_callback;
};

#endif // UICOMPONENTS_H
