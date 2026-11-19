#include "MainWindow.h"
#include "CanvasView.h"
#include "ColorPicker.h"
#include "LayersPanel.h"
#include "PalettePanel.h"
#include "ResizeCanvasDialog.h"
#include "Selection.h"
#include "SpriteOps.h"
#include "tools/EyedropperTool.h"
#include "tools/FillTool.h"
#include "tools/PencilTool.h"
#include "tools/SelectTool.h"
#include "tools/ShapeTool.h"
#include <QActionGroup>
#include <QClipboard>
#include <QDockWidget>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QGuiApplication>
#include <QLabel>
#include <QMenuBar>
#include <QSpinBox>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>

using namespace spx;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), m_canvas(new CanvasView)
{
    setCentralWidget(m_canvas);
    createToolBar();
    createDocks();
    createMenus();

    auto *zoomLabel = new QLabel;
    statusBar()->addPermanentWidget(zoomLabel);
    auto showZoom = [zoomLabel](int z) { zoomLabel->setText(tr("Zoom %1%").arg(z * 100)); };
    connect(m_canvas, &CanvasView::zoomChanged, this, showZoom);
    showZoom(m_canvas->zoom());
    connect(&m_undo, &QUndoGroup::cleanChanged, this, [this](bool clean) { setWindowModified(!clean); });

    setDocument(std::make_unique<Document>(makeSprite({32, 32})));
    resize(1280, 800);
    m_canvas->setFocus();
}

MainWindow::~MainWindow()
{
    m_canvas->setDocument(nullptr); // the canvas outlives m_doc
}

void MainWindow::createToolBar()
{
    auto *bar = new QToolBar(tr("Tools"));
    bar->setObjectName(QStringLiteral("tools"));
    bar->setMovable(false);
    bar->setIconSize({20, 20});
    addToolBar(Qt::LeftToolBarArea, bar);
    auto *group = new QActionGroup(this);

    auto addTool = [&](const char *icon, const QString &name, const QString &key, std::unique_ptr<Tool> tool) {
        Tool *t = tool.get();
        m_tools.push_back(std::move(tool));
        QAction *a = bar->addAction(QIcon(QStringLiteral(":/icons/%1.svg").arg(QLatin1String(icon))), name);
        a->setCheckable(true);
        a->setActionGroup(group);
        a->setToolTip(tr("%1 (%2)").arg(name, key));
        if (key.size() == 1) a->setShortcut(QKeySequence(key));
        connect(a, &QAction::triggered, this, [this, t] { settleSelection(); m_canvas->setTool(t); });
        return a;
    };
    QAction *pencil = addTool("pencil", tr("Pencil"), QStringLiteral("B"), std::make_unique<PencilTool>(false));
    addTool("eraser", tr("Eraser"), QStringLiteral("E"), std::make_unique<PencilTool>(true));
    addTool("fill", tr("Fill"), QStringLiteral("G"), std::make_unique<FillTool>());
    addTool("eyedropper", tr("Eyedropper"), QStringLiteral("I"), std::make_unique<EyedropperTool>());
    m_selectTool = addTool("select", tr("Select"), QStringLiteral("M"), std::make_unique<SelectTool>(&m_canvas->selection()));
    addTool("line", tr("Line"), QStringLiteral("L"), std::make_unique<ShapeTool>(ShapeTool::Line));
    QAction *rect = addTool("rect", tr("Rectangle"), QStringLiteral("U"), std::make_unique<ShapeTool>(ShapeTool::Rect));
    QAction *ellipse = addTool("ellipse", tr("Ellipse"), QStringLiteral("U"), std::make_unique<ShapeTool>(ShapeTool::Ellipse));
    rect->setShortcut({});
    ellipse->setShortcut({});
    auto *cycle = new QAction(this);
    cycle->setShortcut(Qt::Key_U);
    addAction(cycle);
    connect(cycle, &QAction::triggered, this, [rect, ellipse] { (rect->isChecked() ? ellipse : rect)->trigger(); });
    pencil->setChecked(true);
    m_canvas->setTool(m_tools.front().get());

    bar->addSeparator();
    ToolContext &ctx = m_canvas->context();
    auto addToggle = [&](const char *icon, const QString &name, bool *flag) {
        QAction *a = bar->addAction(QIcon(QStringLiteral(":/icons/%1.svg").arg(QLatin1String(icon))), name);
        a->setCheckable(true);
        a->setChecked(*flag);
        connect(a, &QAction::toggled, this, [flag](bool on) { *flag = on; });
    };
    addToggle("pixel-perfect", tr("Pixel-perfect strokes"), &ctx.pixelPerfect);
    addToggle("mirror-h", tr("Mirror horizontally"), &ctx.mirrorH);
    addToggle("mirror-v", tr("Mirror vertically"), &ctx.mirrorV);
    addToggle("filled", tr("Filled shapes"), &ctx.filled);
}

