#pragma once
#include "Document.h"
#include "tools/Tool.h"
#include <QMainWindow>
#include <QUndoGroup>
#include <functional>
#include <memory>
#include <vector>

class CanvasView;
class ColorPicker;
class Timeline;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
    bool openFile(const QString &path); // on failure shows the loader error; the current document stays
    bool save();
    bool saveAs();
    spx::Document *document() const { return m_doc.get(); }

signals:
    void documentChanged(spx::Document *doc); // after New/Open/Import; docks re-bind here
    void currentChanged();                     // the current layer or frame changed through setCurrent()

private:
    void createToolBar();
    void createMenus();
    void createDocks();
    void setDocument(std::unique_ptr<spx::Document> doc);
    void updateTitle();
    // The core ties floating pixels to neither undo nor a layer/frame: discard them before undo/redo,
    // drop them into the cel before switching layer/frame/tool, saving or exporting.
    void settleSelection(bool discard = false);
    void setCurrent(int layer, int frame); // the one path for user layer/frame changes (playback ticks bypass it)
    void spriteOp(const QString &text, const std::function<spx::Sprite(spx::Sprite)> &op);
    void transformOp(const QString &text, const QTransform &t);
    void cut();
    void copy();
    void paste();
    void editSliceGrid();
    // FileActions.cpp
    void newSprite();
    void openDialog();
    void importPng();
    void exportFrame();
    void exportSheet();
    void importPalette();
    QString suggestedPath(const QString &suffix) const;

    CanvasView *m_canvas;
    QUndoGroup m_undo;
    std::unique_ptr<spx::Document> m_doc;
    std::vector<std::unique_ptr<spx::Tool>> m_tools;
    QAction *m_selectTool = nullptr;
    ColorPicker *m_picker = nullptr;
    Timeline *m_timeline = nullptr;
};
