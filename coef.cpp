#include "matrices.h"
#include <stdio.h>
#include <QDebug>
#include <iostream>
#include <math.h>

using namespace std;

void printArray(char* name, double* arr, int size, int spacePlace)
{
    cout<<"\n++++++++++++++++++ "<< name << "+++++++++++++++++\n";
    for(int i=0; i<size; i++)
    {
        if((i % spacePlace) == 0 && i != 0)
        {
            cout<<"\n";
        }
        cout<<arr[i]<<" ";
    }
}

void printCameraParams(double* arr, int size)
{
    for( int i=0; i<size; i+=6)
    {
        qDebug()<<arr[i]<<" "<<arr[i+1]<<" "<<arr[i+2]<<" "<<
                  arr[i+3]<<" "<<arr[i+4]<<" "<<arr[i+5]<<"\n";
    }
}

void showCoef(char* fileName, int m)
{
    FILE* fParams = fopen(fileName,"rt");
    FILE* fResults = fopen("results.txt","rt");

    double params[6];
    double results[6];

    int scanRes;
    printf("\n");

    for (int i=0; i<m && !feof(fParams); i++)
    {
        for (int j=0; j<6; j++)
        {
            fscanf(fParams,"%lf",&params[j]);
            fscanf(fResults,"%lf ",&results[j]);
            printf("par : res %9.6f : %9.6f\n",params[j],results[j]);/////////////////////////////DELETE
        }
        fscanf(fParams,"\n");
        scanRes = fscanf(fResults,"\n");

        if (scanRes == EOF)
            break;

        for (int j=0; j<6; j++)
        {
            if (results[j] == 0)
                printf("x       ");
            else
                printf("%9.6f ",params[j]/results[j]);
        }
        printf("\n");
        if (i%3 == 0) printf("\n");
    }

    fclose (fParams);
    fclose (fResults);
}

void showRotationCoefs(double* p,  vector <cv::Mat> rotations, int m)
{
    int cnp = 6;
    double x,y,z,w;
    double** theor = new double*[3];
    double** pract = new double*[3];
    for (int i=0; i<3; i++)
    {
        theor[i] = new double[3];
        pract[i] = new double[3];
    }

    for (int i=0; i<m*cnp; i+=cnp)
    {
        cout<<"FRAME "<<(i/6)<<endl;
        x = p[i]; y = p[i+1]; z = p[i+2];
        w = 1-x*x-y*y-z*z;

        for (int j=0; j<3; j++)
        {
            for (int k=0; k<3; k++)
            {
                theor[j][k] = rotations.at(i/6).at<float>(j,k);
            }
        }
        pract[0][0] = 1 - 2*(y*y+z*z);
        pract[0][1] = 2*(x*y-w*z);
        pract[0][2] = 2*(x*z+w*y);

        pract[1][0] = 2*(x*y+w*z);
        pract[1][1] = 1-2*(x*x-z*z);
        pract[1][2] = 2*(y*z-w*x);

        pract[2][0] = 2*(x*z-w*y);
        pract[2][1] = 2*(y*z+w*x);
        pract[2][2] = 1-2*(x*x+y*y);

        for (int j=0; j<3; j++)
        {
            for (int k=0; k<3; k++)
            {
                if (pract[j][k] == 0)
                {
                    printf("x       ");
                }
                else
                {
                    printf("%.6f ",theor[j][k]/pract[j][k]);
                }
            }
            cout<<endl;
        }
    }
}

void toEulerAngles(double* p, int m)
{
    double q0,q1,q2,q3;
    for (int i=0; i<m*6; i+=6)
    {
        q1 = p[i];
        q2 = p[i+1];
        q3 = p[i+2];
        q0 = 1.0 - q1*q1 - q2*q2 - q3*q3;

        p[i] = atan( 2.0*(q0*q1+q2*q3)/(1.0-2.0*(q1*q1+q2*q2)) );
        p[i+1] = asin(2.0*(q0*q2-q3*q1));
        p[i+2] = atan( 2.0*(q0*q3+q1*q2)/(1.0-2.0*(q2*q2+q3*q3)) );
    }
}

void axisToQuaternion(double* p, int m) {
    double a,b,g;

    for (int i=0; i<m*6; i+=6) {
        a = p[i];
        b = p[i+1];
        g = p[i+2];

        p[i+0] = sin(g/2) * cos(b/2) * cos(a/2) - cos(g/2) * sin(b/2) * sin(a/2); //q1
        p[i+1] = cos(g/2) * sin(b/2) * cos(a/2) + sin(g/2) * cos(b/2) * sin(a/2); //q2
        p[i+2] = cos(g/2) * cos(b/2) * sin(a/2) - sin(g/2) * sin(b/2) * cos(a/2); //q3
    }
}

double** readParams(char* filename, int m)
{
    double** arr = new double* [m];
    for (int i=0; i<m; i++)
        arr[i] = new double[6];

    FILE* fParams = fopen(filename,"rt");
    if (fParams == NULL)
    {
        qDebug()<<"fail";
    }
    else
    {
    int scanRes;

    for (int i=0; i<m && !feof(fParams); i++)
    {
        for (int j=0; j<6; j++)
        {
            fscanf(fParams,"%lf",&arr[i][j]);
        }
        scanRes = fscanf(fParams,"\n");

        if (scanRes == EOF)
            break;
    }
    fclose (fParams);
    }
    return arr;
}
