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
    // `settle` drops floating pixels into the current layer; it runs before the current layer changes.
    LayersPanel(CanvasView *canvas, std::function<void()> settle, QWidget *parent = nullptr);
    void setDocument(spx::Document *doc);

private:
    void rebuild();
    void setProps(int layer, const spx::Sprite &base, int opacity, bool apply);
    void rowChanged(int row);

    CanvasView *m_canvas;
    std::function<void()> m_settle;
    spx::Document *m_doc = nullptr;
    QMetaObject::Connection m_conn;
    QListWidget *m_list;
    QSlider *m_opacity;
    bool m_syncing = false, m_dragging = false;
    spx::Sprite m_beforeDrag;
};
