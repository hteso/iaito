#ifndef XREFSDIALOG_H
#define XREFSDIALOG_H

#include "highlighter.h"
#include "qrcore.h"

#include <QDialog>
#include <QTreeWidgetItem>

class MainWindow;

namespace Ui
{
    class XrefsDialog;
}

class XrefsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit XrefsDialog(MainWindow *main, QWidget *parent = 0);
    ~XrefsDialog();

    void fillRefsForFunction(RVA addr, QString name);

private slots:

    void on_fromTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);

    void on_toTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);

    QString normalizeAddr(QString addr);

    void highlightCurrentLine();
    void on_fromTreeWidget_itemSelectionChanged();

    void on_toTreeWidget_itemSelectionChanged();

private:
    Ui::XrefsDialog *ui;
    MainWindow *main;

    Highlighter      *highlighter;

    void fillRefs(QList<XRefDescription> refs, QList<XRefDescription> xrefs);
    void updateLabels(QString name);

};

#endif // XREFSDIALOG_H
