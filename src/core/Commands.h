#pragma once
#include "Document.h"
#include <QUndoCommand>

namespace spx {

class SnapshotCommand : public QUndoCommand
{
public:
    SnapshotCommand(Document *d, const QString &text, Sprite before, Sprite after);
    void undo() override;
    void redo() override;
private:
    Document *m_doc;
    Sprite m_before, m_after;
};

class CelPixelsCommand : public QUndoCommand
{
public:
    CelPixelsCommand(Document *d, const QString &text, int layer, int frame, QRect rect, QImage before, QImage after);
    void undo() override;
    void redo() override;
private:
    void paint(const QImage &img);
    Document *m_doc;
    int m_layer, m_frame;
    QRect m_rect;
    QImage m_before, m_after;
    bool m_first = true; // the stroke already painted the cel
};

} // namespace spx
