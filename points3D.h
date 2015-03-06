#ifndef POINTS3D_H
#define POINTS3D_H
#include <opencv2/highgui/highgui.hpp>
#include <vector>
using namespace std;
void initPoints3D(vector <cv::Mat>& points3D);
void generatePoints(vector <cv::Mat>& rotations, vector <cv::Mat>& translations, int quantity);
void getPointsFromFile(const char* filePath, vector <cv::Mat>& points3D, vector < vector <cv::Mat> >& points2D);
#endif // POINTS3D_H
