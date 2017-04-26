#include "functionswidget.h"
#include "ui_functionswidget.h"

#include "dialogs/commentsdialog.h"
#include "dialogs/renamedialog.h"
#include "dialogs/xrefsdialog.h"
#include "mainwindow.h"

#include <algorithm>

#include <QMenu>
#include <QDebug>
#include <QString>

FunctionModel::FunctionModel(bool nested, QFont default_font, QFont highlight_font, MainWindow *main, QObject *parent)
        : main(main),
          nested(nested),
          default_font(default_font),
          highlight_font(highlight_font),
          QAbstractItemModel(parent)
{
    current_index = -1;

    connect(main, SIGNAL(cursorAddressChanged(RVA)), this, SLOT(cursorAddressChanged(RVA)));
    connect(main->core, SIGNAL(functionRenamed(QString, QString)), this, SLOT(functionRenamed(QString, QString)));
}

void FunctionModel::setFunctions(QList<RFunction> functions)
{
    beginResetModel();
    this->functions = functions;
    endResetModel();
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
        return functions.count();

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

    const RFunction &function = functions.at(function_index);

    if(function_index >= functions.count())
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
        if(nested)
        {
            return tr("Name");
        }
        else
        {
            switch (section)
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

    for(int i=0; i<functions.count(); i++)
    {
        const RFunction &function = functions.at(i);

        if(function.contains(addr)
           && function.offset >= offset)
        {
            offset = function.offset;
            index = i;
        }
    }

    current_index = index;
}

void FunctionModel::functionRenamed(QString prev_name, QString new_name)
{
    for(int i=0; i<functions.count(); i++)
    {
        RFunction &function = functions[i];
        if(function.name == prev_name)
        {
            function.name = new_name;
            emit dataChanged(index(i, 0), index(i, columnCount()));
            return;
        }
    }
}




FunctionSortFilterProxyModel::FunctionSortFilterProxyModel(FunctionModel *source_model, QObject *parent)
        : QSortFilterProxyModel(parent)
{
    setSourceModel(source_model);
}

bool FunctionSortFilterProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    QModelIndex index = sourceModel()->index(row, 0, parent);
    RFunction function = index.data(Qt::UserRole).value<RFunction>();
    return function.name.contains(filterRegExp().pattern());
}


bool FunctionSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if(!left.isValid() || !right.isValid())
        return false;

    if(left.parent().isValid() || right.parent().isValid())
        return left.row() < right.row();

    RFunction left_function = left.data(Qt::UserRole).value<RFunction>();
    RFunction right_function = right.data(Qt::UserRole).value<RFunction>();

    if(static_cast<FunctionModel *>(sourceModel())->isNested())
    {
        return left_function.name < right_function.name;
    }
    else
    {
        switch (left.column())
        {
            case 1:
                if (left_function.size != right_function.size)
                    return left_function.size < right_function.size;
                // fallthrough
            case 0:
                return left_function.offset < right_function.offset;
            case 2:
                return left_function.name < right_function.name;
            default:
                return false;
        }
    }
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

    function_model = new FunctionModel(false, default_font, highlight_font, main, this);
    function_proxy_model = new FunctionSortFilterProxyModel(function_model, this);
    connect(ui->filterLineEdit, SIGNAL(textChanged(const QString &)), function_proxy_model, SLOT(setFilterWildcard(const QString &)));
    ui->functionsTreeView->setModel(function_proxy_model);

    nested_function_model = new FunctionModel(true, default_font, highlight_font, main, this);
    nested_function_proxy_model = new FunctionSortFilterProxyModel(nested_function_model, this);
    connect(ui->filterLineEdit, SIGNAL(textChanged(const QString &)), nested_function_proxy_model, SLOT(setFilterWildcard(const QString &)));
    ui->nestedFunctionsTreeView->setModel(nested_function_proxy_model);

    //ui->functionsTreeView->setContextMenuPolicy(Qt::CustomContextMenu);



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
}

FunctionsWidget::~FunctionsWidget()
{
    delete ui;
}

void FunctionsWidget::fillFunctions()
{
    functions = this->main->core->getAllFunctions();
    function_model->setFunctions(functions);
    nested_function_model->setFunctions(functions);

    // resize offset and size columns
    ui->functionsTreeView->resizeColumnToContents(0);
    ui->functionsTreeView->resizeColumnToContents(1);
}

QTreeView *FunctionsWidget::getCurrentTreeView()
{
    if (ui->tabWidget->currentIndex() == 0)
        return ui->functionsTreeView;
    else
        return ui->nestedFunctionsTreeView;
}

void FunctionsWidget::on_functionsTreeView_itemDoubleClicked(const QModelIndex &index)
{
    RFunction function = index.data(Qt::UserRole).value<RFunction>();
    this->main->seek(function.offset, function.name);
    this->main->memoryDock->raise();
}

void FunctionsWidget::showFunctionsContextMenu(const QPoint &pt)
{
    QTreeView *treeView = getCurrentTreeView();

    // Set functions popup menu
    QMenu *menu = new QMenu(ui->functionsTreeView);
    menu->clear();
    menu->addAction(ui->actionDisasAdd_comment);
    menu->addAction(ui->actionFunctionsRename);
    menu->addAction(ui->actionFunctionsUndefine);
    menu->addSeparator();
    menu->addAction(ui->action_References);

    treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    menu->exec(treeView->mapToGlobal(pt));

    delete menu;
}

void FunctionsWidget::on_actionDisasAdd_comment_triggered()
{
    // Get selected item in functions tree view
    QTreeView *treeView = getCurrentTreeView();
    RFunction function = treeView->selectionModel()->currentIndex().data(Qt::UserRole).value<RFunction>();

    // Create dialog
    CommentsDialog *c = new CommentsDialog(this);

    if (c->exec())
    {
        // Get new function name
        QString comment = c->getComment();
        this->main->add_debug_output("Comment: " + comment + " at: " + function.name);
        // Rename function in r2 core
        this->main->core->setComment(function.offset, comment);
        // Seek to new renamed function
        this->main->seek(function.offset, function.name);
        // TODO: Refresh functions tree widget
    }
    this->main->refreshComments();
}


void FunctionsWidget::on_actionFunctionsRename_triggered()
{
    // Get selected item in functions tree view
    QTreeView *treeView = getCurrentTreeView();
    RFunction function = treeView->selectionModel()->currentIndex().data(Qt::UserRole).value<RFunction>();

    // Create dialog
    RenameDialog *r = new RenameDialog(this);

    // Set function name in dialog
    r->setFunctionName(function.name);
    // If user accepted
    if (r->exec())
    {
        // Get new function name
        QString new_name = r->getFunctionName();
        // Rename function in r2 core
        this->main->core->renameFunction(function.name, new_name);

        // Scroll to show the new name in functions tree widget
        //
        // QAbstractItemView::EnsureVisible
        // QAbstractItemView::PositionAtTop
        // QAbstractItemView::PositionAtBottom
        // QAbstractItemView::PositionAtCenter
        //
        //ui->functionsTreeWidget->scrollToItem(selected_rows.first(), QAbstractItemView::PositionAtTop);
        // Seek to new renamed function
        this->main->seek(new_name);
    }
}

void FunctionsWidget::on_action_References_triggered()
{
    // Get selected item in functions tree view
    QTreeView *treeView = getCurrentTreeView();
    RFunction function = treeView->selectionModel()->currentIndex().data(Qt::UserRole).value<RFunction>();

    //this->main->add_debug_output("Addr: " + address);

    // Get function for clicked offset
    RAnalFunction *fcn = this->main->core->functionAt(function.offset);

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
    x->exec();
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


