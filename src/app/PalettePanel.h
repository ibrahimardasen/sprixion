#pragma once
#include "Document.h"
#include <QWidget>

class CanvasView;

class PalettePanel : public QWidget
{
    Q_OBJECT
public:
    // `settle` drops floating pixels so no palette edit lands on top of a lift.
    PalettePanel(CanvasView *canvas, std::function<void()> settle, QWidget *parent = nullptr);
    void setDocument(spx::Document *doc);

signals:
    void colorsChanged(); // primary or secondary was set from a swatch

private:
    class Grid;
    void editPalette(const QString &text, const std::function<void(QVector<QColor> &)> &edit);

    CanvasView *m_canvas;
    std::function<void()> m_settle;
    spx::Document *m_doc = nullptr;
    QMetaObject::Connection m_conn;
    Grid *m_grid;
};
