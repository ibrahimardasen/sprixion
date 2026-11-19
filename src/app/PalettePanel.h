#pragma once
#include "Document.h"
#include <QWidget>

class CanvasView;

class PalettePanel : public QWidget
{
    Q_OBJECT
public:
    explicit PalettePanel(CanvasView *canvas, QWidget *parent = nullptr);
    void setDocument(spx::Document *doc);

signals:
    void colorsChanged(); // primary or secondary was set from a swatch

private:
    class Grid;
    void editPalette(const QString &text, const std::function<void(QVector<QColor> &)> &edit);

    CanvasView *m_canvas;
    spx::Document *m_doc = nullptr;
    QMetaObject::Connection m_conn;
    Grid *m_grid;
};
