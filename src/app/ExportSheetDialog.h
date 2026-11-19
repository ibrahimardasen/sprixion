#pragma once
#include "io/SheetExport.h"
#include <QDialog>

class QComboBox;
class QLineEdit;
class QSpinBox;

class ExportSheetDialog : public QDialog
{
    Q_OBJECT
public:
    ExportSheetDialog(const QString &suggestedPath, QWidget *parent = nullptr);
    spx::SheetLayout sheetLayout() const;
    int columns() const;
    QString pngPath() const; // always ends in .png

private:
    QComboBox *m_layout;
    QSpinBox *m_columns;
    QLineEdit *m_path;
};
