#ifndef MATRICES_H
#define MATRICES_H
#include <opencv2/highgui/highgui.hpp>
#include <vector>
#include <QDebug>
using namespace std;



void initCalibrationMatrix();
void printMatrix (cv::Mat& mat);
cv::Mat rotateMatrixInOneAxis(float angle, int axis);
cv::Mat getRotationMatrix(float angleX, float angleY, float angleZ);
cv::Mat rotateMatrixFull(float rx, float ry, float rz);
cv::Mat getTranslationMatrix(float x, float y, float z);
cv::Mat get3DPointMatrix(float x, float y, float z);
cv::Mat get2DPointMatrix(cv::Mat matRotation, cv::Mat matTranslation, cv::Mat mat3DPoint);
cv::Mat get2DPointMatrix(float imageX, float imageY);
void get2DPoints(vector <cv::Mat>& rotations, vector <cv::Mat>& translations,
                 vector<cv::Mat>& points3D, vector< vector <cv::Mat> >& points2D,
                 int pointNum, int frameNum);
void initCameraParameters(double* params, int framesNum, vector <cv::Mat>& rotations,
                          vector <cv::Mat>& translations);
void getCameraParameters(const char* fileName, vector <cv::Mat>& rotations, vector <cv::Mat>& translations);
void getCameraParameters(const char* filePath, double* buf);
#endif // MATRICES_H