void MainWindow::createDocks()
{
    m_picker = new ColorPicker(m_canvas);
    auto *palette = new PalettePanel(m_canvas);
    auto *layers = new LayersPanel(m_canvas, [this] { settleSelection(); });
    connect(palette, &PalettePanel::colorsChanged, m_picker, &ColorPicker::refresh);
    connect(this, &MainWindow::documentChanged, palette, &PalettePanel::setDocument);
    connect(this, &MainWindow::documentChanged, layers, &LayersPanel::setDocument);
    const auto dock = [this](const QString &title, const char *name, QWidget *w) {
        auto *d = new QDockWidget(title, this);
        d->setObjectName(QLatin1String(name));
        d->setFeatures(d->features() & ~QDockWidget::DockWidgetClosable); // no View menu to bring it back
        d->setWidget(w);
        addDockWidget(Qt::RightDockWidgetArea, d);
    };
    dock(tr("Color"), "color", m_picker);
    dock(tr("Palette"), "palette", palette);
    dock(tr("Layers"), "layers", layers);
}

void MainWindow::createMenus()
{
    QMenu *file = menuBar()->addMenu(tr("&File"));
    file->addAction(tr("&New…"), QKeySequence::New, this, &MainWindow::newSprite);
    file->addAction(tr("&Open…"), QKeySequence::Open, this, &MainWindow::openDialog);
    file->addAction(tr("&Save"), QKeySequence::Save, this, &MainWindow::save);
    file->addAction(tr("Save &As…"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S), this, &MainWindow::saveAs);
    file->addSeparator();
    file->addAction(tr("&Import PNG…"), this, &MainWindow::importPng);
    file->addAction(tr("Export &Frame PNG…"), this, &MainWindow::exportFrame);
    file->addAction(tr("&Export Sprite Sheet…"), QKeySequence(Qt::CTRL | Qt::Key_E), this, &MainWindow::exportSheet);
    file->addAction(tr("Import &Palette…"), this, &MainWindow::importPalette);
    file->addSeparator();
    file->addAction(tr("&Quit"), QKeySequence::Quit, this, &QWidget::close);

    QMenu *edit = menuBar()->addMenu(tr("&Edit"));
    QAction *undo = edit->addAction(tr("&Undo"), QKeySequence::Undo, this, [this] { settleSelection(true); m_undo.undo(); });
    QAction *redo = edit->addAction(tr("&Redo"), this, [this] { settleSelection(true); m_undo.redo(); });
    redo->setShortcuts({QKeySequence(Qt::CTRL | Qt::Key_Y), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z)});
    connect(&m_undo, &QUndoGroup::canUndoChanged, undo, &QAction::setEnabled);
    connect(&m_undo, &QUndoGroup::canRedoChanged, redo, &QAction::setEnabled);
    edit->addSeparator();
    edit->addAction(tr("S&wap Colors"), QKeySequence(Qt::Key_X), m_picker, &ColorPicker::swap);
    edit->addSeparator();
    edit->addAction(tr("Cu&t"), QKeySequence::Cut, this, &MainWindow::cut);
    edit->addAction(tr("&Copy"), QKeySequence::Copy, this, &MainWindow::copy);
    edit->addAction(tr("&Paste"), QKeySequence::Paste, this, &MainWindow::paste);
    edit->addSeparator();
    edit->addAction(tr("Select &All"), QKeySequence::SelectAll, this, [this] {
        settleSelection();
        m_canvas->selection().rect = QRect(QPoint(), m_doc->sprite().size);
        m_canvas->update();
    });
    edit->addAction(tr("&Deselect"), QKeySequence(Qt::CTRL | Qt::Key_D), this, [this] {
        settleSelection();
        m_canvas->selection().rect = QRect();
        m_canvas->update();
    });
    edit->addSeparator();
    edit->addAction(tr("Flip &Horizontal"), this, [this] { transformOp(tr("Flip Horizontal"), QTransform::fromScale(-1, 1)); });
    edit->addAction(tr("Flip &Vertical"), this, [this] { transformOp(tr("Flip Vertical"), QTransform::fromScale(1, -1)); });
    edit->addAction(tr("Rotate 90° C&W"), this, [this] { transformOp(tr("Rotate 90° CW"), QTransform().rotate(90)); });
    edit->addAction(tr("Rotate 90° CC&W"), this, [this] { transformOp(tr("Rotate 90° CCW"), QTransform().rotate(-90)); });
    edit->addAction(tr("Rotate &180°"), this, [this] { transformOp(tr("Rotate 180°"), QTransform().rotate(180)); });

    QMenu *sprite = menuBar()->addMenu(tr("&Sprite"));
    sprite->addAction(tr("&Resize Canvas…"), this, [this] {
        ResizeCanvasDialog d(m_doc->sprite().size, this);
        if (d.exec() == QDialog::Accepted)
            spriteOp(tr("Resize Canvas"), [&](Sprite s) { return resizeCanvas(std::move(s), d.canvasSize(), d.anchor()); });
    });
    sprite->addAction(tr("&Crop to Selection"), this, [this] {
        settleSelection();
        const QRect r = m_canvas->selection().rect;
        if (!r.isEmpty()) spriteOp(tr("Crop to Selection"), [r](Sprite s) { return cropCanvas(std::move(s), r); });
    });
    sprite->addSeparator();
    sprite->addAction(tr("Flip Sprite &Horizontal"), this, [this] {
        spriteOp(tr("Flip Sprite Horizontal"), [](Sprite s) { return flipSprite(std::move(s), Qt::Horizontal); });
    });
    sprite->addAction(tr("Flip Sprite &Vertical"), this, [this] {
        spriteOp(tr("Flip Sprite Vertical"), [](Sprite s) { return flipSprite(std::move(s), Qt::Vertical); });
    });
    const std::pair<QString, int> rotations[] = {{tr("Rotate Sprite 90° CW"), 90}, {tr("Rotate Sprite 90° CCW"), 270}, {tr("Rotate Sprite 180°"), 180}};
    for (const auto &[label, deg] : rotations)
        sprite->addAction(label, this, [this, label, deg] { spriteOp(label, [deg](Sprite s) { return rotateSprite(std::move(s), deg); }); });
    sprite->addSeparator();
    sprite->addAction(tr("S&lice Grid…"), this, &MainWindow::editSliceGrid);

    QMenu *view = menuBar()->addMenu(tr("&View"));
    QAction *grid = view->addAction(tr("Pixel &Grid"), QKeySequence(Qt::CTRL | Qt::Key_Apostrophe), m_canvas, &CanvasView::setGridVisible);
    grid->setCheckable(true);
    QAction *zoomIn = view->addAction(tr("Zoom &In"), this, [this] { m_canvas->setZoom(m_canvas->zoom() * 2); });
    zoomIn->setShortcuts({QKeySequence(Qt::Key_Plus), QKeySequence(Qt::Key_Equal), QKeySequence::ZoomIn});
    QAction *zoomOut = view->addAction(tr("Zoom &Out"), this, [this] { m_canvas->setZoom(m_canvas->zoom() / 2); });
    zoomOut->setShortcuts({QKeySequence(Qt::Key_Minus), QKeySequence::ZoomOut});
    QAction *onion = view->addAction(tr("&Onion Skin"), QKeySequence(Qt::Key_F3), this, [this](bool on) { m_canvas->setOnionSkin(on, 1, 1); });
    onion->setCheckable(true);
}

