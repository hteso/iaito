#include "functionswidget.h"
#include "ui_functionswidget.h"

#include "mainwindow.h"
#include "helpers.h"
#include "dialogs/commentsdialog.h"
#include "dialogs/renamedialog.h"
#include "dialogs/xrefsdialog.h"

#include <algorithm>
#include <QMenu>
#include <QDebug>
#include <QString>
#include <QResource>
#include <QShortcut>

FunctionModel::FunctionModel(QList<FunctionDescription> *functions, QSet<RVA> *import_addresses, bool nested, QFont default_font, QFont highlight_font, MainWindow *main, QObject *parent)
    : QAbstractItemModel(parent),
      main(main),
      functions(functions),
      import_addresses(import_addresses),
      highlight_font(highlight_font),
      default_font(default_font),
      nested(nested),
      current_index(-1)

{
    connect(main, SIGNAL(cursorAddressChanged(RVA)), this, SLOT(cursorAddressChanged(RVA)));
    connect(main->core, SIGNAL(functionRenamed(QString, QString)), this, SLOT(functionRenamed(QString, QString)));
}

QModelIndex FunctionModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid())
        return createIndex(row, column, (quintptr)0); // root function nodes have id = 0

    return createIndex(row, column, (quintptr)(parent.row() + 1)); // sub-nodes have id = function index + 1
}

QModelIndex FunctionModel::parent(const QModelIndex &index) const
{
    if (!index.isValid() || index.column() != 0)
        return QModelIndex();

    if (index.internalId() == 0) // root function node
        return QModelIndex();
    else // sub-node
        return this->index((int)(index.internalId() - 1), 0);
}

int FunctionModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return functions->count();

    if (nested)
    {
        if (parent.internalId() == 0)
            return 3; // sub-nodes for nested functions
        return 0;
    }
    else
        return 0;
}

int FunctionModel::columnCount(const QModelIndex &/*parent*/) const
{
    if (nested)
        return 1;
    else
        return 4;
}


QVariant FunctionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    int function_index;
    bool subnode;
    if (index.internalId() != 0) // sub-node
    {
        function_index = index.parent().row();
        subnode = true;
    }
    else // root function node
    {
        function_index = index.row();
        subnode = false;
    }

    const FunctionDescription &function = functions->at(function_index);

    if (function_index >= functions->count())
        return QVariant();

    switch (role)
    {
    case Qt::DisplayRole:
        if (nested)
        {
            if (subnode)
            {
                switch (index.row())
                {
                case 0:
                    return tr("Offset: %1").arg(RAddressString(function.offset));
                case 1:
                    return tr("Size: %1").arg(RSizeString(function.size));
                case 2:
                    return tr("Import: %1").arg(import_addresses->contains(function.offset) ? tr("true") : tr("false"));
                default:
                    return QVariant();
                }
            }
            else
                return function.name;
        }
        else
        {
            switch (index.column())
            {
            case 0:
                return RAddressString(function.offset);
            case 1:
                return RSizeString(function.size);
            case 3:
                return function.name;
            default:
                return QVariant();
            }
        }

    case Qt::DecorationRole:
        if (import_addresses->contains(function.offset) &&
                (nested ? false : index.column() == 2))
            return QIcon(":/img/icons/import_light.svg");
        return QVariant();

    case Qt::FontRole:
        if (current_index == function_index)
            return highlight_font;
        return default_font;

    case Qt::ToolTipRole:
    {
        QList<QString> info = main->core->cmd("afi @ " + function.name).split("\n");
        if (info.length() > 2)
        {
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

    case FunctionDescriptionRole:
        return QVariant::fromValue(function);

    case IsImportRole:
        return import_addresses->contains(function.offset);

    default:
        return QVariant();
    }
}

QVariant FunctionModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        if (nested)
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
                return tr("Imp.");
            case 3:
                return tr("Name");
            default:
                return QVariant();
            }
        }
    }

    return QVariant();
}

void FunctionModel::beginReloadFunctions()
{
    beginResetModel();
}

void FunctionModel::endReloadFunctions()
{
    updateCurrentIndex();
    endResetModel();
}

void FunctionModel::cursorAddressChanged(RVA)
{
    updateCurrentIndex();
    emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
}

