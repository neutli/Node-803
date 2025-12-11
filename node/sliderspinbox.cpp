#include "sliderspinbox.h"
#include <QHBoxLayout>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QKeyEvent>

SliderSpinBox::SliderSpinBox(QWidget *parent) : QWidget(parent)
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    m_slider = new QSlider(Qt::Horizontal, this);
    m_spinBox = new QDoubleSpinBox(this);

    m_min = 0.0;
    m_max = 1.0;
    m_softMin = 0.0;
    m_softMax = 1.0;
    m_multiplier = 100.0;

    m_slider->setRange(0, 100);
    m_spinBox->setRange(-10000.0, 10000.0); // Default wide range
    m_spinBox->setSingleStep(0.1);
    
    // Default styling for dark theme
    m_slider->setStyleSheet("QSlider::groove:horizontal { border: 1px solid #3A3939; height: 4px; background: #201F1F; margin: 2px 0; border-radius: 2px; } QSlider::handle:horizontal { background: #565656; border: 1px solid #565656; width: 12px; height: 12px; margin: -4px 0; border-radius: 6px; }");
    m_spinBox->setStyleSheet("QDoubleSpinBox { background-color: #201F1F; color: #B0B0B0; border: 1px solid #3A3939; border-radius: 2px; padding: 1px; } QDoubleSpinBox:focus { border: 1px solid #565656; }");

    layout->addWidget(m_slider);
    layout->addWidget(m_spinBox);

    connect(m_slider, &QSlider::valueChanged, this, &SliderSpinBox::onSliderValueChanged);
    connect(m_spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SliderSpinBox::onSpinBoxValueChanged);
}

void SliderSpinBox::setRange(double min, double max)
{
    setSoftRange(min, max);
    setSpinBoxRange(min, max);
}

void SliderSpinBox::setSoftRange(double min, double max)
{
    m_softMin = min;
    m_softMax = max;
    updateSliderRange();
}

void SliderSpinBox::setSpinBoxRange(double min, double max)
{
    m_min = min;
    m_max = max;
    m_spinBox->setRange(min, max);
}

void SliderSpinBox::setValue(double value)
{
    // Block signals to prevent feedback loop
    bool oldState = m_spinBox->blockSignals(true);
    m_spinBox->setValue(value);
    m_spinBox->blockSignals(oldState);
    
    updateSliderFromValue(value);
}

double SliderSpinBox::value() const
{
    return m_spinBox->value();
}

void SliderSpinBox::setSingleStep(double step)
{
    m_spinBox->setSingleStep(step);
}

void SliderSpinBox::setDecimals(int decimals)
{
    m_spinBox->setDecimals(decimals);
}

void SliderSpinBox::onSliderValueChanged(int value)
{
    double doubleValue = static_cast<double>(value) / m_multiplier + m_softMin;
    
    bool oldState = m_spinBox->blockSignals(true);
    m_spinBox->setValue(doubleValue);
    m_spinBox->blockSignals(oldState);
    
    emit valueChanged(doubleValue);
}

void SliderSpinBox::onSpinBoxValueChanged(double value)
{
    updateSliderFromValue(value);
    emit valueChanged(value);
}

void SliderSpinBox::updateSliderRange()
{
    // Adjust multiplier based on range to keep slider smooth (e.g. 1000 steps)
    double range = m_softMax - m_softMin;
    if (range <= 0) range = 1.0;
    
    // Target around 1000 steps
    m_multiplier = 1000.0 / range;
    
    // Ensure multiplier is power of 10 for nice steps if possible, but not strictly necessary
    // Just setting range is enough
    
    m_slider->blockSignals(true);
    m_slider->setRange(0, 1000);
    m_slider->blockSignals(false);
    
    updateSliderFromValue(m_spinBox->value());
}

void SliderSpinBox::updateSliderFromValue(double value)
{
    double sliderVal = (value - m_softMin) * m_multiplier;
    
    m_slider->blockSignals(true);
    m_slider->setValue(static_cast<int>(sliderVal));
    m_slider->blockSignals(false);
}

void SliderSpinBox::keyPressEvent(QKeyEvent* event)
{
    // Pass key events to spinbox if it doesn't have focus but we do
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        event->ignore(); // Let parent handle it (e.g. node editor)
    } else {
        QWidget::keyPressEvent(event);
    }
}
