#include "widget.h"
#include "main.h"
#include "coef.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout * layout = new QHBoxLayout();
    view = new QWebView;
    QUrl * url = new QUrl("qrc:/index.html");
    qDebug() << url->path();
    view->load(*url);
    layout->addWidget(view);
    setLayout(layout);
    connect(view, SIGNAL(loadFinished(bool)), this, SLOT(loadFinished(bool)));
    t = 0;
}

void Widget::loadFinished(bool f) {
    QWebFrame * frame = view->page()->mainFrame();
    frame->addToJavaScriptWindowObject("qt", this);
}

QList<QVariant> Widget::run(QList<QVariant> input, int ncon, int mcon, bool isInitData) {
    QListIterator<QVariant> iter(input);
    QList<QVariant> output;
    double * sbaInput = new double[input.size()];
    double * sbaOutput;
    int counter = 0;

    while(iter.hasNext()) {
        sbaInput[counter++] = iter.next().toDouble();
    }

    qDebug()<<"=======SBA INPUT========";
    printCameraParams(sbaInput, input.size());

    sbaOutput = runSBAandGetResult(sbaInput, input.size()/6, ncon, mcon, isInitData);

    qDebug()<<"=======SBA OUTPUT========";
    printCameraParams(sbaOutput, input.size());

    for(int i=0; i<input.size(); i++) {
        output.append(*(new QVariant(sbaOutput[i])));
    }

    delete sbaInput;
    //delete sbaOutput;
    return output;
}

Widget::~Widget()
{

}
