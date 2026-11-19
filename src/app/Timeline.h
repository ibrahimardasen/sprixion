#pragma once
#include "Document.h"
#include <QTimer>
#include <QWidget>
#include <functional>
#include <utility>

class CanvasView;
class QAction;
class QToolBar;

// Frames x layers grid with a tag bar and frame-number header; owns playback. Lives in a QScrollArea.
class Timeline : public QWidget
{
    Q_OBJECT
public:
    // `settle` drops floating pixels before a frame op; `select` is MainWindow's single path for changing layer/frame.
    Timeline(CanvasView *canvas, std::function<void()> settle, std::function<void(int layer, int frame)> select, QWidget *parent = nullptr);
    void setDocument(spx::Document *doc);
    QToolBar *toolBar() const { return m_bar; } // MainWindow places it above the scroll area and adds the onion controls
    void pause();

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;
    void contextMenuEvent(QContextMenuEvent *) override;
    bool event(QEvent *) override; // tooltips

private:
    enum Zone { TagBar, Header, Cells };
    struct Hit { Zone zone; int frame, layer, tag; }; // -1 = none (name column, past the end, no tag)
    Hit hitTest(QPoint) const;
    int frame() const;
    std::pair<int, int> selection() const; // selected header range, clamped
    std::pair<int, int> playRange() const;
    void togglePlay(bool on);
    void tick();
    void step(int delta);
    void frameOp(const QString &text, int newCurrent, const std::function<spx::Sprite(spx::Sprite)> &op);
    void editDurations(int frame);
    void editTag(int index);
    void newTag();

    CanvasView *m_canvas;
    std::function<void()> m_settle;
    std::function<void(int, int)> m_select;
    spx::Document *m_doc = nullptr;
    QMetaObject::Connection m_conn;
    QToolBar *m_bar;
    QAction *m_play, *m_loop, *m_tagOnly;
    QTimer m_timer;
    int m_anchor = 0, m_selEnd = 0; // header range selection
    bool m_dragHeader = false;
};
