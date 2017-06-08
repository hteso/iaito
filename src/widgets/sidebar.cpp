#include "sidebar.h"
#include "ui_sidebar.h"

#include "mainwindow.h"

#include <QSettings>


SideBar::SideBar(MainWindow *main) :
    QWidget(main),
    ui(new Ui::SideBar),
    // Radare core found in:
    main(main)
{
    ui->setupUi(this);

    QSettings settings;
    if (settings.value("responsive").toBool())
    {
        ui->respButton->setChecked(true);
    }
    else
    {
        ui->respButton->setChecked(false);
    }
    if (settings.value("dark").toBool())
    {
        ui->themesButton->setChecked(true);
    }
    else
    {
        ui->themesButton->setChecked(false);
    }
}

SideBar::~SideBar()
{
    delete ui;
}

void SideBar::on_tabsButton_clicked()
{
    this->main->on_actionTabs_triggered();
}

void SideBar::on_consoleButton_clicked()
{
    this->main->on_actionhide_bottomPannel_triggered();
    if (ui->consoleButton->isChecked())
    {
        ui->consoleButton->setIcon(QIcon(":/img/icons/up_white.svg"));
    }
    else
    {
        ui->consoleButton->setIcon(QIcon(":/img/icons/down_white.svg"));
    }
}

void SideBar::on_webServerButton_clicked()
{
    main->setWebServerState(ui->webServerButton->isChecked());
}

void SideBar::on_lockButton_clicked()
{
    if (ui->lockButton->isChecked())
    {
        ui->lockButton->setIcon(QIcon(":/img/icons/unlock_white.svg"));
        this->main->lockUnlock_Docks(1);
    }
    else
    {
        ui->lockButton->setIcon(QIcon(":/img/icons/lock_white.svg"));
        this->main->lockUnlock_Docks(0);
    }
}

void SideBar::themesButtonToggle()
{
    ui->themesButton->click();
}

void SideBar::on_themesButton_clicked()
{
    if (ui->themesButton->isChecked())
    {
        // Dark theme
        this->main->dark();
    }
    else
    {
        // Clear theme
        this->main->def_theme();
    }
}

void SideBar::on_calcInput_textChanged(const QString &arg1)
{
    ui->calcOutput->setText(QString::number(this->main->core->math(arg1)));
}

void SideBar::on_asm2hex_clicked()
{
    ui->hexInput->setPlainText(main->core->assemble(ui->asmInput->toPlainText()));
}

void SideBar::on_hex2asm_clicked()
{
    ui->asmInput->setPlainText(main->core->disassemble(ui->hexInput->toPlainText()));
}

void SideBar::on_respButton_toggled(bool checked)
{
    this->main->toggleResponsive(checked);
}

void SideBar::on_refreshButton_clicked()
{
    this->main->refreshVisibleDockWidgets();
}