void FunctionModel::updateCurrentIndex()
{
    RVA addr = main->getCursorAddress();

    int index = -1;
    RVA offset = 0;

    for (int i = 0; i < functions->count(); i++)
    {
        const FunctionDescription &function = functions->at(i);

        if (function.contains(addr)
                && function.offset >= offset)
        {
            offset = function.offset;
            index = i;
        }
    }

    current_index = index;
}

void FunctionModel::functionRenamed(const QString &prev_name, const QString &new_name)
{
    for (int i = 0; i < functions->count(); i++)
    {
        FunctionDescription &function = (*functions)[i];
        if (function.name == prev_name)
        {
            function.name = new_name;
            emit dataChanged(index(i, 0), index(i, columnCount() - 1));
        }
    }
}




FunctionSortFilterProxyModel::FunctionSortFilterProxyModel(FunctionModel *source_model, QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(source_model);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

bool FunctionSortFilterProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    QModelIndex index = sourceModel()->index(row, 0, parent);
    FunctionDescription function = index.data(FunctionModel::FunctionDescriptionRole).value<FunctionDescription>();
    return function.name.contains(filterRegExp());
}


bool FunctionSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if (!left.isValid() || !right.isValid())
        return false;

    if (left.parent().isValid() || right.parent().isValid())
        return false;

    FunctionDescription left_function = left.data(FunctionModel::FunctionDescriptionRole).value<FunctionDescription>();
    FunctionDescription right_function = right.data(FunctionModel::FunctionDescriptionRole).value<FunctionDescription>();


    if (static_cast<FunctionModel *>(sourceModel())->isNested())
    {
        return left_function.name < right_function.name;
    }
    else
    {
        switch (left.column())
        {
        case 0:
            return left_function.offset < right_function.offset;
        case 1:
            if (left_function.size != right_function.size)
                return left_function.size < right_function.size;
            break;
        case 2:
        {
            bool left_is_import = left.data(FunctionModel::IsImportRole).toBool();
            bool right_is_import = right.data(FunctionModel::IsImportRole).toBool();
            if (!left_is_import && right_is_import)
                return true;
            break;
        }
        case 3:
            return left_function.name < right_function.name;
        default:
            return false;
        }

        return left_function.offset < right_function.offset;
    }
}






FunctionsWidget::FunctionsWidget(MainWindow *main, QWidget *parent) :
    DockWidget(parent),
    ui(new Ui::FunctionsWidget),
    main(main)
{
    ui->setupUi(this);

    // Radare core found in:
    this->main = main;

    // leave the filter visible by default so users know it exists
    //ui->filterLineEdit->setVisible(false);

    // Ctrl-F to show/hide the filter entry
    QShortcut *search_shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F), this);
    connect(search_shortcut, SIGNAL(activated()), this, SLOT(show_filter()));
    search_shortcut->setContext(Qt::WidgetWithChildrenShortcut);

    // Esc to clear the filter entry
    QShortcut *clear_shortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(clear_shortcut, SIGNAL(activated()), this, SLOT(clear_filter()));
    clear_shortcut->setContext(Qt::WidgetWithChildrenShortcut);

    QFontInfo font_info = ui->functionsTreeView->fontInfo();
    QFont default_font = QFont(font_info.family(), font_info.pointSize());
    QFont highlight_font = QFont(font_info.family(), font_info.pointSize(), QFont::Bold);

    function_model = new FunctionModel(&functions, &import_addresses, false, default_font, highlight_font, main, this);
    function_proxy_model = new FunctionSortFilterProxyModel(function_model, this);
    connect(ui->filterLineEdit, SIGNAL(textChanged(const QString &)), function_proxy_model, SLOT(setFilterWildcard(const QString &)));
    ui->functionsTreeView->setModel(function_proxy_model);

    nested_function_model = new FunctionModel(&functions, &import_addresses, true, default_font, highlight_font, main, this);
    nested_function_proxy_model = new FunctionSortFilterProxyModel(nested_function_model, this);
    connect(ui->filterLineEdit, SIGNAL(textChanged(const QString &)), nested_function_proxy_model, SLOT(setFilterWildcard(const QString &)));
    ui->nestedFunctionsTreeView->setModel(nested_function_proxy_model);


    // Set Functions context menu
    connect(ui->functionsTreeView, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showFunctionsContextMenu(const QPoint &)));
    connect(ui->nestedFunctionsTreeView, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showFunctionsContextMenu(const QPoint &)));


    connect(ui->functionsTreeView, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(functionsTreeView_doubleClicked(const QModelIndex &)));
    connect(ui->nestedFunctionsTreeView, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(functionsTreeView_doubleClicked(const QModelIndex &)));

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