void MainWindow::setDocument(std::unique_ptr<Document> doc)
{
    m_canvas->setDocument(doc.get());
    m_undo.addStack(doc->undoStack());
    m_undo.setActiveStack(doc->undoStack());
    m_doc = std::move(doc); // the old document and its stack go away here
    emit documentChanged(m_doc.get());
    setWindowModified(m_doc->isModified());
    updateTitle();
}

void MainWindow::updateTitle()
{
    const QString name = m_doc->filePath.isEmpty() ? tr("Untitled") : QFileInfo(m_doc->filePath).fileName();
    setWindowTitle(tr("%1[*] — Sprixion").arg(name));
}

void MainWindow::settleSelection(bool discard)
{
    m_canvas->endStroke();
    Selection &s = m_canvas->selection();
    if (discard) s.floating = QImage(); // undoing the lift restores the original pixels
    else dropSelection(*m_doc, s, m_canvas->context().layer, m_canvas->context().frame);
    m_canvas->update();
}

void MainWindow::spriteOp(const QString &text, const std::function<Sprite(Sprite)> &op)
{
    settleSelection();
    m_canvas->selection() = {}; // the canvas may change size; an old rect would point at the wrong pixels
    m_doc->apply(text, op(m_doc->sprite()));
}

void MainWindow::transformOp(const QString &text, const QTransform &t)
{
    Selection &sel = m_canvas->selection();
    const ToolContext &c = m_canvas->context();
    if (sel.isActive()) { // an unlifted selection is lifted so only its pixels turn
        liftSelection(*m_doc, sel, c.layer, c.frame);
        transformSelection(sel, t);
        m_canvas->update();
    } else if (!m_doc->sprite().layers[c.layer].cels[c.frame].isNull()) {
        m_doc->apply(text, transformCel(m_doc->sprite(), c.layer, c.frame, t));
    }
}

