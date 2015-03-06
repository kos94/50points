#ifndef MAIN_H
#define MAIN_H

void pointsToArr(vector <cv::Mat>& points3D, double* dest, int offset);
double* callSBA(vector <vector <cv::Mat> >& points2D, vector <cv::Mat>& points3D, double *initData, int ncon, int mcon);

#endif // MAIN_H
