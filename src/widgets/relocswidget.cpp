#include "relocswidget.h"
#include "ui_relocswidget.h"

#include "mainwindow.h"

RelocsWidget::RelocsWidget(MainWindow *main, QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::RelocsWidget)
{
    ui->setupUi(this);

    // Radare core found in:
    this->main = main;

    this->relocsTreeWidget = ui->relocsTreeWidget;
    relocsTreeWidget->setColumnHidden(0, true);
}

RelocsWidget::~RelocsWidget()
{
    delete ui;
}

void RelocsWidget::on_relocsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    QNOTUSED(column);

    // Get offset and name of item double clicked
    RelocDescription reloc = item->data(0, Qt::UserRole).value<RelocDescription>();
    main->seek(reloc.vaddr, reloc.name);
}

void RelocsWidget::fillRelocs()
{
    relocsTreeWidget->clear();

    for (auto i : main->core->getAllRelocs())
    {
        QTreeWidgetItem *item = main->appendRow(relocsTreeWidget, RAddressString(i.vaddr), i.type, i.name);
        item->setData(0, Qt::UserRole, QVariant::fromValue(i));
    }

    main->adjustColumns(relocsTreeWidget);
}