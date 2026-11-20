#pragma once
#include "Document.h"
#include <QWidget>
#include <functional>

class CanvasView;
class QListWidget;
class QListWidgetItem;
class QSlider;

class LayersPanel : public QWidget
{
    Q_OBJECT
public:
    // `settle` drops floating pixels before any layer edit; `select` is MainWindow's path for changing the current layer.
    LayersPanel(CanvasView *canvas, std::function<void()> settle, std::function<void(int layer)> select, QWidget *parent = nullptr);
    void setDocument(spx::Document *doc);
    void rebuild();

private:
    void setProps(int layer, const spx::Sprite &base, int opacity, bool apply);
    void rowChanged(int row);

    CanvasView *m_canvas;
    std::function<void()> m_settle;
    std::function<void(int)> m_select;
    spx::Document *m_doc = nullptr;
    QMetaObject::Connection m_conn;
    QListWidget *m_list;
    QSlider *m_opacity;
    bool m_syncing = false, m_dragging = false;
    spx::Sprite m_beforeDrag;
};
