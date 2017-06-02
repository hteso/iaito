#ifndef ANALTHREAD_H
#define ANALTHREAD_H

#include <QThread>

class QRCore;

class AnalThread : public QThread
{
    Q_OBJECT
public:
    explicit AnalThread(QWidget *parent = 0);
    ~AnalThread();

    void start(QRCore *core, int level, QList<QString> advanced);

protected:
    void run();

    using QThread::start;

private:
    QRCore *core;
    int level;
    QList<QString> advanced;
};

#endif // ANALTHREAD_H
