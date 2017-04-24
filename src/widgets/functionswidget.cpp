#include "functionswidget.h"
#include "ui_functionswidget.h"

#include "dialogs/commentsdialog.h"
#include "dialogs/renamedialog.h"
#include "dialogs/xrefsdialog.h"
#include "mainwindow.h"

#include <QMenu>
#include <QDebug>
#include <QString>

FunctionModel::FunctionModel(QList<RFunction> *functions, bool nested, QFont default_font, QFont highlight_font, MainWindow *main, QObject *parent)
        : functions(functions),
          main(main),
          nested(nested),
          default_font(default_font),
          highlight_font(highlight_font),
          QAbstractItemModel(parent)
{
    current_index = -1;

    connect(main, SIGNAL(cursorAddressChanged(RVA)), this, SLOT(cursorAddressChanged(RVA)));
}

QModelIndex FunctionModel::index(int row, int column, const QModelIndex &parent) const
{
    if(!parent.isValid())
        return createIndex(row, column, (quintptr)0); // root function nodes have id = 0

    return createIndex(row, column, (quintptr)(parent.row() + 1)); // sub-nodes have id = function index + 1
}

QModelIndex FunctionModel::parent(const QModelIndex &index) const
{
    if(!index.isValid() || index.column() != 0)
        return QModelIndex();

    if(index.internalId() == 0) // root function node
        return QModelIndex();
    else // sub-node
        return this->index((int)(index.internalId()-1), 0);
}

int FunctionModel::rowCount(const QModelIndex &parent) const
{
    if(!parent.isValid())
        return functions->count();

    if(nested)
    {
        if(parent.internalId() == 0)
            return 2;
        return 0;
    }
    else
        return 0;
}

int FunctionModel::columnCount(const QModelIndex &parent) const
{
    if(nested)
        return 1;
    else
        return 3;
}


QVariant FunctionModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    int function_index;
    bool subnode;
    if(index.internalId() != 0) // sub-node
    {
        function_index = index.parent().row();
        subnode = true;
    }
    else // root function node
    {
        function_index = index.row();
        subnode = false;
    }

    const RFunction &function = functions->at(function_index);

    if(function_index >= functions->count())
        return QVariant();

    switch(role)
    {
        case Qt::DisplayRole:
            if(nested)
            {
                if(subnode)
                {
                    switch(index.row())
                    {
                        case 0:
                            return tr("Offset: %1").arg(RAddressString(function.offset));
                        case 1:
                            return tr("Size: %1").arg(RSizeString(function.size));
                        default:
                            return QVariant();
                    }
                }
                else
                    return function.name;
            }
            else
            {
                switch(index.column())
                {
                    case 0:
                        return RAddressString(function.offset);
                    case 1:
                        return RSizeString(function.size);
                    case 2:
                        return function.name;
                    default:
                        return QVariant();
                }
            }

        case Qt::DecorationRole:
            //if(function.name.contains("imp") && index.column() == 2)
            //    return QColor(Qt::yellow);
            return QVariant();

        case Qt::FontRole:
            if(current_index == function_index)
                return highlight_font;
            return default_font;

        case Qt::ToolTipRole:
        {
            QList<QString> info = main->core->cmd("afi @ " + function.name).split("\n");
            if (info.length() > 2) {
                QString size = info[4].split(" ")[1];
                QString complex = info[8].split(" ")[1];
                QString bb = info[11].split(" ")[1];
                return QString("Summary:\n\n    Size: " + size +
                               "\n    Cyclomatic complexity: " + complex +
                               "\n    Basic blocks: " + bb +
                               "\n\nDisasm preview:\n\n" + main->core->cmd("pdi 10 @ " + function.name) +
                               "\nStrings:\n\n" + main->core->cmd("pdsf @ " + function.name));
            }
            return QVariant();
        }

        case Qt::UserRole:
            return QVariant::fromValue(function);

        default:
            return QVariant();
    }
}

QVariant FunctionModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch(section)
        {
            case 0:
                return tr("Offset");
            case 1:
                return tr("Size");
            case 2:
                return tr("Name");
            default:
                return QVariant();
        }
    }

    return QVariant();
}

