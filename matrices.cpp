#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <vector>
#include "matrices.h"
#include <QDebug>

using namespace std;

cv::Mat matCalibration;

void initCalibrationMatrix()
{
//    K[0]=851.57945; K[1]=0.0; K[2]=330.24755;
//    K[3]=0.0; K[4]=853.01905; K[5]=262.19500;
    matCalibration.create(3,4,CV_32FC1);

    matCalibration.at<float>(0,0) = 500;
    matCalibration.at<float>(0,1) = 0;
    matCalibration.at<float>(0,2) = 300;
    matCalibration.at<float>(0,3) = 0;

    matCalibration.at<float>(1,0) = 0;
    matCalibration.at<float>(1,1) = 500;
    matCalibration.at<float>(1,2) = 250;
    matCalibration.at<float>(1,3) = 0;

    matCalibration.at<float>(2,0) = 0;
    matCalibration.at<float>(2,1) = 0;
    matCalibration.at<float>(2,2) = 1;
    matCalibration.at<float>(2,3) = 0;
}

cv::Mat getRotationMatrix(float angleX, float angleY, float angleZ)
{
    cv::Mat matRotation = rotateMatrixInOneAxis(angleX, 1);
    matRotation *= rotateMatrixInOneAxis(angleY, 2);
    matRotation *= rotateMatrixInOneAxis(angleZ, 3);
    return matRotation;
}

cv::Mat rotateMatrixInOneAxis(float angle, int axis)
{
    cv::Mat mat(4,4,CV_32FC1);
    switch (axis) {
    case 1:
        mat.at<float>(0,0) = 1;
        mat.at<float>(1,0) = 0;
        mat.at<float>(2,0) = 0;
        mat.at<float>(0,1) = 0;
        mat.at<float>(1,1) = cos(angle);
        mat.at<float>(2,1) = sin(angle);
        mat.at<float>(0,2) = 0;
        mat.at<float>(1,2) = -sin(angle);
        mat.at<float>(2,2) = cos(angle);
        break;
    case 2:
        mat.at<float>(0,0) = cos(angle);
        mat.at<float>(1,0) = 0;
        mat.at<float>(2,0) = -sin(angle);
        mat.at<float>(0,1) = 0;
        mat.at<float>(1,1) = 1;
        mat.at<float>(2,1) = 0;
        mat.at<float>(0,2) = sin(angle);
        mat.at<float>(1,2) = 0;
        mat.at<float>(2,2) = cos(angle);
        break;
    default:
        mat.at<float>(0,0) = cos(angle);
        mat.at<float>(1,0) = sin(angle);
        mat.at<float>(2,0) = 0;
        mat.at<float>(0,1) = -sin(angle);
        mat.at<float>(1,1) = cos(angle);
        mat.at<float>(2,1) = 0;
        mat.at<float>(0,2) = 0;
        mat.at<float>(1,2) = 0;
        mat.at<float>(2,2) = 1;
        break;
    }

    mat.at<float>(0,3) = 0;
    mat.at<float>(1,3) = 0;
    mat.at<float>(2,3) = 0;
    mat.at<float>(3,0) = 0;
    mat.at<float>(3,1) = 0;
    mat.at<float>(3,2) = 0;
    mat.at<float>(3,3) = 1;
    return mat;
}

cv::Mat getTranslationMatrix(float x, float y, float z)
{
    cv::Mat matTranslation(4, 4, CV_32FC1);
    for (int i=0; i<4; i++)
    {
        for (int j=0; j<4; j++)
        {
            if (i == j)
                matTranslation.at<float>(i,j) = 1;
            else
                matTranslation.at<float>(i,j) = 0;
        }
    }
    matTranslation.at<float>(0,3) = x;
    matTranslation.at<float>(1,3) = y;
    matTranslation.at<float>(2,3) = z;
    return matTranslation;
}

cv::Mat get3DPointMatrix(float x, float y, float z)
{
    cv::Mat matPoint(4, 1, CV_32FC1);
    matPoint.at<float>(0,0) = x;
    matPoint.at<float>(1,0) = y;
    matPoint.at<float>(2,0) = z;
    matPoint.at<float>(3,0) = 1;
    return matPoint;
}

cv::Mat get2DPointMatrix(cv::Mat matRotation,cv::Mat matTranslation, cv::Mat mat3DPoint)
{
    cv::Mat matTransform = matRotation * (-1.0 * matTranslation);

    cv::Mat mat1 = matCalibration * matTransform;

    cv::Mat mat2DPoint = mat1 * mat3DPoint;
    float t = mat2DPoint.at<float>(2,0);
    if (t != 0)
    {
        mat2DPoint.at<float>(0,0) /= t;
        mat2DPoint.at<float>(1,0) /= t;
        mat2DPoint.at<float>(2,0) /= t;
    }

    return mat2DPoint;
}

void get2DPoints(vector<cv::Mat>& rotations, vector<cv::Mat> &translations,
                 vector<cv::Mat>& points3D, vector< vector <cv::Mat> >& points2D,
                 int pointNum, int frameNum)
{
    for (int i=0; i<pointNum; i++)
    {
        vector<cv::Mat> temp;

        for (int j=0; j<frameNum; j++)
        {
            temp.push_back( get2DPointMatrix(rotations.at(j),
                                             translations.at(j),
                                             points3D.at(i)) );

        }

        points2D.push_back(temp);
    }
}

void printMatrix (int rows, int cols, cv::Mat& mat)
{
    for (int i=0; i<rows; i++)
    {
        for(int j=0; j<cols; j++)
        {
            cout<<mat.at<float>(i,j)<<' ';
        }
        cout<<'\n';
    }
    cout<<"==============================================\n";
}

