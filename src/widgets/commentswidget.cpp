#include "commentswidget.h"
#include "ui_commentswidget.h"

#include "mainwindow.h"

CommentsWidget::CommentsWidget(MainWindow *main, QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::CommentsWidget)
{
    ui->setupUi(this);

    // Radare core found in:
    this->main = main;
    this->commentsTreeWidget = ui->commentsTreeWidget;
    this->nestedCommentsTreeWidget = ui->nestedCmtsTreeWidget;

    QTabBar *tabs = ui->tabWidget->tabBar();
    tabs->setVisible(false);

    // Use a custom context menu on the dock title bar
    //this->title_bar = this->titleBarWidget();
    ui->actionHorizontal->setChecked(true);
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showTitleContextMenu(const QPoint &)));

    // Hide the buttons frame
    ui->frame->hide();
}

CommentsWidget::~CommentsWidget()
{
    delete ui;
}

void CommentsWidget::on_commentsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    QNOTUSED(column);

    // Get offset and name of item double clicked
    CommentDescription comment = item->data(0, Qt::UserRole).value<CommentDescription>();
    this->main->add_debug_output(RAddressString(comment.offset) + ": " + comment.name);
    this->main->seek(comment.offset, comment.name);
}

void CommentsWidget::refreshTree()
{
    this->commentsTreeWidget->clear();
    QList<CommentDescription> comments = this->main->core->getAllComments("CCu");

    for (CommentDescription comment : comments)
    {
        this->main->add_debug_output(RAddressString(comment.offset));
        QString fcn_name = this->main->core->cmdFunctionAt(comment.offset);
        QTreeWidgetItem *item = this->main->appendRow(this->commentsTreeWidget, RAddressString(comment.offset), fcn_name, comment.name);
        item->setData(0, Qt::UserRole, QVariant::fromValue(comment));
    }
    this->main->adjustColumns(this->commentsTreeWidget);

    // Add nested comments
    this->nestedCommentsTreeWidget->clear();
    QMap<QString, QList<QList<QString>>> cmts = this->main->core->getNestedComments();
    for (auto cmt : cmts.keys())
    {
        QTreeWidgetItem *item = new QTreeWidgetItem(this->nestedCommentsTreeWidget);
        item->setText(0, cmt);
        QList<QList<QString>> meow = cmts.value(cmt);
        for (int i = 0; i < meow.size(); ++i)
        {
            QList<QString> tmp = meow.at(i);
            QTreeWidgetItem *it = new QTreeWidgetItem();
            it->setText(0, tmp[1]);
            it->setText(1, tmp[0].remove('"'));
            item->addChild(it);
        }
        this->nestedCommentsTreeWidget->addTopLevelItem(item);
    }
    this->main->adjustColumns(this->nestedCommentsTreeWidget);
}

void CommentsWidget::on_toolButton_clicked()
{
    ui->tabWidget->setCurrentIndex(0);
}

void CommentsWidget::on_toolButton_2_clicked()
{
    ui->tabWidget->setCurrentIndex(1);
}

void CommentsWidget::showTitleContextMenu(const QPoint &pt)
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

void CommentsWidget::on_actionHorizontal_triggered()
{
    ui->tabWidget->setCurrentIndex(0);
}

void CommentsWidget::on_actionVertical_triggered()
{
    ui->tabWidget->setCurrentIndex(1);
}

void CommentsWidget::resizeEvent(QResizeEvent *event)
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
