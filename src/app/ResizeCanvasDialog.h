#pragma once
#include <QDialog>
#include <QSize>

class QComboBox;
class QSpinBox;

class ResizeCanvasDialog : public QDialog
{
    Q_OBJECT
public:
    ResizeCanvasDialog(QSize current, QWidget *parent = nullptr);
    QSize canvasSize() const;
    Qt::Alignment anchor() const;

private:
    QSpinBox *m_w, *m_h;
    QComboBox *m_anchor;
};