void MainWindow::copy()
{
    const ToolContext &c = m_canvas->context();
    const QImage img = copySelection(m_doc->sprite(), m_canvas->selection(), c.layer, c.frame);
    if (!img.isNull()) QGuiApplication::clipboard()->setImage(img);
}

void MainWindow::cut()
{
    Selection &sel = m_canvas->selection();
    if (!sel.isActive()) return;
    copy();
    liftSelection(*m_doc, sel, m_canvas->context().layer, m_canvas->context().frame); // the one undo entry
    settleSelection(true);
}

void MainWindow::paste()
{
    const QImage img = QGuiApplication::clipboard()->image();
    if (img.isNull()) return;
    m_selectTool->trigger(); // before pasting: a tool switch drops floating pixels
    Selection &sel = m_canvas->selection();
    const ToolContext &c = m_canvas->context();
    pasteImage(*m_doc, sel, c.layer, c.frame, img, sel.isActive() ? sel.rect.topLeft() : QPoint());
    m_canvas->update();
}

void MainWindow::editSliceGrid()
{
    const SliceGrid g = m_doc->sprite().sliceGrid.value_or(SliceGrid{});
    QDialog d(this);
    d.setWindowTitle(tr("Slice Grid"));
    auto *box = new QGroupBox(tr("Slice into a grid"));
    box->setCheckable(true);
    box->setChecked(m_doc->sprite().sliceGrid.has_value());
    auto *form = new QFormLayout(box);
    auto spin = [&](const QString &label, int min, int value) {
        auto *s = new QSpinBox;
        s->setRange(min, 2048);
        s->setValue(value);
        s->setSuffix(tr(" px"));
        form->addRow(label, s);
        return s;
    };
    QSpinBox *cw = spin(tr("Cell width:"), 1, g.cellWidth), *ch = spin(tr("Cell height:"), 1, g.cellHeight);
    QSpinBox *ox = spin(tr("Offset X:"), 0, g.offsetX), *oy = spin(tr("Offset Y:"), 0, g.offsetY);
    QSpinBox *sx = spin(tr("Spacing X:"), 0, g.spacingX), *sy = spin(tr("Spacing Y:"), 0, g.spacingY);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &d, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &d, &QDialog::reject);
    auto *layout = new QVBoxLayout(&d);
    layout->addWidget(box);
    layout->addWidget(buttons);
    if (d.exec() != QDialog::Accepted) return;
    Sprite s = m_doc->sprite();
    if (box->isChecked()) s.sliceGrid = SliceGrid{cw->value(), ch->value(), ox->value(), oy->value(), sx->value(), sy->value()};
    else s.sliceGrid.reset();
    m_doc->apply(tr("Slice Grid"), std::move(s));
}
