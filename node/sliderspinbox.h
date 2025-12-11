#ifndef SLIDERSPINBOX_H
#define SLIDERSPINBOX_H

#include <QWidget>

class QSlider;
class QDoubleSpinBox;

class SliderSpinBox : public QWidget
{
    Q_OBJECT
public:
    explicit SliderSpinBox(QWidget *parent = nullptr);

    void setRange(double min, double max);
    void setSoftRange(double min, double max); // For slider only
    void setSpinBoxRange(double min, double max); // For spinbox only (wider range)
    void setValue(double value);
    double value() const;
    void setSingleStep(double step);
    void setDecimals(int decimals);

protected:
    void keyPressEvent(QKeyEvent* event) override;

signals:
    void valueChanged(double value);

private slots:
    void onSliderValueChanged(int value);
    void onSpinBoxValueChanged(double value);

private:
    void updateSliderRange();
    void updateSliderFromValue(double value);

    QSlider* m_slider;
    QDoubleSpinBox* m_spinBox;
    double m_min;
    double m_max;
    double m_softMin;
    double m_softMax;
    double m_multiplier; // To convert double to int for slider
};

#endif // SLIDERSPINBOX_H
