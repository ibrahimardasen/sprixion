#pragma once
#include <QColor>
#include <QWidget>

class CanvasView;
class QLineEdit;
class QSlider;

class ColorPicker : public QWidget
{
    Q_OBJECT
public:
    explicit ColorPicker(CanvasView *canvas, QWidget *parent = nullptr);
    void refresh(); // re-read primary/secondary from the canvas context
    void swap();

    static QColor parseHex(const QString &text); // "#RRGGBB" or "#RRGGBBAA"; invalid result if malformed
    static QString toHex(const QColor &c);

private:
    class Surface;
    QColor &target();
    void setColor(const QColor &c);
    void fromHsv();

    CanvasView *m_canvas;
    bool m_editSecondary = false, m_syncing = false;
    int m_h = 0, m_s = 0, m_v = 0;
    Surface *m_sv, *m_swatches;
    QSlider *m_hue, *m_alpha;
    QLineEdit *m_hex;
};