void FunctionModel::cursorAddressChanged(RVA)
{
    updateCurrentIndex();
    emit dataChanged(index(0, 0), index(rowCount(), columnCount()));
}

void FunctionModel::updateCurrentIndex()
{
    RVA addr = main->getCursorAddress();

    int index = -1;
    RVA offset = 0;

    for(int i=0; i<functions->count(); i++)
    {
        const RFunction &function = functions->at(i);

        if(function.contains(addr)
           && function.offset >= offset)
        {
            offset = function.offset;
            index = i;
        }
    }

    current_index = index;
}



FunctionsWidget::FunctionsWidget(MainWindow *main, QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::FunctionsWidget)
{
    ui->setupUi(this);

    // Radare core found in:
    this->main = main;

    QFontInfo font_info = ui->functionsTreeView->fontInfo();
    QFont default_font = QFont(font_info.family(), font_info.pointSize());
    QFont highlight_font = QFont(font_info.family(), font_info.pointSize(), QFont::Bold);

    function_model = new FunctionModel(&functions, false, default_font, highlight_font, main, this);
    nested_function_model = new FunctionModel(&functions, true, default_font, highlight_font, main, this);

    //ui->functionsTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->functionsTreeView->setModel(function_model);
    ui->nestedFunctionsTreeView->setModel(nested_function_model);


    //this->functionsTreeWidget->setFont(QFont("Monospace", 8));
    // Set Functions context menu
    connect(ui->functionsTreeView, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showFunctionsContextMenu(const QPoint &)));
    connect(ui->nestedFunctionsTreeView, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showFunctionsContextMenu(const QPoint &)));

    connect(ui->functionsTreeView, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(on_functionsTreeView_itemDoubleClicked(const QModelIndex &)));
    connect(ui->nestedFunctionsTreeView, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(on_functionsTreeView_itemDoubleClicked(const QModelIndex &)));

    // Hide the tabs
    QTabBar *tabs = ui->tabWidget->tabBar();
    tabs->setVisible(false);

    // Use a custom context menu on the dock title bar
    //this->title_bar = this->titleBarWidget();
    ui->actionHorizontal->setChecked(true);
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showTitleContextMenu(const QPoint &)));

    connect(this->main->core, SIGNAL(offsetChanged(RVA)), this, SLOT(on_offsetChanged(RVA)));
    connect(this->main, SIGNAL(cursorAddressChanged(RVA)), this, SLOT(on_cursorAddressChanged(RVA)));
}

FunctionsWidget::~FunctionsWidget()
{
    delete ui;
}

void FunctionsWidget::fillFunctions()
{
    function_model->beginReload();
    nested_function_model->beginReload();
    functions = this->main->core->getAllFunctions();
    function_model->endReload();
    nested_function_model->endReload();

    // resize offset and size columns
    ui->functionsTreeView->resizeColumnToContents(0);
    ui->functionsTreeView->resizeColumnToContents(1);

    addTooltips();
}

void FunctionsWidget::on_functionsTreeView_itemDoubleClicked(const QModelIndex &index)
{
    RFunction function = index.data(Qt::UserRole).value<RFunction>();
    this->main->seek(function.offset, function.name);
    this->main->memoryDock->raise();
}

void FunctionsWidget::showFunctionsContextMenu(const QPoint &pt)
{
    // Set functions popup menu
    /*QMenu *menu = new QMenu(ui->functionsTreeWidget);
    menu->clear();
    menu->addAction(ui->actionDisasAdd_comment);
    menu->addAction(ui->actionFunctionsRename);
    menu->addAction(ui->actionFunctionsUndefine);
    menu->addSeparator();
    menu->addAction(ui->action_References);

    if (ui->tabWidget->currentIndex() == 0)
    {
        this->functionsTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
        menu->exec(ui->functionsTreeWidget->mapToGlobal(pt));
    }
    else
    {
        ui->nestedFunctionsTree->setContextMenuPolicy(Qt::CustomContextMenu);
        menu->exec(ui->nestedFunctionsTree->mapToGlobal(pt));
    }
    delete menu;*/
}