void FunctionsWidget::setup()
{
    setScrollMode();
    refreshTree();
}

void FunctionsWidget::refresh()
{
    setup();
}

void FunctionsWidget::refreshTree()
{
    function_model->beginReloadFunctions();
    nested_function_model->beginReloadFunctions();

    functions = this->main->core->getAllFunctions();

    import_addresses.clear();
    foreach (ImportDescription import, main->core->getAllImports())
        import_addresses.insert(import.plt);

    function_model->endReloadFunctions();
    nested_function_model->endReloadFunctions();

    // resize offset and size columns
    ui->functionsTreeView->resizeColumnToContents(0);
    ui->functionsTreeView->resizeColumnToContents(1);
    ui->functionsTreeView->resizeColumnToContents(2);
}

QTreeView *FunctionsWidget::getCurrentTreeView()
{
    if (ui->tabWidget->currentIndex() == 0)
        return ui->functionsTreeView;
    else
        return ui->nestedFunctionsTreeView;
}

void FunctionsWidget::functionsTreeView_doubleClicked(const QModelIndex &index)
{
    FunctionDescription function = index.data(FunctionModel::FunctionDescriptionRole).value<FunctionDescription>();
    this->main->seek(function.offset, function.name, true);
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

    menu->exec(treeView->mapToGlobal(pt));

    delete menu;
}

void FunctionsWidget::on_actionDisasAdd_comment_triggered()
{
    // Get selected item in functions tree view
    QTreeView *treeView = getCurrentTreeView();
    FunctionDescription function = treeView->selectionModel()->currentIndex().data(FunctionModel::FunctionDescriptionRole).value<FunctionDescription>();

    // Create dialog
    CommentsDialog *c = new CommentsDialog(this);

    if (c->exec())
    {
        // Get new function name
        QString comment = c->getComment();
        this->main->addDebugOutput("Comment: " + comment + " at: " + function.name);
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
    FunctionDescription function = treeView->selectionModel()->currentIndex().data(FunctionModel::FunctionDescriptionRole).value<FunctionDescription>();

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
        this->main->seek(function.offset);
    }
}

void FunctionsWidget::on_action_References_triggered()
{
    // Get selected item in functions tree view
    QTreeView *treeView = getCurrentTreeView();
    FunctionDescription function = treeView->selectionModel()->currentIndex().data(FunctionModel::FunctionDescriptionRole).value<FunctionDescription>();

    //this->main->add_debug_output("Addr: " + address);

    // Get function for clicked offset
    RAnalFunction *fcn = this->main->core->functionAt(function.offset);

    XrefsDialog *x = new XrefsDialog(this->main, this);
    x->setWindowTitle("X-Refs for function " + QString::fromUtf8(fcn->name));

    // Get Refs and Xrefs

    // refs = calls q hace esa funcion
    QList<XRefDescription> refs = main->core->getXRefs(fcn->addr, false, "C");

    // xrefs = calls a esa funcion
    QList<XRefDescription> xrefs = main->core->getXRefs(fcn->addr, true);

    x->fillRefs(refs, xrefs);
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

void FunctionsWidget::resizeEvent(QResizeEvent *event)
{
    if (main->responsive && isVisible())
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

void FunctionsWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->functionsTreeView);
}

void FunctionsWidget::show_filter()
{
    ui->filterLineEdit->setVisible(true);
    ui->closeFilterButton->setVisible(true);
    ui->filterLineEdit->setFocus();
}

void FunctionsWidget::clear_filter()
{
    if (ui->filterLineEdit->text() == "")
    {
        ui->filterLineEdit->setVisible(false);
        ui->closeFilterButton->setVisible(false);
        ui->functionsTreeView->setFocus();
    }
    else
    {
        ui->filterLineEdit->setText("");
    }
}

void FunctionsWidget::on_closeFilterButton_clicked()
{
    ui->filterLineEdit->setVisible(false);
    ui->closeFilterButton->setVisible(false);
    ui->functionsTreeView->setFocus();
}
