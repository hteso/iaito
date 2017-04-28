#ifndef OMNIBAR_H
#define OMNIBAR_H

#include <QLineEdit>

class MainWindow;

class Omnibar : public QLineEdit
{
    Q_OBJECT
public:
    explicit Omnibar(MainWindow *main, QWidget *parent = 0);

    void refresh(const QStringList &flagList);

private slots:
    void on_gotoEntry_returnPressed();

    void restoreCompleter();

public slots:
    void showCommands();
    void clear();

private:
    void setupCompleter();

    MainWindow          *main;
    const QStringList   commands;
    QStringList         flags;
};

#endif // OMNIBAR_H
