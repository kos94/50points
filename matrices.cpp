#include <iostream>
#include "matrices.h"


using namespace std;

cv::Mat matCalibration;

void initCalibrationMatrix()
{
    matCalibration.create(3,4,CV_32FC1);
    double calib[12];
    FILE* fCalib = fopen("calibration.txt","rt");
    assert(fCalib != NULL);

    qDebug()<<"CALIBRATION MATRIX READ: \n";
    for (int i=0; i<12; i++)
    {
        fscanf(fCalib,"%lf ",&calib[i]);
        qDebug()<<"х["<<i<<"]: "<<calib[i];
    }
    fclose(fCalib);
    for (int i=0; i<3; i++)
    {
        for (int j=0; j<4; j++)
        {
            matCalibration.at<float>(i,j) = calib[i*4+j];
        }
    }
}
cv::Mat getWikiRotationMatrix(float a, float b, float g)
{
    cv::Mat mat(4,4,CV_32FC1);
    mat.at<float>(0,0) = cos(a) * cos(g) - sin(a) * cos(b) * sin(g);
    mat.at<float>(0,1) = -cos(a) * sin(g) - sin(a) * cos(b) * cos(g);
    mat.at<float>(0,2) = sin(a) * sin(b);

    mat.at<float>(1,0) = sin(a) * cos(g) + cos(a) * cos(b) * sin(g);
    mat.at<float>(1,1) = -sin(a) * sin(g) + cos(a) * cos(b) * cos(g);
    mat.at<float>(1,2) = -cos(a) * sin(b);

    mat.at<float>(2,0) = sin(b) * sin(g);
    mat.at<float>(2,1) = sin(b) * cos(g);
    mat.at<float>(2,2) = cos(b);

    mat.at<float>(0,3) = 0;
    mat.at<float>(1,3) = 0;
    mat.at<float>(2,3) = 0;
    mat.at<float>(3,0) = 0;
    mat.at<float>(3,1) = 0;
    mat.at<float>(3,2) = 0;
    mat.at<float>(3,3) = 1;

    return mat;
}

cv::Mat getRotationMatrix(float angleX, float angleY, float angleZ)
{
    cv::Mat matRotation = rotateMatrixInOneAxis(angleX, 1);
    matRotation *= rotateMatrixInOneAxis(angleY, 2);
    matRotation *= rotateMatrixInOneAxis(angleZ, 3);

    return matRotation;
}

//cv::Mat getQuaternion(float angleX, float angleY, float angleZ)
//{
//    cv::Mat mat(4,4,CV_32FC1);
//    float k = sqrt(1-angleX*angleX-angleY*angleY-angleZ*angleZ);
//    for (int i=0; i<4; i++)
//        mat.at<float>(i,i) = k;
//    mat.at<float>(0,1) = -angleX; mat.at<float>(0,2) = -angleY; mat.at<float>(0,3) = -angleZ;
//    mat.at<float>(1,0) = angleX; mat.at<float>(1,2) = -angleZ; mat.at<float>(1,3) = angleY;
//    mat.at<float>(2,0) = angleY; mat.at<float>(2,1) = angleZ; mat.at<float>(2,3) = -angleX;
//    mat.at<float>(3,0) = angleZ; mat.at<float>(3,1) = -angleY; mat.at<float>(3,2) = -angleX;
//    return mat;
//}


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
    cv::Mat mat(4, 1, CV_32FC1);
    mat.at<float>(0,0) = x;
    mat.at<float>(1,0) = y;
    mat.at<float>(2,0) = z;
    mat.at<float>(3,0) = 1;
    return mat;
}

cv::Mat get2DPointMatrix(float imageX, float imageY)
{
    cv::Mat mat(3, 1, CV_32FC1);
    mat.at<float>(0,0) = imageX;
    mat.at<float>(1,0) = imageY;
    mat.at<float>(2,0) = 1;
    return mat;
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

void get2DPoints(vector <cv::Mat>& rotations, vector <cv::Mat>& translations,
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

void initCameraParameters(double* params, int framesNum, vector <cv::Mat>& rotations, vector <cv::Mat>& translations)
{
    double anX, anY, anZ, x, y, z;
//    qDebug()<<"===========INIT CAMERA PARAMETERS========";
    for (int i=0; i<framesNum; i++)
    {
        anX = params[i*6 + 0]; anY = params[i*6 + 1]; anZ = params[i*6 + 2];
        x   = params[i*6 + 3]; y   = params[i*6 + 4]; z   = params[i*6 + 5];
//        qDebug()<<anX<<" "<<anY<<" "<<anZ<<" "<<x<<" "<<y<<" "<<z<<"\n";
        rotations.push_back( getRotationMatrix(anX, anY, anZ) );
        translations.push_back( getTranslationMatrix(x, y, z) );
    }
}

void getCameraParameters(const char* filePath, double* buf)
{
    FILE* f = fopen(filePath, "rt");
    assert(f!= NULL);

    for(int i=0; ; i++)
    {
        fscanf(f, "%lf", &buf[i]);
        if( feof(f) )
        {
            break;
        }
    }
    fclose(f);
}

void getCameraParameters(const char* fileName, vector <cv::Mat>& rotations, vector <cv::Mat>& translations)
{
    float angleX, angleY, angleZ, x, y, z;
    int scanRes;
    int i=0;
    FILE* fParams = fopen(fileName,"rt");
    assert(fParams != NULL);

    while ( !feof(fParams) )
    {
        scanRes = fscanf(fParams,"%f %f %f %f %f %f\n",
                         &angleX, &angleY, &angleZ, &x, &y, &z);
        if (scanRes == 0)
            continue;
        if (scanRes == EOF)
            break;
//        qDebug()<<angleX<<" "<<angleY<<" "<<angleZ<<" "<<x<<" "<<y<<" "<<z<<"\n";
        rotations.push_back( getRotationMatrix(angleX, angleY, angleZ) );
        translations.push_back( getTranslationMatrix(x, y, z) );

        i++;
    }
    fclose (fParams);
}

void printMatrix (cv::Mat& mat)
{
    for (int i=0; i<mat.rows; i++)
    {
        for(int j=0; j<mat.cols; j++)
        {
            qDebug("%f ",mat.at<float>(i,j));
        }
    }
    qDebug("==============================================\n");
}
