#include <QApplication>
#include "qmlapplicationviewer.h"
#include <QDebug>
#include "opencv/highgui.h"
#include <vector>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>

#include "matrices.h"

using namespace std;

int main()
{
    qDebug()<<"begin";
    vector <cv::Mat> rotations;
    vector <cv::Mat> translations;
    vector <cv::Mat> points3D;
    vector < vector <cv::Mat> > points2D;

    initCalibrationMatrix();
    points3D.push_back(get3DPointMatrix(1,1,4));
    points3D.push_back (get3DPointMatrix(0.6, 0.9, 4));
    points3D.push_back (get3DPointMatrix(0.5, 1, 4));
    points3D.push_back (get3DPointMatrix(0.4, 1.2, 3.5));
    points3D.push_back (get3DPointMatrix(0.4, 0.95, 5));

    //1
        rotations.push_back( getRotationMatrix(0.0, 0.0, 0.0) );
        translations.push_back( getTranslationMatrix(0.0, 0.0, 0.0) );
//    rotations.push_back( getRotationMatrix(0.5, 0.15, 0.05) );
//    translations.push_back( getTranslationMatrix(0.8, 0.1, 0.3) );
    //2
    rotations.push_back( getRotationMatrix(0.21, 0, 0.1) );
    translations.push_back( getTranslationMatrix(0.5, -0.2, 0.4) );
    //3
    rotations.push_back( getRotationMatrix(0.3, -0.1, 0.2) );
    translations.push_back( getTranslationMatrix(-0.31, 0.1, 0.2) );
    //4
    rotations.push_back( getRotationMatrix(0.17, 0.36, -0.05) );
    translations.push_back( getTranslationMatrix(-0.5, 0.3, 0.1) );
    //5
    rotations.push_back( getRotationMatrix(0.37, -0.12, 0.4) );
    translations.push_back( getTranslationMatrix(-0.2, -0.4, 0.36) );
    //6
    rotations.push_back( getRotationMatrix(-0.14, 0.02, 0.25) );
    translations.push_back( getTranslationMatrix(0.22, -0.05, 1.05) );
    //7
    rotations.push_back( getRotationMatrix(0.01, -0.01, 0.06) );
    translations.push_back( getTranslationMatrix(-0.4, 0.1, 1.77) );
    //8
    rotations.push_back( getRotationMatrix(0.21, 0.2, 0.07) );
    translations.push_back( getTranslationMatrix(0.03, -0.23, 0.2) );
    //9
    rotations.push_back( getRotationMatrix(0.29, 0.11, 1.1) );
    translations.push_back( getTranslationMatrix(-0.31, 0.11, 1.2) );
    //10
    rotations.push_back( getRotationMatrix(0.153, -0.11, -0.19) );
    translations.push_back( getTranslationMatrix(0.26, 0.1, 0.68) );

    //get 2d points
    qDebug()<<"POINT 1";

//    get2DPoint(rotations, translations, points3D, points2D, 0, 10);
//    get2DPoint(rotations, translations, points3D, points2D, 1, 10);
//    get2DPoint(rotations, translations, points3D, points2D, 2, 10);
//    get2DPoint(rotations, translations, points3D, points2D, 3, 10);
      get2DPoints(rotations, translations, points3D, points2D, 5, 10);
//    for (int i=0; i<10; i++)
//    {
//        cout<<"["<<i+1<<"]\n";
//        printMatrix(2,1,points2D.at(0).at(i));
//    }
    for (int i=0; i<4; i++)
    {
        qDebug()<<"point "<<(i+1)<<":\n";
        for (int j=0; j<10; j++)
        {
            qDebug()<<"( "<<points2D.at(i).at(j).at<float>(0,0)<<" ; "<<points2D.at(i).at(j).at<float>(1,0)<<" )\n";
        }
    }
    callSBA(5,10,points2D,points3D);
}

