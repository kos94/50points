#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QHBoxLayout>
#include <QWebView>
#include <QWebFrame>
#include <QUrl>
#include <QVariant>
#include <QList>
#include <QDebug>

class Widget : public QWidget
{
    Q_OBJECT

private:
    QWebView * view;
public:
    int t;

    Widget(QWidget *parent = 0);
    ~Widget();
public slots:
    void loadFinished(bool);
    QList<QVariant> run(QList<QVariant>,int,int,bool);
};

#endif // WIDGET_H
