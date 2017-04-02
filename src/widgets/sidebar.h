#ifndef SIDEBAR_H
#define SIDEBAR_H

#include <QWidget>

class MainWindow;

namespace Ui {
class SideBar;
}

class SideBar : public QWidget
{
    Q_OBJECT

public:
    explicit SideBar(MainWindow *main);
    ~SideBar();

public slots:

    void themesButtonToggle();

private slots:

    void on_tabsButton_clicked();

    void on_consoleButton_clicked();

    void on_webServerButton_clicked();

    void on_lockButton_clicked();

    void on_themesButton_clicked();

    void on_calcInput_textChanged(const QString &arg1);

    void on_asm2hex_clicked();

    void on_hex2asm_clicked();

    void on_respButton_toggled(bool checked);

private:
    Ui::SideBar *ui;
    MainWindow  *main;
};

#endif // SIDEBAR_H