void FunctionsWidget::on_actionDisasAdd_comment_triggered()
{
    /*QString fcn_name = "";
    // Create dialog
    CommentsDialog *c = new CommentsDialog(this);
    // Get selected item in functions tree widget
    if (ui->tabWidget->currentIndex() == 0)
    {
        QList<QTreeWidgetItem *> selected_rows = ui->functionsTreeWidget->selectedItems();
        // Get selected function name
        fcn_name = selected_rows.first()->text(3);
    }
    else
    {
        QList<QTreeWidgetItem *> selected_rows = ui->nestedFunctionsTree->selectedItems();
        // Get selected function name
        fcn_name = selected_rows.first()->text(0);
    }
    if (c->exec())
    {
        // Get new function name
        QString comment = c->getComment();
        this->main->add_debug_output("Comment: " + comment + " at: " + fcn_name);
        // Rename function in r2 core
        this->main->core->setComment(fcn_name, comment);
        // Seek to new renamed function
        this->main->seek(fcn_name);
        // TODO: Refresh functions tree widget
    }
    this->main->refreshComments();*/
}

void FunctionsWidget::addTooltips()
{
/*
    // Add comments to list functions
    QList<QTreeWidgetItem *> clist = this->functionsTreeWidget->findItems("*", Qt::MatchWildcard, 3);
    foreach (QTreeWidgetItem *item, clist)
    {
        QString name = item->text(3);
        QList<QString> info = this->main->core->cmd("afi @ " + name).split("\n");
        if (info.length() > 2)
        {
            QString size = info[4].split(" ")[1];
            QString complex = info[8].split(" ")[1];
            QString bb = info[11].split(" ")[1];
            item->setToolTip(3, "Summary:\n\n    Size: " + size +
                             "\n    Cyclomatic complexity: " + complex +
                             "\n    Basic blocks: " + bb +
                             "\n\nDisasm preview:\n\n" + this->main->core->cmd("pdi 10 @ " + name) +
                             "\nStrings:\n\n" + this->main->core->cmd("pdsf @ " + name));
            //"\nStrings:\n\n" + this->main->core->cmd("pds @ " + name + "!$F"));
        }
    }

    // Add comments to nested functions
    QList<QTreeWidgetItem *> nlist = ui->nestedFunctionsTree->findItems("*", Qt::MatchWildcard, 0);
    foreach (QTreeWidgetItem *item, nlist)
    {
        QString name = item->text(0);
        QList<QString> info = this->main->core->cmd("afi @ " + name).split("\n");
        if (info.length() > 2)
        {
            QString size = info[4].split(" ")[1];
            QString complex = info[8].split(" ")[1];
            QString bb = info[11].split(" ")[1];
            item->setToolTip(0, "Summary:\n\n    Size: " + size +
                             "\n    Cyclomatic complexity: " + complex +
                             "\n    Basic blocks: " + bb +
                             "\n\nDisasm preview:\n\n" + this->main->core->cmd("pdi 10 @ " + name) +
                             "\nStrings:\n\n" + this->main->core->cmd("pdsf @ " + name));
            //"\nStrings:\n\n" + this->main->core->cmd("pds @ " + name + "!$F"));
        }
    }*/
}

void FunctionsWidget::on_actionFunctionsRename_triggered()
{
    /*// Create dialog
    RenameDialog *r = new RenameDialog(this);
    // Get selected item in functions tree widget
    QList<QTreeWidgetItem *> selected_rows = ui->functionsTreeWidget->selectedItems();
    // Get selected function name
    QString old_name = selected_rows.first()->text(3);
    // Set function name in dialog
    r->setFunctionName(old_name);
    // If user accepted
    if (r->exec())
    {
        // Get new function name
        QString new_name = r->getFunctionName();
        // Rename function in r2 core
        this->main->core->renameFunction(old_name, new_name);
        // Change name in functions tree widget
        selected_rows.first()->setText(3, new_name);
        // Scroll to show the new name in functions tree widget
        //
        // QAbstractItemView::EnsureVisible
        // QAbstractItemView::PositionAtTop
        // QAbstractItemView::PositionAtBottom
        // QAbstractItemView::PositionAtCenter
        //
        ui->functionsTreeWidget->scrollToItem(selected_rows.first(), QAbstractItemView::PositionAtTop);
        // Seek to new renamed function
        this->main->seek(new_name);
    }*/
}

