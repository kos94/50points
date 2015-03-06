#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "matrices.h"
#include "assert.h"

void getPointsFromFile(const char* filePath, vector <cv::Mat>& points3D, vector < vector <cv::Mat> >& points2D)
{
    float x, y, z;
    float imgX, imgY;
    int scanRes;
    int framesNum;
    int pointIndex = 0;
    int i = 0;

    FILE* fPoints = fopen(filePath,"rt");
    assert(fPoints != NULL);

    while ( !feof(fPoints) )
    {
        scanRes = fscanf(fPoints,"%f %f %f\n", &x, &y, &z);
        if (scanRes == 0)
            continue;
        if (scanRes == EOF)
            break;
        pointIndex++;

        qDebug()<<"i = "<<i<<"; point: "<<x<<" "<<y<<" "<<z; /////////////////////////////////////SHOW POINTS
        points3D.push_back( get3DPointMatrix(x, y, z) );

        fscanf(fPoints, "%d", &framesNum);

        vector < cv::Mat > pointProj;
        for(int j=0; j<framesNum; j++)
        {
            fscanf(fPoints, "%*f %f %f", &imgX, &imgY);
            pointProj.push_back( get2DPointMatrix(imgX, imgY) );
            qDebug("j: %d, x: %f,y: %f", j, imgX, imgY);
        }
        points2D.push_back( pointProj );
        i++;
    }
    fclose (fPoints);
}

void initPoints3D(vector <cv::Mat>& points3D)
{
    float x, y, z;
    int scanRes;
    FILE* fPoints = fopen("points.txt","rt");

    while ( !feof(fPoints) )
    {
        scanRes = fscanf(fPoints,"%f %f %f\n", &x, &y, &z);
        if (scanRes == 0)
            continue;
        if (scanRes == EOF)
            break;
//        qDebug()<<"point: "<<x<<" "<<y<<" "<<z; /////////////////////////////////////SHOW POINTS
        points3D.push_back( get3DPointMatrix(x, y, z) );
    }
    fclose (fPoints);
}

void generatePoints(vector <cv::Mat>& rotations, vector <cv::Mat>& translations, int quantity)
{
    float rx, ry, rz, imageX, imageY;
    int counter,framesNum;
    cv::Mat rPoint3D, rPoint2D;
    FILE* fPoints = fopen("points.txt","wt");

    framesNum = rotations.size();
    srand (time(NULL));

    for (int i=0; i<quantity; i++)
    {
        counter = 0;
        do
        {
            if (i%5==0 && i!=0 && counter==0)
                fprintf(fPoints,"\n");
            rx = (rand() % 6000 - 3000)*0.001;
            ry = (rand() % 6000 - 3000)*0.001;
            rz = (rand() % 6000)*0.001;

            rPoint3D = get3DPointMatrix(rx,ry,rz);

            int j;
            for (j=0; j<framesNum; j++)
            {
                rPoint2D = get2DPointMatrix(rotations.at(j),translations.at(j),rPoint3D);
                imageX = rPoint2D.at<float>(0,0); imageY = rPoint2D.at<float>(1,0);
                if ( !(0 <= imageX && imageX <= 640 && 0<=imageY && imageY<=480) ) {
                    //qDebug()<<"fail: i="<<i<<", frame #"<<j<<"\n";
                    break;
                }
                //qDebug()<<"pic: "<<imageX<<", "<<imageY<<";\n";
            }
            if (j == framesNum)
                break;
            counter++;
        }
        while (counter < 2000);
        cout<<counter<<"; ";
        fprintf(fPoints, "%f %f %f\n",rx, ry, rz);
    }
    cout<<"\n";
    fclose (fPoints);
}
