#ifndef FUNCTIONSWIDGET_H
#define FUNCTIONSWIDGET_H

#include <QDockWidget>
#include <QTreeWidget>
#include <QSortFilterProxyModel>
#include "qrcore.h"

class MainWindow;

namespace Ui
{
    class FunctionsWidget;
}


class FunctionModel : public QAbstractItemModel
{
    Q_OBJECT

private:
    MainWindow *main;

    QList<RFunction> functions;

    QFont highlight_font;
    QFont default_font;
    bool nested;

    int current_index;

public:
    FunctionModel(bool nested, QFont default_font, QFont highlight_font, MainWindow *main, QObject *parent = 0);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    //void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

    void setFunctions(QList<RFunction> functions);

    void updateCurrentIndex();

    bool isNested()     { return nested; }

private slots:
    void cursorAddressChanged(RVA addr);
    void functionRenamed(QString prev_name, QString new_name);

};


class FunctionSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    FunctionSortFilterProxyModel(FunctionModel *source_model, QObject *parent = 0);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};



class FunctionsWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit FunctionsWidget(MainWindow *main, QWidget *parent = 0);
    ~FunctionsWidget();

    void fillFunctions();

private slots:
    void on_functionsTreeView_itemDoubleClicked(const QModelIndex &index);
    void showFunctionsContextMenu(const QPoint &pt);

    void on_actionDisasAdd_comment_triggered();

    void on_actionFunctionsRename_triggered();

    void on_action_References_triggered();

    void showTitleContextMenu(const QPoint &pt);

    void on_actionHorizontal_triggered();

    void on_actionVertical_triggered();

    void on_nestedFunctionsTree_itemDoubleClicked(QTreeWidgetItem *item, int column);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QTreeView *getCurrentTreeView();

    Ui::FunctionsWidget *ui;

    MainWindow      *main;

    QList<RFunction> functions;

    FunctionModel *function_model;
    FunctionSortFilterProxyModel *function_proxy_model;

    FunctionModel *nested_function_model;
    FunctionSortFilterProxyModel *nested_function_proxy_model;
};





#endif // FUNCTIONSWIDGET_H
