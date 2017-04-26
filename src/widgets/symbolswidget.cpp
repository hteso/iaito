#include "symbolswidget.h"
#include "ui_symbolswidget.h"

#include "mainwindow.h"

SymbolsWidget::SymbolsWidget(MainWindow *main, QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::SymbolsWidget)
{
    ui->setupUi(this);
    this->main = main;
    this->symbolsTreeWidget = ui->symbolsTreeWidget;
}

SymbolsWidget::~SymbolsWidget()
{
    delete ui;
}

void SymbolsWidget::fillSymbols()
{
    this->symbolsTreeWidget->clear();
    for (auto symbol : this->main->core->getAllSymbols())
    {
        QTreeWidgetItem *item = this->main->appendRow(this->symbolsTreeWidget,
                                                      RAddressString(symbol.vaddr),
                                                      QString("%1 %2").arg(symbol.bind, symbol.type).trimmed(),
                                                      symbol.name);

        item->setData(0, Qt::UserRole, QVariant::fromValue(symbol));
    }
    this->main->adjustColumns(this->symbolsTreeWidget);
}

void SymbolsWidget::on_symbolsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    QNOTUSED(column);

    // Get offset and name of item double clicked
    SymbolDescription symbol = item->data(0, Qt::UserRole).value<SymbolDescription>();
    this->main->seek(symbol.vaddr, symbol.name);
}
