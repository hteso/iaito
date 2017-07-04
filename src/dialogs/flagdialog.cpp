
#include "ui_flagdialog.h"
#include "flagdialog.h"

FlagDialog::FlagDialog(IaitoRCore *core, RVA offset, QWidget *parent) :
        QDialog(parent),
        ui(new Ui::FlagDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    this->core = core;
    this->offset = offset;
}

FlagDialog::~FlagDialog()
{
    delete ui;
}

void FlagDialog::on_buttonBox_accepted()
{
    QString name = ui->nameEdit->text();
    core->cmd(QString("f %1 @ %2").arg(name).arg(offset));
}

void FlagDialog::on_buttonBox_rejected()
{
    close();
}