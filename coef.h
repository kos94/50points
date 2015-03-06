#ifndef COEF_H
#define COEF_H
#include <opencv2/highgui/highgui.hpp>
#include <vector>
void printArray(char* name, double* arr, int size, int spacePlace);
void printCameraParams(double* arr, int size);
void showCoef(char* fileName, int m);
void showRotationCoefs(double* p,  std::vector <cv::Mat> rotations, int m);
void toEulerAngles(double* p, int m);
void axisToQuaternion(double* p, int m);
double** readParams(char* filename, int m);
#endif // COEF_H
