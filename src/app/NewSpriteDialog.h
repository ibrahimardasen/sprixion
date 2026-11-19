#pragma once
#include <QDialog>
#include <QSize>

class QSpinBox;

class NewSpriteDialog : public QDialog
{
    Q_OBJECT
public:
    explicit NewSpriteDialog(QWidget *parent = nullptr);
    QSize spriteSize() const;

private:
    QSpinBox *m_w, *m_h;
};
