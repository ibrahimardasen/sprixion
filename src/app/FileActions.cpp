// MainWindow: file commands (new, open, save, import, export).
#include "MainWindow.h"
#include "CanvasView.h"
#include "Compositor.h"
#include "ExportSheetDialog.h"
#include "NewSpriteDialog.h"
#include "Selection.h"
#include "io/PaletteIO.h"
#include "io/SheetExport.h"
#include "io/SprxIO.h"
#include <QBuffer>
#include <QCoreApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QSaveFile>

using namespace spx;

static bool writeAtomic(const QString &path, const QByteArray &data, QString *error)
{
    if (data.isEmpty()) { // encoding failed: never replace a good file with nothing
        *error = QCoreApplication::translate("MainWindow", "The image could not be encoded.");
        return false;
    }
    QSaveFile f(path);
    if (f.open(QIODevice::WriteOnly) && f.write(data) == data.size() && f.commit()) return true;
    *error = f.errorString();
    return false;
}

static QByteArray pngBytes(const QImage &img)
{
    QByteArray bytes;
    QBuffer buf(&bytes);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf, "PNG");
    return bytes;
}

QString MainWindow::suggestedPath(const QString &suffix) const
{
    if (m_doc->filePath.isEmpty()) return tr("untitled") + suffix;
    const QFileInfo fi(m_doc->filePath);
    return fi.path() + QLatin1Char('/') + fi.completeBaseName() + suffix;
}

void MainWindow::newSprite()
{
    NewSpriteDialog d(this);
    if (d.exec() == QDialog::Accepted) setDocument(std::make_unique<Document>(makeSprite(d.spriteSize())));
}

void MainWindow::openDialog()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Open Sprite"), QString(), tr("Sprixion sprite (*.sprx)"));
    if (!path.isEmpty()) openFile(path);
}

bool MainWindow::openFile(const QString &path)
{
    QString error;
    std::optional<Sprite> s = loadSprx(path, &error);
    if (!s) {
        QMessageBox::critical(this, tr("Open Failed"), tr("Could not open %1:\n%2").arg(QFileInfo(path).fileName(), error));
        return false;
    }
    auto doc = std::make_unique<Document>(std::move(*s));
    doc->filePath = path;
    setDocument(std::move(doc));
    return true;
}

bool MainWindow::save()
{
    if (m_doc->filePath.isEmpty()) return saveAs();
    settleSelection();
    QString error;
    if (!saveSprx(m_doc->sprite(), m_doc->filePath, &error)) {
        QMessageBox::critical(this, tr("Save Failed"), tr("Could not save %1:\n%2").arg(QFileInfo(m_doc->filePath).fileName(), error));
        return false;
    }
    m_doc->undoStack()->setClean();
    return true;
}

bool MainWindow::saveAs()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Save Sprite"), suggestedPath(QStringLiteral(".sprx")), tr("Sprixion sprite (*.sprx)"));
    if (path.isEmpty()) return false;
    if (!path.endsWith(QLatin1String(".sprx"), Qt::CaseInsensitive)) path += QLatin1String(".sprx");
    const QString old = m_doc->filePath;
    m_doc->filePath = path;
    if (!save()) { m_doc->filePath = old; return false; }
    updateTitle();
    return true;
}

void MainWindow::importPng()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Import PNG"), QString(), tr("PNG image (*.png)"));
    if (path.isEmpty()) return;
    const QImage img(path);
    if (img.isNull() || img.width() > 2048 || img.height() > 2048) {
        QMessageBox::critical(this, tr("Import Failed"),
                              img.isNull() ? tr("%1 is not a readable PNG image.").arg(QFileInfo(path).fileName())
                                           : tr("The image is %1×%2 pixels; Sprixion supports up to 2048×2048.").arg(img.width()).arg(img.height()));
        return;
    }
    Sprite s = makeSprite(img.size());
    celForWrite(s, 0, 0) = img.convertToFormat(QImage::Format_ARGB32);
    setDocument(std::make_unique<Document>(std::move(s)));
}

void MainWindow::exportFrame()
{
    settleSelection();
    QString path = QFileDialog::getSaveFileName(this, tr("Export Frame"), suggestedPath(QStringLiteral(".png")), tr("PNG image (*.png)"));
    if (path.isEmpty()) return;
    if (!path.endsWith(QLatin1String(".png"), Qt::CaseInsensitive)) path += QLatin1String(".png");
    QString error;
    if (!writeAtomic(path, pngBytes(compositeFrame(m_doc->sprite(), m_canvas->context().frame)), &error))
        QMessageBox::critical(this, tr("Export Failed"), tr("Could not write %1:\n%2").arg(QFileInfo(path).fileName(), error));
}

void MainWindow::exportSheet()
{
    settleSelection();
    ExportSheetDialog d(suggestedPath(QStringLiteral(".png")), this);
    if (d.exec() != QDialog::Accepted) return;
    const QFileInfo png(d.pngPath());
    const QString jsonPath = png.path() + QLatin1Char('/') + png.completeBaseName() + QLatin1String(".json");
    const SheetResult r = spx::exportSheet(m_doc->sprite(), d.sheetLayout(), d.columns(), png.fileName());
    QString error;
    if (!writeAtomic(png.filePath(), pngBytes(r.image), &error) || !writeAtomic(jsonPath, r.json.toJson(), &error))
        QMessageBox::critical(this, tr("Export Failed"), tr("Could not export the sprite sheet:\n%1").arg(error));
}

void MainWindow::importPalette()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Import Palette"), QString(), tr("Palettes (*.gpl *.hex)"));
    if (path.isEmpty()) return;
    QString error;
    const std::optional<QVector<QColor>> colors = loadPalette(path, &error);
    if (!colors) {
        QMessageBox::critical(this, tr("Import Failed"), tr("Could not import %1:\n%2").arg(QFileInfo(path).fileName(), error));
        return;
    }
    Sprite s = m_doc->sprite();
    s.palette = *colors;
    m_doc->apply(tr("Import Palette"), std::move(s));
}
