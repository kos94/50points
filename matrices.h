#ifndef MATRICES_H
#define MATRICES_H
#include <opencv2/highgui/highgui.hpp>
#include <vector>
using namespace std;

extern cv::Mat matCalibration;

void initCalibrationMatrix();
void printMatrix (int rows, int cols, cv::Mat& mat);
cv::Mat rotateMatrixInOneAxis(float angle, int axis);
cv::Mat getRotationMatrix(float angleX, float angleY, float angleZ);
cv::Mat getTranslationMatrix(float x, float y, float z);
cv::Mat get3DPointMatrix(float x, float y, float z);
void get2DPoints(vector<cv::Mat>& rotations, vector<cv::Mat> &translations,
                 vector<cv::Mat>& points3D, vector< vector <cv::Mat> >& points2D,
                 int pointNum, int frameNum);
cv::Mat get2DPointMatrix(cv::Mat matRotation, cv::Mat matTranslation, cv::Mat mat3DPoint);

void callSBA(int n, int m, vector <vector <cv::Mat> >& points2D, vector <cv::Mat>& points3D);
#endif // MATRICES_H
