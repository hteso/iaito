#ifndef SECTIONSWIDGET_H
#define SECTIONSWIDGET_H

#include <QWidget>
#include <QSplitter>
#include <QTreeWidget>

class MainWindow;

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QAbstractItemView;
class QItemSelectionModel;
QT_END_NAMESPACE


namespace Ui {
class SectionsWidget;
}

class SectionsWidget : public QSplitter
{
    Q_OBJECT

public:
    explicit SectionsWidget(MainWindow *main);
    void fillSections(int row, const QString &str, const QString &str2 = QString(),
                      const QString &str3 = QString(), const QString &str4 = QString());
    void adjustColumns();
    QTreeWidget              *tree;

private slots:

private:
    //void setupModel();
    void setupViews();

    //QAbstractItemModel     *model;
    QAbstractItemView      *pieChart;
    QItemSelectionModel    *selectionModel;
    MainWindow             *main;
    bool eventFilter(QObject *obj, QEvent *event) override;
};

#endif // SECTIONSWIDGET_H