void FunctionsWidget::on_action_References_triggered()
{
    /*QList<QTreeWidgetItem *> selected_rows = ui->functionsTreeWidget->selectedItems();
    // Get selected function address
    QString address = selected_rows.first()->text(1);

    //this->main->add_debug_output("Addr: " + address);

    // Get function for clicked offset
    RAnalFunction *fcn = this->main->core->functionAt(address.toLongLong(0, 16));

    XrefsDialog *x = new XrefsDialog(this->main, this);
    x->setWindowTitle("X-Refs for function " + QString::fromUtf8(fcn->name));

    // Get Refs and Xrefs
    QList<QStringList> ret_refs;
    QList<QStringList> ret_xrefs;

    // refs = calls q hace esa funcion
    QList<QString> refs = this->main->core->getFunctionRefs(fcn->addr, 'C');
    if (refs.size() > 0)
    {
        for (int i = 0; i < refs.size(); ++i)
        {
            //this->main->add_debug_output(refs.at(i));
            QStringList retlist = refs.at(i).split(",");
            QStringList temp;
            QString addr = retlist.at(2);
            temp << addr;
            QString op = this->main->core->cmd("pi 1 @ " + addr);
            temp << op.simplified();
            ret_refs << temp;
        }
    }

    // xrefs = calls a esa funcion
    //qDebug() << this->main->core->getFunctionXrefs(offset.toLong(&ok, 16));
    QList<QString> xrefs = this->main->core->getFunctionXrefs(fcn->addr);
    if (xrefs.size() > 0)
    {
        for (int i = 0; i < xrefs.size(); ++i)
        {
            //this->main->add_debug_output(xrefs.at(i));
            QStringList retlist = xrefs.at(i).split(",");
            QStringList temp;
            QString addr = retlist.at(1);
            temp << addr;
            QString op = this->main->core->cmd("pi 1 @ " + addr);
            temp << op.simplified();
            ret_xrefs << temp;
        }
    }
    x->fillRefs(ret_refs, ret_xrefs);
    x->exec();*/
}

void FunctionsWidget::showTitleContextMenu(const QPoint &pt)
{
    // Set functions popup menu
    QMenu *menu = new QMenu(this);
    menu->clear();
    menu->addAction(ui->actionHorizontal);
    menu->addAction(ui->actionVertical);

    if (ui->tabWidget->currentIndex() == 0)
    {
        ui->actionHorizontal->setChecked(true);
        ui->actionVertical->setChecked(false);
    }
    else
    {
        ui->actionVertical->setChecked(true);
        ui->actionHorizontal->setChecked(false);
    }

    this->setContextMenuPolicy(Qt::CustomContextMenu);

    menu->exec(this->mapToGlobal(pt));
    delete menu;
}

void FunctionsWidget::on_actionHorizontal_triggered()
{
    ui->tabWidget->setCurrentIndex(0);
}

void FunctionsWidget::on_actionVertical_triggered()
{
    ui->tabWidget->setCurrentIndex(1);
}

void FunctionsWidget::on_nestedFunctionsTree_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    QNOTUSED(column);

    //QString offset = item->text(1);
    QString name = item->text(0);
    QString offset = item->child(0)->text(0).split(":")[1];
    this->main->seek(offset, name);
    this->main->memoryDock->raise();
}

void FunctionsWidget::on_offsetChanged(RVA offset)
{
}

void FunctionsWidget::on_cursorAddressChanged(RVA address)
{
}

void FunctionsWidget::resizeEvent(QResizeEvent *event)
{
    if(main->responsive && isVisible())
    {
        if (event->size().width() >= event->size().height())
        {
            // Set horizontal view (list)
            on_actionHorizontal_triggered();
        }
        else
        {
            // Set vertical view (Tree)
            on_actionVertical_triggered();
        }
    }
    QDockWidget::resizeEvent(event);
}


