#include "sectionsdock.h"
#include "ui_sectionsdock.h"

#include "mainwindow.h"
#include "widgets/sectionswidget.h"

#include <QMenu>
#include <QResizeEvent>


SectionsDock::SectionsDock(MainWindow *main, QWidget *parent) :
    DockWidget(parent),
    ui(new Ui::SectionsDock)
{
    ui->setupUi(this);

    // Radare core found in:
    this->main = main;

    this->sectionsWidget = new SectionsWidget(this->main);
    this->setWidget(this->sectionsWidget);
    this->sectionsWidget->setContentsMargins(0, 0, 0, 5);
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showSectionsContextMenu(const QPoint &)));
}

SectionsDock::~SectionsDock()
{
    delete ui;
}

void SectionsDock::setup()
{
    sectionsWidget->setup();
}

void SectionsDock::refresh()
{
    sectionsWidget->setup();
}

void SectionsDock::showSectionsContextMenu(const QPoint &pt)
{
    // Set functions popup menu
    QMenu *menu = new QMenu(this);
    menu->clear();
    menu->addAction(ui->actionHorizontal);
    menu->addAction(ui->actionVertical);

    if (this->sectionsWidget->orientation() == 1)
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

void SectionsDock::resizeEvent(QResizeEvent *event)
{
    if(main->responsive && isVisible())
    {
        if (event->size().width() >= event->size().height())
        {
            // Set horizontal view (list)
            this->on_actionHorizontal_triggered();
        }
        else
        {
            // Set vertical view (Tree)
            this->on_actionVertical_triggered();
        }
    }
    QWidget::resizeEvent(event);
}

void SectionsDock::on_actionVertical_triggered()
{
    this->sectionsWidget->setOrientation(Qt::Vertical);
}

void SectionsDock::on_actionHorizontal_triggered()
{
    this->sectionsWidget->setOrientation(Qt::Horizontal);
}
