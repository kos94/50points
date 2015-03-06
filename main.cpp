#include <QApplication>
#include <QDebug>
#include <sba.h>
//#include "opencv/highgui.h"
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include "points3D.h"
#include "matrices.h"
#include "coef.h"
#include "sbaFunctions.h"
#include "widget.h"
using namespace std;

//double* runSBAandGetResult(double* sbaInput, int framesNum, bool isInitData);
void workWithConsole();

int main(int argc, char** argv)
{
//    workWithConsole();
//    return 0;

    //запустить editor
    QApplication app(argc,argv);
    QWebSettings::globalSettings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
    Widget w;
    w.show();
    return app.exec();
}

void workWithConsole()
{
    vector <cv::Mat> rotations;
    vector <cv::Mat> translations;
    vector <cv::Mat> points3D;
    vector < vector <cv::Mat> > points2D;
    int n, m;
    double* sbaInput;

    const int mcon = 0;
    const int ncon = 5;
    const int cnp = 6;
    const char* pointsPath = "initdata/5pts.txt";
    const char* camerasPath = "initdata/4cams.txt";

    initCalibrationMatrix();

    getCameraParameters(camerasPath, rotations, translations);
    n = rotations.size();
    qDebug("n = %d", n);

    getPointsFromFile(pointsPath, points3D,points2D);
    m = points2D.at(0).size();
    qDebug("m = %d", m);

    sbaInput = new double[m * cnp];
    getCameraParameters(camerasPath, sbaInput);
    axisToQuaternion(sbaInput, m);

    double* sbaRes = callSBA(points2D, points3D, NULL, ncon, mcon);
    toEulerAngles(sbaRes,m); //коэф. кватерниона в углы эйлера
    printArray("result", sbaRes, m*cnp, cnp);
}

double* runSBAandGetResult(double* sbaInput, int framesNum, int ncon, int mcon, bool isInitData)
{
    vector <cv::Mat> rotations;
    vector <cv::Mat> translations;
    vector <cv::Mat> points3D;
    vector < vector <cv::Mat> > points2D;
    int m = framesNum;
    int n = 5;
    double* sbaRes;

    initCalibrationMatrix();

    initCameraParameters(sbaInput, framesNum, rotations, translations);

    generatePoints(rotations, translations, n);
    initPoints3D(points3D);
    get2DPoints(rotations, translations, points3D, points2D, n, m);

    axisToQuaternion(sbaInput,m); //углы эйл

    sbaRes = callSBA(points2D, points3D, (isInitData)? sbaInput : NULL, ncon, mcon);

//    showRotationCoefs(sbaRes,rotations,m);

    toEulerAngles(sbaRes,m); //коэф. кватерниона в углы эйлера

    return sbaRes;
}

