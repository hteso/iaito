#include "stringswidget.h"
#include "ui_stringswidget.h"

#include "dialogs/xrefsdialog.h"
#include "mainwindow.h"

StringsWidget::StringsWidget(MainWindow *main, QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::StringsWidget)
{
    ui->setupUi(this);

    // Radare core found in:
    this->main = main;
    this->stringsTreeWidget = ui->stringsTreeWidget;
}

StringsWidget::~StringsWidget()
{
    delete ui;
}

void StringsWidget::on_stringsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    QNOTUSED(column);

    // Get offset and name of item double clicked
    // TODO: use this info to change disasm contents
    StringDescription str = item->data(0, Qt::UserRole).value<StringDescription>();
    this->main->seek(str.vaddr, NULL, true);
}

void StringsWidget::fillStrings()
{
    stringsTreeWidget->clear();

    for (auto i : main->core->getAllStrings())
    {
        QTreeWidgetItem *item = main->appendRow(stringsTreeWidget, RAddressString(i.vaddr), i.string);
        item->setData(0, Qt::UserRole, QVariant::fromValue(i));
    }

    main->adjustColumns(stringsTreeWidget);
}