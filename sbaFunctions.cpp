#include <iostream>

#include <sba.h>
#include <sba-1.6/sba.h>
#include <sba-1.6/compiler.h>
#include <sba-1.6/sba_chkjac.h>
#include <sba-1.6/demo/eucsbademo.h>
#include <sba-1.6/demo/readparams.h>
#include <QDebug>
//#include "sba-1.6/sba.h"
//#include "sba-1.6/compiler.h"
//#include "sba-1.6/sba_chkjac.h"
//#include "sba-1.6/demo/eucsbademo.h"
//#include "sba-1.6/demo/readparams.h"

#include <vector>
#include <opencv/highgui.h>
#include <opencv2/highgui/highgui.hpp>
#include "matrices.h"

static void img_projsRTS_x(double *p, struct sba_crsm *idxij, int *rcidxs, int *rcsubs, double *hx, void *adata);
static void img_projsRTS_jac_x(double *p, struct sba_crsm *idxij, int *rcidxs, int *rcsubs, double *jac, void *adata);

using namespace std;

struct globs_{
    double *rot0params;
    double *intrcalib;
    int nccalib;
    int ncdist;
    int cnp, pnp, mnp;
    double *ptparams;
    double *camparams;
} globs;

/* unit quaternion from vector part */
#define _MK_QUAT_FRM_VEC(q, v){                                     \
  (q)[1]=(v)[0]; (q)[2]=(v)[1]; (q)[3]=(v)[2];                      \
  (q)[0]=sqrt(1.0 - (q)[1]*(q)[1] - (q)[2]*(q)[2]- (q)[3]*(q)[3]);  \
}

void callSBA(int n, int m, vector <vector <cv::Mat> >& points2D, vector <cv::Mat>& points3D)
{
    const int mcon = 0; //n of images with const parameters
    char vmask[n*m];
    const int cnp = 6; //n of camera parameters
    const int pnp = 3; //n of point parameters (3d)
    const int mnp = 2; //n of image point parameters
    int itmax = 20;                                        //to parameters?
    double initrot[m*4];
    double p[m*cnp + n*pnp];
    double x[n*m*mnp];
    for (int i=0; i<m*4; i++)
    {
        if ( i % 4 == 0)
            initrot[i] = 1;
        else
            initrot[i] = 0;
    }

    for (int i=0; i<m*n; i++)
        vmask[i] = 1;
    //cameras params & 3d points coords
    for (int i=0; i<m*cnp; i++)
        p[i] = 0;
    int i1 = m*cnp;
    for (int i=0; i<n; i++)
    {
        p[i1+i*3] = points3D.at(i).at<float>(0,0);
        p[i1+i*3+1] = points3D.at(i).at<float>(1,0);
        p[i1+i*3+2] = points3D.at(i).at<float>(2,0);
    }

    qDebug()<<"POINT3\n";
    //image projections
    for (int i=0; i<n; i++)
    {
        for (int j=0; j<m; j++)
        {
            x[i*m*2+2*j] = points2D.at(i).at(j).at<float>(0,0);
            x[i*m*2+2*j+1] = points2D.at(i).at(j).at<float>(1,0);
        }
    }

    qDebug()<<"POINT4\n";
    //fill globs struct
    double ical[5],K[9];
    K[0]=500.0; K[1]=0.0; K[2]=300.0;
    K[3]=0.0; K[4]=500.0; K[5]=250.0;
//    K[0]=851.57945; K[1]=0.0; K[2]=330.24755;
//    K[3]=0.0; K[4]=853.01905; K[5]=262.19500;
    ical[0]=K[0]; // fu
    ical[1]=K[2]; // u0
    ical[2]=K[5]; // v0
    ical[3]=K[4]/K[0]; // ar
    ical[4]=K[1]; // s
    globs.intrcalib=ical;
    globs.ptparams=NULL;
    globs.camparams=NULL;
    globs.cnp = cnp; globs.mnp = mnp; globs.pnp = pnp;
    globs.rot0params = initrot; // ??????????????????????????????????????
    //globs.nccalib globs.ncdist - ????????????????????????????????????
    //fill opts & info
    double opts[SBA_OPTSSZ], info[SBA_INFOSZ];
    opts[0]=SBA_INIT_MU;
    opts[1]=SBA_STOP_THRESH;
    opts[2]=SBA_STOP_THRESH;
    opts[3]=SBA_STOP_THRESH;
    //opts[3]=0.05*numprojs; // uncomment to force termination if the average reprojection error drops below 0.05
    opts[4]=0.0;
    //opts[4]=1E-05; // uncomment to force termination if the relative reduction in the RMS reprojection error drops below 1E-05
    sba_motstr_levmar_x(n,0,m,mcon,vmask,p,cnp,pnp,x,
                      NULL,mnp,img_projsRTS_x,img_projsRTS_jac_x,
                      (void*)(&globs), itmax, 0, opts, info);
    qDebug()<<"m: "<<m<<"cnp: "<<cnp<<"n: "<<n<<"pnp: "<<pnp;
    for (int i=0; i<m*cnp; i++)
    {
        if (i % 6 == 0)
            qDebug()<<"\n==================\nFrame "<<(i/6+1);
        if (i % 6 < 3)
            qDebug()<<"--"<<p[i];
        else
            qDebug()<<p[i];
    }
}
/*
 * fast multiplication of the two quaternions in q1 and q2 into p
 * this is the second of the two schemes derived in pg. 8 of
 * T. D. Howell, J.-C. Lafon, The complexity of the quaternion product, TR 75-245, Cornell Univ., June 1975.
 *
 * total additions increase from 12 to 27 (28), but total multiplications decrease from 16 to 9 (12)
 */
inline static void quatMultFast(double q1[FULLQUATSZ], double q2[FULLQUATSZ], double p[FULLQUATSZ])
{
double t1, t2, t3, t4, t5, t6, t7, t8, t9;
//double t10, t11, t12;

  t1=(q1[0]+q1[1])*(q2[0]+q2[1]);
  t2=(q1[3]-q1[2])*(q2[2]-q2[3]);
  t3=(q1[1]-q1[0])*(q2[2]+q2[3]);
  t4=(q1[2]+q1[3])*(q2[1]-q2[0]);
  t5=(q1[1]+q1[3])*(q2[1]+q2[2]);
  t6=(q1[1]-q1[3])*(q2[1]-q2[2]);
  t7=(q1[0]+q1[2])*(q2[0]-q2[3]);
  t8=(q1[0]-q1[2])*(q2[0]+q2[3]);

#if 0
  t9 =t5+t6;
  t10=t7+t8;
  t11=t5-t6;
  t12=t7-t8;

  p[0]= t2 + 0.5*(-t9+t10);
  p[1]= t1 - 0.5*(t9+t10);
  p[2]=-t3 + 0.5*(t11+t12);
  p[3]=-t4 + 0.5*(t11-t12);
#endif

  /* following fragment it equivalent to the one above */
  t9=0.5*(t5-t6+t7+t8);
  p[0]= t2 + t9-t5;
  p[1]= t1 - t9-t6;
  p[2]=-t3 + t9-t8;
  p[3]=-t4 + t9-t7;
}

/* Given a parameter vector p made up of the 3D coordinates of n points and the parameters of m cameras, compute in
 * hx the prediction of the measurements, i.e. the projections of 3D points in the m images. The measurements
 * are returned in the order (hx_11^T, .. hx_1m^T, ..., hx_n1^T, .. hx_nm^T)^T, where hx_ij is the predicted
 * projection of the i-th point on the j-th camera.
 * Notice that depending on idxij, some of the hx_ij might be missing
 *
 */
#include <math.h>
static void img_projsRTS_x(double *p, struct sba_crsm *idxij, int *rcidxs, int *rcsubs, double *hx, void *adata)
{
  register int i, j;
  int cnp, pnp, mnp;
  double *pa, *pb, *pqr, *pt, *ppt, *pmeas, *Kparms, *pr0, lrot[FULLQUATSZ], trot[FULLQUATSZ];
  //int n;
  int m, nnz;
  struct globs_ *gl;

  gl=(struct globs_ *)adata;
  cnp=gl->cnp; pnp=gl->pnp; mnp=gl->mnp;
  Kparms=gl->intrcalib;

  //n=idxij->nr;
  m=idxij->nc;
  pa=p; pb=p+m*cnp;

  for(j=0; j<m; ++j){
    /* j-th camera parameters */
    pqr=pa+j*cnp;
    pt=pqr+3; // quaternion vector part has 3 elements
    pr0=gl->rot0params+j*FULLQUATSZ; // full quat for initial rotation estimate
    _MK_QUAT_FRM_VEC(lrot, pqr);
    quatMultFast(lrot, pr0, trot); // trot=lrot*pr0
    nnz=sba_crsm_col_elmidxs(idxij, j, rcidxs, rcsubs); /* find nonzero hx_ij, i=0...n-1 */

    for(i=0; i<nnz; ++i){
      ppt=pb + rcsubs[i]*pnp;
      pmeas=hx + idxij->val[rcidxs[i]]*mnp; // set pmeas to point to hx_ij

      calcImgProjFullR(Kparms, trot, pt, ppt, pmeas); // evaluate Q in pmeas
      //calcImgProj(Kparms, pr0, pqr, pt, ppt, pmeas); // evaluate Q in pmeas
    }
  }
}


/* Given a parameter vector p made up of the 3D coordinates of n points and the parameters of m cameras, compute in
 * jac the jacobian of the predicted measurements, i.e. the jacobian of the projections of 3D points in the m images.
 * The jacobian is returned in the order (A_11, ..., A_1m, ..., A_n1, ..., A_nm, B_11, ..., B_1m, ..., B_n1, ..., B_nm),
 * where A_ij=dx_ij/db_j and B_ij=dx_ij/db_i (see HZ).
 * Notice that depending on idxij, some of the A_ij, B_ij might be missing
 *
 */
static void img_projsRTS_jac_x(double *p, struct sba_crsm *idxij, int *rcidxs, int *rcsubs, double *jac, void *adata)
{
  register int i, j;
  int cnp, pnp, mnp;
  double *pa, *pb, *pqr, *pt, *ppt, *pA, *pB, *Kparms, *pr0;
  //int n;
  int m, nnz, Asz, Bsz, ABsz;
  struct globs_ *gl;

  gl=(struct globs_ *)adata;
  cnp=gl->cnp; pnp=gl->pnp; mnp=gl->mnp;
  Kparms=gl->intrcalib;

  //n=idxij->nr;
  m=idxij->nc;
  pa=p; pb=p+m*cnp;
  Asz=mnp*cnp; Bsz=mnp*pnp; ABsz=Asz+Bsz;

  for(j=0; j<m; ++j){
    /* j-th camera parameters */
    pqr=pa+j*cnp;
    pt=pqr+3; // quaternion vector part has 3 elements
    pr0=gl->rot0params+j*FULLQUATSZ; // full quat for initial rotation estimate

    nnz=sba_crsm_col_elmidxs(idxij, j, rcidxs, rcsubs); /* find nonzero hx_ij, i=0...n-1 */

    for(i=0; i<nnz; ++i){
      ppt=pb + rcsubs[i]*pnp;
      pA=jac + idxij->val[rcidxs[i]]*ABsz; // set pA to point to A_ij
      pB=pA  + Asz; // set pB to point to B_ij

      calcImgProjJacRTS(Kparms, pr0, pqr, pt, ppt, (double (*)[6])pA, (double (*)[3])pB); // evaluate dQ/da, dQ/db in pA, pB
    }
  }
}
/* Code automatically generated by maple's codegen package */

/* Computation of the predicted projection of a 3D point and its jacobians */


#include <math.h>
void calcImgProj(double a[5],double qr0[4],double v[3],double t[3],double M[3],
double n[2])
{
  double t1;
  double t10;
  double t12;
  double t14;
  double t16;
  double t18;
  double t19;
  double t2;
  double t25;
  double t26;
  double t3;
  double t32;
  double t33;
  double t35;
  double t36;
  double t4;
  double t42;
  double t46;
  double t5;
  double t51;
  double t52;
  double t57;
  double t58;
  double t6;
  double t69;
  double t7;
  double t77;
  double t80;
  double t9;
  {
    t1 = a[0];
    t2 = v[0];
    t3 = t2*t2;
    t4 = v[1];
    t5 = t4*t4;
    t6 = v[2];
    t7 = t6*t6;
    t9 = sqrt(1.0-t3-t5-t7);
    t10 = qr0[1];
    t12 = qr0[0];
    t14 = qr0[3];
    t16 = qr0[2];
    t18 = t9*t10+t12*t2+t4*t14-t6*t16;
    t19 = M[0];
    t25 = t9*t16+t12*t4+t6*t10-t2*t14;
    t26 = M[1];
    t32 = t9*t14+t12*t6+t2*t16-t4*t10;
    t33 = M[2];
    t35 = -t18*t19-t25*t26-t32*t33;
    t36 = -t18;
    t42 = t9*t12-t2*t10-t4*t16-t6*t14;
    t46 = t42*t19+t25*t33-t32*t26;
    t51 = t42*t26+t32*t19-t18*t33;
    t52 = -t32;
    t57 = t42*t33+t18*t26-t25*t19;
    t58 = -t25;
    t69 = t35*t58+t42*t51+t57*t36-t46*t52+t[1];
    t77 = t35*t52+t42*t57+t46*t58-t51*t36+t[2];
    t80 = 1/t77;
    n[0] = (t1*(t35*t36+t42*t46+t51*t52-t57*t58+t[0])+a[4]*t69+a[1]*t77)*t80;
    n[1] = (t1*a[3]*t69+a[2]*t77)*t80;
    return;
  }
}

void calcImgProjFullR(double a[5],double qr0[4],double t[3],double M[3],
double n[2])
{
  double t1;
  double t11;
  double t13;
  double t17;
  double t2;
  double t22;
  double t27;
  double t3;
  double t38;
  double t46;
  double t49;
  double t5;
  double t6;
  double t8;
  double t9;
  {
    t1 = a[0];
    t2 = qr0[1];
    t3 = M[0];
    t5 = qr0[2];
    t6 = M[1];
    t8 = qr0[3];
    t9 = M[2];
    t11 = -t3*t2-t5*t6-t8*t9;
    t13 = qr0[0];
    t17 = t13*t3+t5*t9-t8*t6;
    t22 = t6*t13+t8*t3-t9*t2;
    t27 = t13*t9+t6*t2-t5*t3;
    t38 = -t5*t11+t13*t22-t27*t2+t8*t17+t[1];
    t46 = -t11*t8+t13*t27-t5*t17+t2*t22+t[2];
    t49 = 1/t46;
    n[0] = (t1*(-t2*t11+t13*t17-t22*t8+t5*t27+t[0])+a[4]*t38+a[1]*t46)*t49;
    n[1] = (t1*a[3]*t38+a[2]*t46)*t49;
    return;
  }
}

#include <math.h>
void calcImgProjJacKRTS(double a[5],double qr0[4],double v[3],double t[3],
double M[3],double jacmKRT[2][11],double jacmS[2][3])
{
  double t1;
  double t102;
  double t107;
  double t109;
  double t11;
  double t114;
  double t116;
  double t120;
  double t129;
  double t13;
  double t131;
  double t140;
  double t148;
  double t149;
  double t15;
  double t150;
  double t152;
  double t154;
  double t161;
  double t164;
  double t167;
  double t17;
  double t170;
  double t172;
  double t174;
  double t177;
  double t18;
  double t182;
  double t187;
  double t189;
  double t194;
  double t196;
  double t2;
  double t208;
  double t218;
  double t229;
  double t232;
  double t235;
  double t237;
  double t239;
  double t24;
  double t242;
  double t247;
  double t25;
  double t252;
  double t254;
  double t259;
  double t261;
  double t273;
  double t283;
  double t295;
  double t296;
  double t298;
  double t3;
  double t301;
  double t304;
  double t305;
  double t307;
  double t308;
  double t31;
  double t311;
  double t32;
  double t326;
  double t327;
  double t329;
  double t332;
  double t333;
  double t34;
  double t349;
  double t35;
  double t352;
  double t4;
  double t41;
  double t45;
  double t5;
  double t50;
  double t51;
  double t56;
  double t57;
  double t6;
  double t60;
  double t66;
  double t67;
  double t68;
  double t74;
  double t76;
  double t78;
  double t79;
  double t8;
  double t81;
  double t83;
  double t85;
  double t87;
  double t89;
  double t9;
  double t91;
  double t93;
  double t95;
  double t97;
  {
    t1 = v[0];
    t2 = t1*t1;
    t3 = v[1];
    t4 = t3*t3;
    t5 = v[2];
    t6 = t5*t5;
    t8 = sqrt(1.0-t2-t4-t6);
    t9 = qr0[1];
    t11 = qr0[0];
    t13 = qr0[3];
    t15 = qr0[2];
    t17 = t8*t9+t11*t1+t13*t3-t5*t15;
    t18 = M[0];
    t24 = t8*t15+t3*t11+t5*t9-t13*t1;
    t25 = M[1];
    t31 = t8*t13+t5*t11+t1*t15-t3*t9;
    t32 = M[2];
    t34 = -t17*t18-t24*t25-t31*t32;
    t35 = -t17;
    t41 = t11*t8-t1*t9-t3*t15-t5*t13;
    t45 = t41*t18+t24*t32-t31*t25;
    t50 = t41*t25+t31*t18-t17*t32;
    t51 = -t31;
    t56 = t41*t32+t17*t25-t24*t18;
    t57 = -t24;
    t60 = t34*t35+t41*t45+t50*t51-t56*t57+t[0];
    t66 = t34*t51+t41*t56+t45*t57-t50*t35+t[2];
    t67 = 1/t66;
    jacmKRT[0][0] = t60*t67;
    t68 = a[3];
    t74 = t34*t57+t41*t50+t56*t35-t45*t51+t[1];
    jacmKRT[1][0] = t68*t74*t67;
    jacmKRT[0][1] = 1.0;
    jacmKRT[1][1] = 0.0;
    jacmKRT[0][2] = 0.0;
    jacmKRT[1][2] = 1.0;
    jacmKRT[0][3] = 0.0;
    t76 = a[0];
    jacmKRT[1][3] = t76*t74*t67;
    jacmKRT[0][4] = t74*t67;
    jacmKRT[1][4] = 0.0;
    t78 = 1/t8;
    t79 = t78*t9;
    t81 = -t79*t1+t11;
    t83 = t78*t15;
    t85 = -t83*t1-t13;
    t87 = t78*t13;
    t89 = -t87*t1+t15;
    t91 = -t81*t18-t85*t25-t89*t32;
    t93 = -t81;
    t95 = t78*t11;
    t97 = -t95*t1-t9;
    t102 = t97*t18+t85*t32-t89*t25;
    t107 = t97*t25+t89*t18-t81*t32;
    t109 = -t89;
    t114 = t97*t32+t81*t25-t85*t18;
    t116 = -t85;
    t120 = a[4];
    t129 = t91*t57+t34*t116+t97*t50+t41*t107+t114*t35+t56*t93-t102*t51-t45*t109
;
    t131 = a[1];
    t140 = t91*t51+t34*t109+t97*t56+t41*t114+t102*t57+t45*t116-t107*t35-t50*t93
;
    t148 = t66*t66;
    t149 = 1/t148;
    t150 = (t76*t60+t120*t74+t131*t66)*t149;
    jacmKRT[0][5] = (t76*(t91*t35+t34*t93+t97*t45+t41*t102+t107*t51+t50*t109-
t114*t57-t56*t116)+t129*t120+t131*t140)*t67-t150*t140;
    t152 = t76*t68;
    t154 = a[2];
    t161 = (t152*t74+t154*t66)*t149;
    jacmKRT[1][5] = (t152*t129+t154*t140)*t67-t161*t140;
    t164 = -t79*t3+t13;
    t167 = -t83*t3+t11;
    t170 = -t87*t3-t9;
    t172 = -t164*t18-t167*t25-t170*t32;
    t174 = -t164;
    t177 = -t95*t3-t15;
    t182 = t177*t18+t167*t32-t170*t25;
    t187 = t177*t25+t170*t18-t164*t32;
    t189 = -t170;
    t194 = t177*t32+t164*t25-t167*t18;
    t196 = -t167;
    t208 = t172*t57+t34*t196+t177*t50+t41*t187+t194*t35+t56*t174-t182*t51-t45*
t189;
    t218 = t172*t51+t34*t189+t177*t56+t41*t194+t182*t57+t45*t196-t187*t35-t50*
t174;
    jacmKRT[0][6] = (t76*(t172*t35+t34*t174+t177*t45+t41*t182+t187*t51+t50*t189
-t194*t57-t56*t196)+t120*t208+t131*t218)*t67-t150*t218;
    jacmKRT[1][6] = (t152*t208+t154*t218)*t67-t161*t218;
    t229 = -t79*t5-t15;
    t232 = -t83*t5+t9;
    t235 = -t87*t5+t11;
    t237 = -t229*t18-t232*t25-t235*t32;
    t239 = -t229;
    t242 = -t95*t5-t13;
    t247 = t242*t18+t232*t32-t235*t25;
    t252 = t242*t25+t235*t18-t229*t32;
    t254 = -t235;
    t259 = t242*t32+t229*t25-t232*t18;
    t261 = -t232;
    t273 = t237*t57+t261*t34+t242*t50+t41*t252+t259*t35+t56*t239-t247*t51-t45*
t254;
    t283 = t237*t51+t34*t254+t242*t56+t41*t259+t247*t57+t45*t261-t252*t35-t50*
t239;
    jacmKRT[0][7] = (t76*(t237*t35+t34*t239+t242*t45+t41*t247+t252*t51+t50*t254
-t259*t57-t56*t261)+t120*t273+t131*t283)*t67-t150*t283;
    jacmKRT[1][7] = (t152*t273+t154*t283)*t67-t161*t283;
    jacmKRT[0][8] = t76*t67;
    jacmKRT[1][8] = 0.0;
    jacmKRT[0][9] = t120*t67;
    jacmKRT[1][9] = t152*t67;
    jacmKRT[0][10] = t131*t67-t150;
    jacmKRT[1][10] = t154*t67-t161;
    t295 = t35*t35;
    t296 = t41*t41;
    t298 = t57*t57;
    t301 = t35*t57;
    t304 = t41*t51;
    t305 = 2.0*t301+t41*t31-t304;
    t307 = t35*t51;
    t308 = t41*t57;
    t311 = t307+2.0*t308-t31*t35;
    jacmS[0][0] = (t76*(t295+t296+t31*t51-t298)+t120*t305+t131*t311)*t67-t150*
t311;
    jacmS[1][0] = (t152*t305+t154*t311)*t67-t161*t311;
    t326 = t51*t51;
    t327 = t298+t296+t17*t35-t326;
    t329 = t57*t51;
    t332 = t41*t35;
    t333 = 2.0*t329+t41*t17-t332;
    jacmS[0][1] = (t76*(t301+2.0*t304-t17*t57)+t120*t327+t131*t333)*t67-t150*
t333;
    jacmS[1][1] = (t152*t327+t154*t333)*t67-t161*t333;
    t349 = t329+2.0*t332-t24*t51;
    t352 = t326+t296+t24*t57-t295;
    jacmS[0][2] = (t76*(2.0*t307+t41*t24-t308)+t120*t349+t131*t352)*t67-t150*
t352;
    jacmS[1][2] = (t152*t349+t154*t352)*t67-t161*t352;
    return;
  }
}

#include <math.h>
void calcImgProjJacKRT(double a[5],double qr0[4],double v[3],double t[3],
double M[3],double jacmKRT[2][11])
{
  double t1;
  double t102;
  double t107;
  double t109;
  double t11;
  double t114;
  double t116;
  double t120;
  double t129;
  double t13;
  double t131;
  double t140;
  double t148;
  double t149;
  double t15;
  double t150;
  double t152;
  double t154;
  double t161;
  double t164;
  double t167;
  double t17;
  double t170;
  double t172;
  double t174;
  double t177;
  double t18;
  double t182;
  double t187;
  double t189;
  double t194;
  double t196;
  double t2;
  double t208;
  double t218;
  double t229;
  double t232;
  double t235;
  double t237;
  double t239;
  double t24;
  double t242;
  double t247;
  double t25;
  double t252;
  double t254;
  double t259;
  double t261;
  double t273;
  double t283;
  double t3;
  double t31;
  double t32;
  double t34;
  double t35;
  double t4;
  double t41;
  double t45;
  double t5;
  double t50;
  double t51;
  double t56;
  double t57;
  double t6;
  double t60;
  double t66;
  double t67;
  double t68;
  double t74;
  double t76;
  double t78;
  double t79;
  double t8;
  double t81;
  double t83;
  double t85;
  double t87;
  double t89;
  double t9;
  double t91;
  double t93;
  double t95;
  double t97;
  {
    t1 = v[0];
    t2 = t1*t1;
    t3 = v[1];
    t4 = t3*t3;
    t5 = v[2];
    t6 = t5*t5;
    t8 = sqrt(1.0-t2-t4-t6);
    t9 = qr0[1];
    t11 = qr0[0];
    t13 = qr0[3];
    t15 = qr0[2];
    t17 = t8*t9+t11*t1+t13*t3-t5*t15;
    t18 = M[0];
    t24 = t8*t15+t3*t11+t5*t9-t13*t1;
    t25 = M[1];
    t31 = t8*t13+t5*t11+t1*t15-t3*t9;
    t32 = M[2];
    t34 = -t17*t18-t24*t25-t31*t32;
    t35 = -t17;
    t41 = t11*t8-t1*t9-t3*t15-t5*t13;
    t45 = t41*t18+t24*t32-t31*t25;
    t50 = t41*t25+t31*t18-t17*t32;
    t51 = -t31;
    t56 = t41*t32+t17*t25-t24*t18;
    t57 = -t24;
    t60 = t34*t35+t41*t45+t50*t51-t56*t57+t[0];
    t66 = t34*t51+t41*t56+t45*t57-t50*t35+t[2];
    t67 = 1/t66;
    jacmKRT[0][0] = t60*t67;
    t68 = a[3];
    t74 = t34*t57+t41*t50+t56*t35-t45*t51+t[1];
    jacmKRT[1][0] = t68*t74*t67;
    jacmKRT[0][1] = 1.0;
    jacmKRT[1][1] = 0.0;
    jacmKRT[0][2] = 0.0;
    jacmKRT[1][2] = 1.0;
    jacmKRT[0][3] = 0.0;
    t76 = a[0];
    jacmKRT[1][3] = t76*t74*t67;
    jacmKRT[0][4] = t74*t67;
    jacmKRT[1][4] = 0.0;
    t78 = 1/t8;
    t79 = t78*t9;
    t81 = -t79*t1+t11;
    t83 = t78*t15;
    t85 = -t83*t1-t13;
    t87 = t78*t13;
    t89 = -t87*t1+t15;
    t91 = -t81*t18-t85*t25-t89*t32;
    t93 = -t81;
    t95 = t78*t11;
    t97 = -t95*t1-t9;
    t102 = t97*t18+t85*t32-t89*t25;
    t107 = t97*t25+t89*t18-t81*t32;
    t109 = -t89;
    t114 = t97*t32+t81*t25-t85*t18;
    t116 = -t85;
    t120 = a[4];
    t129 = t91*t57+t34*t116+t97*t50+t41*t107+t114*t35+t56*t93-t102*t51-t45*t109
;
    t131 = a[1];
    t140 = t91*t51+t34*t109+t97*t56+t41*t114+t102*t57+t45*t116-t107*t35-t50*t93
;
    t148 = t66*t66;
    t149 = 1/t148;
    t150 = (t76*t60+t120*t74+t131*t66)*t149;
    jacmKRT[0][5] = (t76*(t91*t35+t34*t93+t97*t45+t41*t102+t107*t51+t50*t109-
t114*t57-t56*t116)+t129*t120+t131*t140)*t67-t150*t140;
    t152 = t76*t68;
    t154 = a[2];
    t161 = (t152*t74+t154*t66)*t149;
    jacmKRT[1][5] = (t152*t129+t154*t140)*t67-t161*t140;
    t164 = -t79*t3+t13;
    t167 = -t83*t3+t11;
    t170 = -t87*t3-t9;
    t172 = -t164*t18-t167*t25-t170*t32;
    t174 = -t164;
    t177 = -t95*t3-t15;
    t182 = t177*t18+t167*t32-t170*t25;
    t187 = t177*t25+t170*t18-t164*t32;
    t189 = -t170;
    t194 = t177*t32+t164*t25-t167*t18;
    t196 = -t167;
    t208 = t172*t57+t34*t196+t177*t50+t41*t187+t194*t35+t56*t174-t182*t51-t45*
t189;
    t218 = t172*t51+t34*t189+t177*t56+t41*t194+t182*t57+t45*t196-t187*t35-t50*
t174;
    jacmKRT[0][6] = (t76*(t172*t35+t34*t174+t177*t45+t41*t182+t187*t51+t50*t189
-t194*t57-t56*t196)+t120*t208+t131*t218)*t67-t150*t218;
    jacmKRT[1][6] = (t152*t208+t154*t218)*t67-t161*t218;
    t229 = -t79*t5-t15;
    t232 = -t83*t5+t9;
    t235 = -t87*t5+t11;
    t237 = -t229*t18-t232*t25-t235*t32;
    t239 = -t229;
    t242 = -t95*t5-t13;
    t247 = t242*t18+t232*t32-t235*t25;
    t252 = t242*t25+t235*t18-t229*t32;
    t254 = -t235;
    t259 = t242*t32+t229*t25-t232*t18;
    t261 = -t232;
    t273 = t237*t57+t261*t34+t242*t50+t41*t252+t259*t35+t56*t239-t247*t51-t45*
t254;
    t283 = t237*t51+t34*t254+t242*t56+t41*t259+t247*t57+t45*t261-t252*t35-t50*
t239;
    jacmKRT[0][7] = (t76*(t237*t35+t34*t239+t242*t45+t41*t247+t252*t51+t50*t254
-t259*t57-t56*t261)+t120*t273+t131*t283)*t67-t150*t283;
    jacmKRT[1][7] = (t152*t273+t154*t283)*t67-t161*t283;
    jacmKRT[0][8] = t76*t67;
    jacmKRT[1][8] = 0.0;
    jacmKRT[0][9] = t120*t67;
    jacmKRT[1][9] = t152*t67;
    jacmKRT[0][10] = t131*t67-t150;
    jacmKRT[1][10] = t154*t67-t161;
    return;
  }
}

#include <math.h>
void calcImgProjJacS(double a[5],double qr0[4],double v[3],double t[3],
double M[3],double jacmS[2][3])
{
  double t1;
  double t10;
  double t101;
  double t102;
  double t103;
  double t106;
  double t108;
  double t115;
  double t12;
  double t122;
  double t123;
  double t125;
  double t128;
  double t129;
  double t14;
  double t145;
  double t148;
  double t16;
  double t18;
  double t19;
  double t2;
  double t24;
  double t25;
  double t3;
  double t30;
  double t31;
  double t37;
  double t38;
  double t4;
  double t41;
  double t42;
  double t45;
  double t46;
  double t48;
  double t49;
  double t5;
  double t50;
  double t53;
  double t56;
  double t57;
  double t59;
  double t6;
  double t60;
  double t62;
  double t64;
  double t69;
  double t7;
  double t74;
  double t79;
  double t82;
  double t83;
  double t9;
  double t97;
  {
    t1 = a[0];
    t2 = v[0];
    t3 = t2*t2;
    t4 = v[1];
    t5 = t4*t4;
    t6 = v[2];
    t7 = t6*t6;
    t9 = sqrt(1.0-t3-t5-t7);
    t10 = qr0[1];
    t12 = qr0[0];
    t14 = qr0[3];
    t16 = qr0[2];
    t18 = -t9*t10-t12*t2-t4*t14+t6*t16;
    t19 = t18*t18;
    t24 = t9*t12-t2*t10-t16*t4-t6*t14;
    t25 = t24*t24;
    t30 = t9*t14+t12*t6+t16*t2-t4*t10;
    t31 = -t30;
    t37 = -t9*t16-t12*t4-t6*t10+t2*t14;
    t38 = t37*t37;
    t41 = a[4];
    t42 = t18*t37;
    t45 = t24*t31;
    t46 = 2.0*t42+t24*t30-t45;
    t48 = a[1];
    t49 = t31*t18;
    t50 = t24*t37;
    t53 = t49+2.0*t50-t30*t18;
    t56 = -t18;
    t57 = M[0];
    t59 = -t37;
    t60 = M[1];
    t62 = M[2];
    t64 = -t56*t57-t59*t60-t30*t62;
    t69 = t24*t62+t56*t60-t59*t57;
    t74 = t24*t57+t59*t62-t30*t60;
    t79 = t24*t60+t30*t57-t56*t62;
    t82 = t64*t31+t24*t69+t74*t37-t18*t79+t[2];
    t83 = 1/t82;
    t97 = t64*t37+t24*t79+t69*t18-t74*t31+t[1];
    t101 = t82*t82;
    t102 = 1/t101;
    t103 = (t1*(t64*t18+t24*t74+t79*t31-t69*t37+t[0])+t41*t97+t48*t82)*t102;
    jacmS[0][0] = (t1*(t19+t25+t30*t31-t38)+t41*t46+t48*t53)*t83-t103*t53;
    t106 = t1*a[3];
    t108 = a[2];
    t115 = (t106*t97+t108*t82)*t102;
    jacmS[1][0] = (t106*t46+t108*t53)*t83-t115*t53;
    t122 = t31*t31;
    t123 = t38+t25+t56*t18-t122;
    t125 = t31*t37;
    t128 = t24*t18;
    t129 = 2.0*t125+t24*t56-t128;
    jacmS[0][1] = (t1*(t42+2.0*t45-t56*t37)+t123*t41+t48*t129)*t83-t103*t129;
    jacmS[1][1] = (t106*t123+t108*t129)*t83-t129*t115;
    t145 = t125+2.0*t128-t59*t31;
    t148 = t122+t25+t59*t37-t19;
    jacmS[0][2] = (t1*(2.0*t49+t24*t59-t50)+t41*t145+t148*t48)*t83-t103*t148;
    jacmS[1][2] = (t106*t145+t148*t108)*t83-t115*t148;
    return;
  }
}

#include <math.h>
void calcImgProjJacRTS(double a[5],double qr0[4],double v[3],double t[3],
double M[3],double jacmRT[2][6],double jacmS[2][3])
{
  double t1;
  double t10;
  double t107;
  double t109;
  double t11;
  double t118;
  double t12;
  double t126;
  double t127;
  double t14;
  double t141;
  double t145;
  double t146;
  double t147;
  double t15;
  double t150;
  double t152;
  double t159;
  double t16;
  double t162;
  double t165;
  double t168;
  double t170;
  double t172;
  double t175;
  double t18;
  double t180;
  double t185;
  double t187;
  double t19;
  double t192;
  double t194;
  double t2;
  double t206;
  double t21;
  double t216;
  double t22;
  double t227;
  double t23;
  double t230;
  double t233;
  double t235;
  double t237;
  double t240;
  double t245;
  double t25;
  double t250;
  double t252;
  double t257;
  double t259;
  double t27;
  double t271;
  double t28;
  double t281;
  double t293;
  double t294;
  double t296;
  double t299;
  double t3;
  double t30;
  double t302;
  double t303;
  double t305;
  double t306;
  double t309;
  double t324;
  double t325;
  double t327;
  double t330;
  double t331;
  double t347;
  double t35;
  double t350;
  double t37;
  double t4;
  double t43;
  double t49;
  double t5;
  double t51;
  double t52;
  double t54;
  double t56;
  double t6;
  double t61;
  double t65;
  double t7;
  double t70;
  double t75;
  double t76;
  double t81;
  double t82;
  double t87;
  double t88;
  double t9;
  double t93;
  double t94;
  double t98;
  {
    t1 = a[0];
    t2 = v[0];
    t3 = t2*t2;
    t4 = v[1];
    t5 = t4*t4;
    t6 = v[2];
    t7 = t6*t6;
    t9 = sqrt(1.0-t3-t5-t7);
    t10 = 1/t9;
    t11 = qr0[1];
    t12 = t11*t10;
    t14 = qr0[0];
    t15 = -t12*t2+t14;
    t16 = M[0];
    t18 = qr0[2];
    t19 = t18*t10;
    t21 = qr0[3];
    t22 = -t19*t2-t21;
    t23 = M[1];
    t25 = t10*t21;
    t27 = -t25*t2+t18;
    t28 = M[2];
    t30 = -t15*t16-t22*t23-t27*t28;
    t35 = -t9*t11-t2*t14-t4*t21+t6*t18;
    t37 = -t35;
    t43 = t9*t18+t4*t14+t6*t11-t2*t21;
    t49 = t9*t21+t6*t14+t2*t18-t11*t4;
    t51 = -t37*t16-t43*t23-t49*t28;
    t52 = -t15;
    t54 = t10*t14;
    t56 = -t54*t2-t11;
    t61 = t9*t14-t2*t11-t4*t18-t6*t21;
    t65 = t61*t16+t43*t28-t23*t49;
    t70 = t56*t16+t22*t28-t23*t27;
    t75 = t56*t23+t27*t16-t28*t15;
    t76 = -t49;
    t81 = t61*t23+t49*t16-t37*t28;
    t82 = -t27;
    t87 = t56*t28+t23*t15-t22*t16;
    t88 = -t43;
    t93 = t61*t28+t37*t23-t43*t16;
    t94 = -t22;
    t98 = a[4];
    t107 = t30*t88+t94*t51+t56*t81+t61*t75+t87*t35+t93*t52-t70*t76-t82*t65;
    t109 = a[1];
    t118 = t30*t76+t82*t51+t56*t93+t61*t87+t70*t88+t65*t94-t35*t75-t81*t52;
    t126 = t76*t51+t61*t93+t65*t88-t81*t35+t[2];
    t127 = 1/t126;
    t141 = t51*t88+t61*t81+t93*t35-t65*t76+t[1];
    t145 = t126*t126;
    t146 = 1/t145;
    t147 = (t1*(t35*t51+t61*t65+t81*t76-t93*t88+t[0])+t98*t141+t126*t109)*t146;
    jacmRT[0][0] = (t1*(t30*t35+t52*t51+t56*t65+t61*t70+t76*t75+t81*t82-t88*t87
-t93*t94)+t98*t107+t109*t118)*t127-t118*t147;
    t150 = t1*a[3];
    t152 = a[2];
    t159 = (t150*t141+t126*t152)*t146;
    jacmRT[1][0] = (t107*t150+t152*t118)*t127-t159*t118;
    t162 = -t12*t4+t21;
    t165 = -t19*t4+t14;
    t168 = -t25*t4-t11;
    t170 = -t162*t16-t165*t23-t168*t28;
    t172 = -t162;
    t175 = -t54*t4-t18;
    t180 = t175*t16+t165*t28-t168*t23;
    t185 = t175*t23+t168*t16-t162*t28;
    t187 = -t168;
    t192 = t175*t28+t162*t23-t165*t16;
    t194 = -t165;
    t206 = t170*t88+t51*t194+t175*t81+t61*t185+t192*t35+t93*t172-t76*t180-t65*
t187;
    t216 = t170*t76+t51*t187+t93*t175+t61*t192+t180*t88+t65*t194-t185*t35-t81*
t172;
    jacmRT[0][1] = (t1*(t170*t35+t172*t51+t175*t65+t180*t61+t185*t76+t81*t187-
t192*t88-t93*t194)+t98*t206+t109*t216)*t127-t147*t216;
    jacmRT[1][1] = (t150*t206+t152*t216)*t127-t159*t216;
    t227 = -t12*t6-t18;
    t230 = -t19*t6+t11;
    t233 = -t25*t6+t14;
    t235 = -t227*t16-t23*t230-t233*t28;
    t237 = -t227;
    t240 = -t54*t6-t21;
    t245 = t240*t16+t230*t28-t233*t23;
    t250 = t23*t240+t233*t16-t227*t28;
    t252 = -t233;
    t257 = t240*t28+t227*t23-t230*t16;
    t259 = -t230;
    t271 = t235*t88+t51*t259+t81*t240+t61*t250+t257*t35+t93*t237-t245*t76-t65*
t252;
    t281 = t235*t76+t51*t252+t240*t93+t61*t257+t245*t88+t259*t65-t250*t35-t81*
t237;
    jacmRT[0][2] = (t1*(t235*t35+t237*t51+t240*t65+t61*t245+t250*t76+t81*t252-
t257*t88-t93*t259)+t271*t98+t281*t109)*t127-t147*t281;
    jacmRT[1][2] = (t150*t271+t281*t152)*t127-t159*t281;
    jacmRT[0][3] = t127*t1;
    jacmRT[1][3] = 0.0;
    jacmRT[0][4] = t98*t127;
    jacmRT[1][4] = t150*t127;
    jacmRT[0][5] = t109*t127-t147;
    jacmRT[1][5] = t152*t127-t159;
    t293 = t35*t35;
    t294 = t61*t61;
    t296 = t88*t88;
    t299 = t35*t88;
    t302 = t61*t76;
    t303 = 2.0*t299+t61*t49-t302;
    t305 = t35*t76;
    t306 = t61*t88;
    t309 = t305+2.0*t306-t49*t35;
    jacmS[0][0] = (t1*(t293+t294+t49*t76-t296)+t98*t303+t109*t309)*t127-t147*
t309;
    jacmS[1][0] = (t150*t303+t152*t309)*t127-t159*t309;
    t324 = t76*t76;
    t325 = t296+t294+t35*t37-t324;
    t327 = t76*t88;
    t330 = t61*t35;
    t331 = 2.0*t327+t61*t37-t330;
    jacmS[0][1] = (t1*(t299+2.0*t302-t37*t88)+t98*t325+t109*t331)*t127-t147*
t331;
    jacmS[1][1] = (t150*t325+t152*t331)*t127-t159*t331;
    t347 = t327+2.0*t330-t43*t76;
    t350 = t324+t294+t43*t88-t293;
    jacmS[0][2] = (t1*(2.0*t305+t61*t43-t306)+t98*t347+t350*t109)*t127-t147*
t350;
    jacmS[1][2] = (t150*t347+t152*t350)*t127-t159*t350;
    return;
  }
}

#include <math.h>
void calcImgProjJacRT(double a[5],double qr0[4],double v[3],double t[3],
double M[3],double jacmRT[2][6])
{
  double t1;
  double t10;
  double t107;
  double t109;
  double t11;
  double t118;
  double t12;
  double t126;
  double t127;
  double t14;
  double t141;
  double t145;
  double t146;
  double t147;
  double t15;
  double t150;
  double t152;
  double t159;
  double t16;
  double t162;
  double t165;
  double t168;
  double t170;
  double t172;
  double t175;
  double t18;
  double t180;
  double t185;
  double t187;
  double t19;
  double t192;
  double t194;
  double t2;
  double t206;
  double t21;
  double t216;
  double t22;
  double t227;
  double t23;
  double t230;
  double t233;
  double t235;
  double t237;
  double t240;
  double t245;
  double t25;
  double t250;
  double t252;
  double t257;
  double t259;
  double t27;
  double t271;
  double t28;
  double t281;
  double t3;
  double t30;
  double t35;
  double t37;
  double t4;
  double t43;
  double t49;
  double t5;
  double t51;
  double t52;
  double t54;
  double t56;
  double t6;
  double t61;
  double t65;
  double t7;
  double t70;
  double t75;
  double t76;
  double t81;
  double t82;
  double t87;
  double t88;
  double t9;
  double t93;
  double t94;
  double t98;
  {
    t1 = a[0];
    t2 = v[0];
    t3 = t2*t2;
    t4 = v[1];
    t5 = t4*t4;
    t6 = v[2];
    t7 = t6*t6;
    t9 = sqrt(1.0-t3-t5-t7);
    t10 = 1/t9;
    t11 = qr0[1];
    t12 = t11*t10;
    t14 = qr0[0];
    t15 = -t12*t2+t14;
    t16 = M[0];
    t18 = qr0[2];
    t19 = t18*t10;
    t21 = qr0[3];
    t22 = -t19*t2-t21;
    t23 = M[1];
    t25 = t10*t21;
    t27 = -t25*t2+t18;
    t28 = M[2];
    t30 = -t15*t16-t22*t23-t27*t28;
    t35 = -t9*t11-t2*t14-t4*t21+t6*t18;
    t37 = -t35;
    t43 = t9*t18+t4*t14+t6*t11-t2*t21;
    t49 = t9*t21+t6*t14+t2*t18-t11*t4;
    t51 = -t37*t16-t43*t23-t49*t28;
    t52 = -t15;
    t54 = t10*t14;
    t56 = -t54*t2-t11;
    t61 = t9*t14-t2*t11-t4*t18-t6*t21;
    t65 = t61*t16+t43*t28-t23*t49;
    t70 = t56*t16+t22*t28-t23*t27;
    t75 = t56*t23+t27*t16-t28*t15;
    t76 = -t49;
    t81 = t61*t23+t49*t16-t37*t28;
    t82 = -t27;
    t87 = t56*t28+t23*t15-t22*t16;
    t88 = -t43;
    t93 = t61*t28+t37*t23-t43*t16;
    t94 = -t22;
    t98 = a[4];
    t107 = t30*t88+t94*t51+t56*t81+t61*t75+t87*t35+t93*t52-t70*t76-t82*t65;
    t109 = a[1];
    t118 = t30*t76+t82*t51+t56*t93+t61*t87+t70*t88+t65*t94-t35*t75-t81*t52;
    t126 = t76*t51+t61*t93+t65*t88-t81*t35+t[2];
    t127 = 1/t126;
    t141 = t51*t88+t61*t81+t93*t35-t65*t76+t[1];
    t145 = t126*t126;
    t146 = 1/t145;
    t147 = (t1*(t35*t51+t61*t65+t81*t76-t93*t88+t[0])+t98*t141+t126*t109)*t146;
    jacmRT[0][0] = (t1*(t30*t35+t52*t51+t56*t65+t61*t70+t76*t75+t81*t82-t88*t87
-t93*t94)+t98*t107+t109*t118)*t127-t118*t147;
    t150 = t1*a[3];
    t152 = a[2];
    t159 = (t150*t141+t126*t152)*t146;
    jacmRT[1][0] = (t107*t150+t152*t118)*t127-t159*t118;
    t162 = -t12*t4+t21;
    t165 = -t19*t4+t14;
    t168 = -t25*t4-t11;
    t170 = -t162*t16-t165*t23-t168*t28;
    t172 = -t162;
    t175 = -t54*t4-t18;
    t180 = t175*t16+t165*t28-t168*t23;
    t185 = t175*t23+t168*t16-t162*t28;
    t187 = -t168;
    t192 = t175*t28+t162*t23-t165*t16;
    t194 = -t165;
    t206 = t170*t88+t51*t194+t175*t81+t61*t185+t192*t35+t93*t172-t76*t180-t65*
t187;
    t216 = t170*t76+t51*t187+t93*t175+t61*t192+t180*t88+t65*t194-t185*t35-t81*
t172;
    jacmRT[0][1] = (t1*(t170*t35+t172*t51+t175*t65+t180*t61+t185*t76+t81*t187-
t192*t88-t93*t194)+t98*t206+t109*t216)*t127-t147*t216;
    jacmRT[1][1] = (t150*t206+t152*t216)*t127-t159*t216;
    t227 = -t12*t6-t18;
    t230 = -t19*t6+t11;
    t233 = -t25*t6+t14;
    t235 = -t227*t16-t23*t230-t233*t28;
    t237 = -t227;
    t240 = -t54*t6-t21;
    t245 = t240*t16+t230*t28-t233*t23;
    t250 = t23*t240+t233*t16-t227*t28;
    t252 = -t233;
    t257 = t240*t28+t227*t23-t230*t16;
    t259 = -t230;
    t271 = t235*t88+t51*t259+t81*t240+t61*t250+t257*t35+t93*t237-t245*t76-t65*
t252;
    t281 = t235*t76+t51*t252+t240*t93+t61*t257+t245*t88+t259*t65-t250*t35-t81*
t237;
    jacmRT[0][2] = (t1*(t235*t35+t237*t51+t240*t65+t61*t245+t250*t76+t81*t252-
t257*t88-t93*t259)+t271*t98+t281*t109)*t127-t147*t281;
    jacmRT[1][2] = (t150*t271+t281*t152)*t127-t159*t281;
    jacmRT[0][3] = t127*t1;
    jacmRT[1][3] = 0.0;
    jacmRT[0][4] = t98*t127;
    jacmRT[1][4] = t150*t127;
    jacmRT[0][5] = t109*t127-t147;
    jacmRT[1][5] = t152*t127-t159;
    return;
  }
}

#include <math.h>
void calcDistImgProj(double a[5],double kc[5],double qr0[4],double v[3],
double t[3],double M[3],double n[2])
{
  double t1;
  double t10;
  double t113;
  double t12;
  double t14;
  double t16;
  double t18;
  double t19;
  double t2;
  double t25;
  double t26;
  double t3;
  double t32;
  double t33;
  double t35;
  double t36;
  double t4;
  double t42;
  double t46;
  double t5;
  double t51;
  double t52;
  double t57;
  double t58;
  double t6;
  double t61;
  double t62;
  double t68;
  double t69;
  double t7;
  double t70;
  double t71;
  double t77;
  double t78;
  double t79;
  double t80;
  double t89;
  double t9;
  double t91;
  double t93;
  double t95;
  double t98;
  {
    t1 = a[0];
    t2 = v[0];
    t3 = t2*t2;
    t4 = v[1];
    t5 = t4*t4;
    t6 = v[2];
    t7 = t6*t6;
    t9 = sqrt(1.0-t3-t5-t7);
    t10 = qr0[1];
    t12 = qr0[0];
    t14 = qr0[3];
    t16 = qr0[2];
    t18 = t9*t10+t12*t2+t4*t14-t6*t16;
    t19 = M[0];
    t25 = t9*t16+t12*t4+t6*t10-t2*t14;
    t26 = M[1];
    t32 = t9*t14+t12*t6+t2*t16-t4*t10;
    t33 = M[2];
    t35 = -t18*t19-t25*t26-t33*t32;
    t36 = -t18;
    t42 = t9*t12-t2*t10-t4*t16-t6*t14;
    t46 = t42*t19+t25*t33-t32*t26;
    t51 = t42*t26+t32*t19-t18*t33;
    t52 = -t32;
    t57 = t42*t33+t18*t26-t25*t19;
    t58 = -t25;
    t61 = t35*t36+t42*t46+t52*t51-t57*t58+t[0];
    t62 = t61*t61;
    t68 = t52*t35+t42*t57+t46*t58-t51*t36+t[2];
    t69 = t68*t68;
    t70 = 1/t69;
    t71 = t62*t70;
    t77 = t35*t58+t42*t51+t57*t36-t52*t46+t[1];
    t78 = t77*t77;
    t79 = t78*t70;
    t80 = t71+t79;
    t89 = 1.0+t80*(kc[0]+t80*(kc[1]+t80*kc[4]));
    t91 = 1/t68;
    t93 = kc[2];
    t95 = t70*t77;
    t98 = kc[3];
    t113 = t89*t77*t91+t93*(t71+3.0*t79)+2.0*t98*t61*t95;
    n[0] = t1*(t89*t61*t91+2.0*t93*t61*t95+t98*(3.0*t71+t79))+a[4]*t113+a[1];
    n[1] = t1*a[3]*t113+a[2];
    return;
  }
}

void calcDistImgProjFullR(double a[5],double kc[5],double qr0[4],double t[3],
double M[3],double n[2])
{
  double t1;
  double t11;
  double t13;
  double t17;
  double t2;
  double t22;
  double t27;
  double t3;
  double t30;
  double t31;
  double t37;
  double t38;
  double t39;
  double t40;
  double t46;
  double t47;
  double t48;
  double t49;
  double t5;
  double t58;
  double t6;
  double t60;
  double t62;
  double t64;
  double t67;
  double t8;
  double t82;
  double t9;
  {
    t1 = a[0];
    t2 = qr0[1];
    t3 = M[0];
    t5 = qr0[2];
    t6 = M[1];
    t8 = qr0[3];
    t9 = M[2];
    t11 = -t3*t2-t6*t5-t9*t8;
    t13 = qr0[0];
    t17 = t13*t3+t5*t9-t6*t8;
    t22 = t6*t13+t8*t3-t9*t2;
    t27 = t13*t9+t2*t6-t5*t3;
    t30 = -t2*t11+t13*t17-t8*t22+t27*t5+t[0];
    t31 = t30*t30;
    t37 = -t8*t11+t13*t27-t5*t17+t2*t22+t[2];
    t38 = t37*t37;
    t39 = 1/t38;
    t40 = t39*t31;
    t46 = -t5*t11+t13*t22-t27*t2+t17*t8+t[1];
    t47 = t46*t46;
    t48 = t47*t39;
    t49 = t40+t48;
    t58 = 1.0+t49*(kc[0]+t49*(kc[1]+t49*kc[4]));
    t60 = 1/t37;
    t62 = kc[2];
    t64 = t39*t46;
    t67 = kc[3];
    t82 = t58*t46*t60+t62*(t40+3.0*t48)+2.0*t67*t30*t64;
    n[0] = t1*(t58*t30*t60+2.0*t62*t30*t64+t67*(3.0*t40+t48))+a[4]*t82+a[1];
    n[1] = t1*a[3]*t82+a[2];
    return;
  }
}

#include <math.h>
void calcDistImgProjJacKDRTS(double a[5],double kc[5],double qr0[4],
double v[3],double t[3],double M[3],double jacmKDRT[2][16],double jacmS[2][3])
{
  double t1;
  double t101;
  double t102;
  double t105;
  double t107;
  double t11;
  double t110;
  double t111;
  double t113;
  double t115;
  double t117;
  double t119;
  double t120;
  double t122;
  double t13;
  double t137;
  double t140;
  double t147;
  double t148;
  double t15;
  double t150;
  double t152;
  double t154;
  double t156;
  double t158;
  double t160;
  double t162;
  double t164;
  double t166;
  double t17;
  double t171;
  double t176;
  double t178;
  double t18;
  double t183;
  double t185;
  double t187;
  double t188;
  double t190;
  double t191;
  double t2;
  double t200;
  double t201;
  double t210;
  double t211;
  double t212;
  double t213;
  double t215;
  double t222;
  double t227;
  double t232;
  double t233;
  double t236;
  double t24;
  double t25;
  double t265;
  double t268;
  double t271;
  double t274;
  double t276;
  double t278;
  double t281;
  double t286;
  double t291;
  double t293;
  double t298;
  double t3;
  double t300;
  double t302;
  double t303;
  double t31;
  double t312;
  double t313;
  double t32;
  double t322;
  double t323;
  double t324;
  double t326;
  double t333;
  double t338;
  double t34;
  double t343;
  double t346;
  double t35;
  double t375;
  double t378;
  double t381;
  double t384;
  double t386;
  double t388;
  double t391;
  double t396;
  double t4;
  double t401;
  double t403;
  double t408;
  double t41;
  double t410;
  double t412;
  double t413;
  double t422;
  double t423;
  double t432;
  double t433;
  double t434;
  double t436;
  double t443;
  double t448;
  double t45;
  double t453;
  double t456;
  double t485;
  double t491;
  double t496;
  double t499;
  double t5;
  double t50;
  double t501;
  double t503;
  double t51;
  double t510;
  double t513;
  double t514;
  double t523;
  double t532;
  double t535;
  double t542;
  double t56;
  double t563;
  double t565;
  double t566;
  double t568;
  double t569;
  double t57;
  double t570;
  double t571;
  double t572;
  double t575;
  double t576;
  double t577;
  double t580;
  double t581;
  double t582;
  double t583;
  double t585;
  double t592;
  double t597;
  double t6;
  double t60;
  double t602;
  double t605;
  double t61;
  double t634;
  double t638;
  double t639;
  double t640;
  double t643;
  double t644;
  double t645;
  double t647;
  double t648;
  double t649;
  double t650;
  double t652;
  double t659;
  double t664;
  double t669;
  double t67;
  double t672;
  double t68;
  double t69;
  double t70;
  double t701;
  double t705;
  double t706;
  double t708;
  double t709;
  double t712;
  double t713;
  double t714;
  double t716;
  double t723;
  double t728;
  double t733;
  double t736;
  double t76;
  double t765;
  double t77;
  double t78;
  double t79;
  double t8;
  double t82;
  double t84;
  double t86;
  double t88;
  double t89;
  double t9;
  double t90;
  double t92;
  double t93;
  double t94;
  double t97;
  double t99;
  {
    t1 = v[0];
    t2 = t1*t1;
    t3 = v[1];
    t4 = t3*t3;
    t5 = v[2];
    t6 = t5*t5;
    t8 = sqrt(1.0-t2-t4-t6);
    t9 = qr0[1];
    t11 = qr0[0];
    t13 = qr0[3];
    t15 = qr0[2];
    t17 = t9*t8+t11*t1+t13*t3-t5*t15;
    t18 = M[0];
    t24 = t8*t15+t11*t3+t5*t9-t1*t13;
    t25 = M[1];
    t31 = t8*t13+t5*t11+t1*t15-t3*t9;
    t32 = M[2];
    t34 = -t17*t18-t24*t25-t31*t32;
    t35 = -t17;
    t41 = t8*t11-t1*t9-t3*t15-t5*t13;
    t45 = t41*t18+t24*t32-t31*t25;
    t50 = t41*t25+t31*t18-t32*t17;
    t51 = -t31;
    t56 = t41*t32+t17*t25-t24*t18;
    t57 = -t24;
    t60 = t34*t35+t41*t45+t50*t51-t56*t57+t[0];
    t61 = t60*t60;
    t67 = t34*t51+t41*t56+t45*t57-t50*t35+t[2];
    t68 = t67*t67;
    t69 = 1/t68;
    t70 = t61*t69;
    t76 = t34*t57+t41*t50+t56*t35-t45*t51+t[1];
    t77 = t76*t76;
    t78 = t77*t69;
    t79 = t70+t78;
    t82 = kc[4];
    t84 = kc[1]+t79*t82;
    t86 = kc[0]+t79*t84;
    t88 = 1.0+t79*t86;
    t89 = t88*t60;
    t90 = 1/t67;
    t92 = kc[2];
    t93 = t92*t60;
    t94 = t69*t76;
    t97 = kc[3];
    t99 = 3.0*t70+t78;
    jacmKDRT[0][0] = t89*t90+2.0*t93*t94+t97*t99;
    t101 = a[3];
    t102 = t88*t76;
    t105 = t70+3.0*t78;
    t107 = t97*t60;
    t110 = t102*t90+t92*t105+2.0*t107*t94;
    jacmKDRT[1][0] = t101*t110;
    jacmKDRT[0][1] = 1.0;
    jacmKDRT[1][1] = 0.0;
    jacmKDRT[0][2] = 0.0;
    jacmKDRT[1][2] = 1.0;
    jacmKDRT[0][3] = 0.0;
    t111 = a[0];
    jacmKDRT[1][3] = t110*t111;
    jacmKDRT[0][4] = t110;
    jacmKDRT[1][4] = 0.0;
    t113 = t60*t90;
    t115 = a[4];
    t117 = t76*t90;
    jacmKDRT[0][5] = t111*t79*t113+t115*t79*t117;
    t119 = t111*t101;
    t120 = t79*t76;
    jacmKDRT[1][5] = t119*t120*t90;
    t122 = t79*t79;
    jacmKDRT[0][6] = t111*t122*t113+t115*t122*t117;
    jacmKDRT[1][6] = t119*t122*t76*t90;
    jacmKDRT[0][7] = 2.0*t111*t60*t94+t115*t105;
    jacmKDRT[1][7] = t119*t105;
    jacmKDRT[0][8] = t111*t99+2.0*t115*t60*t94;
    t137 = t60*t69;
    jacmKDRT[1][8] = 2.0*t119*t137*t76;
    t140 = t122*t79;
    jacmKDRT[0][9] = t111*t140*t113+t115*t140*t117;
    jacmKDRT[1][9] = t119*t140*t76*t90;
    t147 = 1/t8;
    t148 = t147*t9;
    t150 = -t148*t1+t11;
    t152 = t147*t15;
    t154 = -t152*t1-t13;
    t156 = t13*t147;
    t158 = -t156*t1+t15;
    t160 = -t150*t18-t154*t25-t158*t32;
    t162 = -t150;
    t164 = t147*t11;
    t166 = -t164*t1-t9;
    t171 = t166*t18+t154*t32-t158*t25;
    t176 = t166*t25+t158*t18-t150*t32;
    t178 = -t158;
    t183 = t32*t166+t150*t25-t154*t18;
    t185 = -t154;
    t187 = t160*t35+t34*t162+t166*t45+t41*t171+t176*t51+t50*t178-t183*t57-t56*
t185;
    t188 = t137*t187;
    t190 = 1/t68/t67;
    t191 = t61*t190;
    t200 = t160*t51+t34*t178+t166*t56+t41*t183+t171*t57+t45*t185-t176*t35-t50*
t162;
    t201 = t191*t200;
    t210 = t160*t57+t34*t185+t166*t50+t41*t176+t183*t35+t56*t162-t171*t51-t45*
t178;
    t211 = t94*t210;
    t212 = t77*t190;
    t213 = t212*t200;
    t215 = 2.0*t188-2.0*t201+2.0*t211-2.0*t213;
    t222 = t215*t86+t79*(t215*t84+t79*t215*t82);
    t227 = t69*t200;
    t232 = t190*t76;
    t233 = t232*t200;
    t236 = t69*t210;
    t265 = t222*t76*t90+t88*t210*t90-t102*t227+t92*(2.0*t188-2.0*t201+6.0*t211
-6.0*t213)+2.0*t97*t187*t94-4.0*t107*t233+2.0*t107*t236;
    jacmKDRT[0][10] = t111*(t222*t60*t90+t88*t187*t90-t89*t227+2.0*t92*t187*t94
-4.0*t93*t233+2.0*t93*t236+t97*(6.0*t188-6.0*t201+2.0*t211-2.0*t213))+t115*t265
;
    jacmKDRT[1][10] = t119*t265;
    t268 = -t148*t3+t13;
    t271 = -t152*t3+t11;
    t274 = -t156*t3-t9;
    t276 = -t268*t18-t271*t25-t274*t32;
    t278 = -t268;
    t281 = -t164*t3-t15;
    t286 = t281*t18+t271*t32-t274*t25;
    t291 = t281*t25+t274*t18-t32*t268;
    t293 = -t274;
    t298 = t281*t32+t268*t25-t271*t18;
    t300 = -t271;
    t302 = t276*t35+t34*t278+t281*t45+t41*t286+t291*t51+t50*t293-t298*t57-t56*
t300;
    t303 = t137*t302;
    t312 = t276*t51+t34*t293+t281*t56+t41*t298+t286*t57+t45*t300-t291*t35-t50*
t278;
    t313 = t191*t312;
    t322 = t276*t57+t34*t300+t281*t50+t41*t291+t298*t35+t56*t278-t286*t51-t45*
t293;
    t323 = t94*t322;
    t324 = t212*t312;
    t326 = 2.0*t303-2.0*t313+2.0*t323-2.0*t324;
    t333 = t326*t86+t79*(t326*t84+t79*t326*t82);
    t338 = t69*t312;
    t343 = t312*t232;
    t346 = t69*t322;
    t375 = t333*t76*t90+t88*t322*t90-t338*t102+t92*(2.0*t303-2.0*t313+6.0*t323
-6.0*t324)+2.0*t97*t302*t94-4.0*t107*t343+2.0*t346*t107;
    jacmKDRT[0][11] = t111*(t333*t60*t90+t88*t302*t90-t89*t338+2.0*t92*t302*t94
-4.0*t93*t343+2.0*t93*t346+t97*(6.0*t303-6.0*t313+2.0*t323-2.0*t324))+t115*t375
;
    jacmKDRT[1][11] = t119*t375;
    t378 = -t148*t5-t15;
    t381 = -t152*t5+t9;
    t384 = -t156*t5+t11;
    t386 = -t378*t18-t25*t381-t384*t32;
    t388 = -t378;
    t391 = -t164*t5-t13;
    t396 = t391*t18+t381*t32-t384*t25;
    t401 = t25*t391+t384*t18-t378*t32;
    t403 = -t384;
    t408 = t391*t32+t378*t25-t381*t18;
    t410 = -t381;
    t412 = t35*t386+t388*t34+t391*t45+t41*t396+t401*t51+t50*t403-t408*t57-t56*
t410;
    t413 = t137*t412;
    t422 = t386*t51+t34*t403+t56*t391+t41*t408+t396*t57+t45*t410-t401*t35-t50*
t388;
    t423 = t191*t422;
    t432 = t386*t57+t34*t410+t391*t50+t41*t401+t408*t35+t56*t388-t396*t51-t45*
t403;
    t433 = t94*t432;
    t434 = t212*t422;
    t436 = 2.0*t413-2.0*t423+2.0*t433-2.0*t434;
    t443 = t436*t86+t79*(t436*t84+t79*t436*t82);
    t448 = t69*t422;
    t453 = t232*t422;
    t456 = t69*t432;
    t485 = t443*t76*t90+t88*t432*t90-t102*t448+t92*(2.0*t413-2.0*t423+6.0*t433
-6.0*t434)+2.0*t97*t412*t94-4.0*t107*t453+2.0*t107*t456;
    jacmKDRT[0][12] = t111*(t443*t60*t90+t88*t412*t90-t89*t448+2.0*t92*t412*t94
-4.0*t93*t453+2.0*t93*t456+t97*(6.0*t413-6.0*t423+2.0*t433-2.0*t434))+t115*t485
;
    jacmKDRT[1][12] = t119*t485;
    t491 = t69*t82;
    t496 = 2.0*t137*t86+t79*(2.0*t137*t84+2.0*t79*t60*t491);
    t499 = t88*t90;
    t501 = t92*t69*t76;
    t503 = t107*t69;
    t510 = 2.0*t93*t69;
    t513 = 2.0*t97*t69*t76;
    t514 = t496*t76*t90+t510+t513;
    jacmKDRT[0][13] = t111*(t496*t60*t90+t499+2.0*t501+6.0*t503)+t115*t514;
    jacmKDRT[1][13] = t119*t514;
    t523 = 2.0*t94*t86+t79*(2.0*t94*t84+2.0*t120*t491);
    t532 = t523*t76*t90+t499+6.0*t501+2.0*t503;
    jacmKDRT[0][14] = t111*(t523*t60*t90+t510+t513)+t115*t532;
    jacmKDRT[1][14] = t119*t532;
    t535 = -2.0*t191-2.0*t212;
    t542 = t535*t86+t79*(t535*t84+t79*t535*t82);
    t563 = t542*t76*t90-t102*t69+t92*(-2.0*t191-6.0*t212)-4.0*t107*t232;
    jacmKDRT[0][15] = t111*(t542*t60*t90-t89*t69-4.0*t93*t232+t97*(-6.0*t191
-2.0*t212))+t115*t563;
    jacmKDRT[1][15] = t119*t563;
    t565 = t35*t35;
    t566 = t41*t41;
    t568 = t57*t57;
    t569 = t565+t566+t31*t51-t568;
    t570 = t137*t569;
    t571 = t35*t51;
    t572 = t41*t57;
    t575 = t571+2.0*t572-t31*t35;
    t576 = t191*t575;
    t577 = t35*t57;
    t580 = t41*t51;
    t581 = 2.0*t577+t41*t31-t580;
    t582 = t94*t581;
    t583 = t212*t575;
    t585 = 2.0*t570-2.0*t576+2.0*t582-2.0*t583;
    t592 = t585*t86+t79*(t84*t585+t79*t585*t82);
    t597 = t69*t575;
    t602 = t232*t575;
    t605 = t69*t581;
    t634 = t592*t76*t90+t88*t581*t90-t102*t597+t92*(2.0*t570-2.0*t576+6.0*t582
-6.0*t583)+2.0*t97*t569*t94-4.0*t107*t602+2.0*t107*t605;
    jacmS[0][0] = t111*(t592*t60*t90+t88*t569*t90-t89*t597+2.0*t92*t569*t94-4.0
*t93*t602+2.0*t93*t605+t97*(6.0*t570-6.0*t576+2.0*t582-2.0*t583))+t115*t634;
    jacmS[1][0] = t119*t634;
    t638 = t577+2.0*t580-t17*t57;
    t639 = t137*t638;
    t640 = t57*t51;
    t643 = t41*t35;
    t644 = 2.0*t640+t41*t17-t643;
    t645 = t191*t644;
    t647 = t51*t51;
    t648 = t568+t566+t17*t35-t647;
    t649 = t94*t648;
    t650 = t212*t644;
    t652 = 2.0*t639-2.0*t645+2.0*t649-2.0*t650;
    t659 = t652*t86+t79*(t652*t84+t79*t652*t82);
    t664 = t69*t644;
    t669 = t232*t644;
    t672 = t69*t648;
    t701 = t659*t76*t90+t88*t648*t90-t102*t664+t92*(2.0*t639-2.0*t645+6.0*t649
-6.0*t650)+2.0*t97*t638*t94-4.0*t107*t669+2.0*t107*t672;
    jacmS[0][1] = t111*(t659*t60*t90+t88*t638*t90-t89*t664+2.0*t92*t638*t94-4.0
*t93*t669+2.0*t93*t672+t97*(6.0*t639-6.0*t645+2.0*t649-2.0*t650))+t115*t701;
    jacmS[1][1] = t119*t701;
    t705 = 2.0*t571+t41*t24-t572;
    t706 = t137*t705;
    t708 = t647+t566+t57*t24-t565;
    t709 = t191*t708;
    t712 = t640+2.0*t643-t24*t51;
    t713 = t94*t712;
    t714 = t212*t708;
    t716 = 2.0*t706-2.0*t709+2.0*t713-2.0*t714;
    t723 = t716*t86+t79*(t716*t84+t79*t716*t82);
    t728 = t69*t708;
    t733 = t232*t708;
    t736 = t69*t712;
    t765 = t723*t76*t90+t88*t712*t90-t102*t728+t92*(2.0*t706-2.0*t709+6.0*t713
-6.0*t714)+2.0*t97*t705*t94-4.0*t107*t733+2.0*t107*t736;
    jacmS[0][2] = t111*(t723*t60*t90+t88*t705*t90-t89*t728+2.0*t92*t705*t94-4.0
*t93*t733+2.0*t93*t736+t97*(6.0*t706-6.0*t709+2.0*t713-2.0*t714))+t115*t765;
    jacmS[1][2] = t119*t765;
    return;
  }
}

#include <math.h>
void calcDistImgProjJacKDRT(double a[5],double kc[5],double qr0[4],double v[3],
double t[3],double M[3],double jacmKDRT[2][16])
{
  double t1;
  double t101;
  double t102;
  double t105;
  double t107;
  double t11;
  double t110;
  double t111;
  double t113;
  double t115;
  double t117;
  double t119;
  double t120;
  double t122;
  double t13;
  double t137;
  double t140;
  double t147;
  double t148;
  double t15;
  double t150;
  double t152;
  double t154;
  double t156;
  double t158;
  double t160;
  double t162;
  double t164;
  double t166;
  double t17;
  double t171;
  double t176;
  double t178;
  double t18;
  double t183;
  double t185;
  double t187;
  double t188;
  double t190;
  double t191;
  double t2;
  double t200;
  double t201;
  double t210;
  double t211;
  double t212;
  double t213;
  double t215;
  double t222;
  double t227;
  double t232;
  double t233;
  double t236;
  double t24;
  double t25;
  double t265;
  double t268;
  double t271;
  double t274;
  double t276;
  double t278;
  double t281;
  double t286;
  double t291;
  double t293;
  double t298;
  double t3;
  double t300;
  double t302;
  double t303;
  double t31;
  double t312;
  double t313;
  double t32;
  double t322;
  double t323;
  double t324;
  double t326;
  double t333;
  double t338;
  double t34;
  double t343;
  double t346;
  double t35;
  double t375;
  double t378;
  double t381;
  double t384;
  double t386;
  double t388;
  double t391;
  double t396;
  double t4;
  double t401;
  double t403;
  double t408;
  double t41;
  double t410;
  double t412;
  double t413;
  double t422;
  double t423;
  double t432;
  double t433;
  double t434;
  double t436;
  double t443;
  double t448;
  double t45;
  double t453;
  double t456;
  double t485;
  double t491;
  double t496;
  double t499;
  double t5;
  double t50;
  double t501;
  double t503;
  double t51;
  double t510;
  double t513;
  double t514;
  double t523;
  double t532;
  double t535;
  double t542;
  double t56;
  double t563;
  double t57;
  double t6;
  double t60;
  double t61;
  double t67;
  double t68;
  double t69;
  double t70;
  double t76;
  double t77;
  double t78;
  double t79;
  double t8;
  double t82;
  double t84;
  double t86;
  double t88;
  double t89;
  double t9;
  double t90;
  double t92;
  double t93;
  double t94;
  double t97;
  double t99;
  {
    t1 = v[0];
    t2 = t1*t1;
    t3 = v[1];
    t4 = t3*t3;
    t5 = v[2];
    t6 = t5*t5;
    t8 = sqrt(1.0-t2-t4-t6);
    t9 = qr0[1];
    t11 = qr0[0];
    t13 = qr0[3];
    t15 = qr0[2];
    t17 = t9*t8+t11*t1+t13*t3-t5*t15;
    t18 = M[0];
    t24 = t8*t15+t11*t3+t5*t9-t1*t13;
    t25 = M[1];
    t31 = t8*t13+t5*t11+t1*t15-t3*t9;
    t32 = M[2];
    t34 = -t17*t18-t24*t25-t31*t32;
    t35 = -t17;
    t41 = t8*t11-t1*t9-t3*t15-t5*t13;
    t45 = t41*t18+t24*t32-t31*t25;
    t50 = t41*t25+t31*t18-t32*t17;
    t51 = -t31;
    t56 = t41*t32+t17*t25-t24*t18;
    t57 = -t24;
    t60 = t34*t35+t41*t45+t50*t51-t56*t57+t[0];
    t61 = t60*t60;
    t67 = t34*t51+t41*t56+t45*t57-t50*t35+t[2];
    t68 = t67*t67;
    t69 = 1/t68;
    t70 = t61*t69;
    t76 = t34*t57+t41*t50+t56*t35-t45*t51+t[1];
    t77 = t76*t76;
    t78 = t77*t69;
    t79 = t70+t78;
    t82 = kc[4];
    t84 = kc[1]+t79*t82;
    t86 = kc[0]+t79*t84;
    t88 = 1.0+t79*t86;
    t89 = t88*t60;
    t90 = 1/t67;
    t92 = kc[2];
    t93 = t92*t60;
    t94 = t69*t76;
    t97 = kc[3];
    t99 = 3.0*t70+t78;
    jacmKDRT[0][0] = t89*t90+2.0*t93*t94+t97*t99;
    t101 = a[3];
    t102 = t88*t76;
    t105 = t70+3.0*t78;
    t107 = t97*t60;
    t110 = t102*t90+t92*t105+2.0*t107*t94;
    jacmKDRT[1][0] = t101*t110;
    jacmKDRT[0][1] = 1.0;
    jacmKDRT[1][1] = 0.0;
    jacmKDRT[0][2] = 0.0;
    jacmKDRT[1][2] = 1.0;
    jacmKDRT[0][3] = 0.0;
    t111 = a[0];
    jacmKDRT[1][3] = t110*t111;
    jacmKDRT[0][4] = t110;
    jacmKDRT[1][4] = 0.0;
    t113 = t60*t90;
    t115 = a[4];
    t117 = t76*t90;
    jacmKDRT[0][5] = t111*t79*t113+t115*t79*t117;
    t119 = t111*t101;
    t120 = t79*t76;
    jacmKDRT[1][5] = t119*t120*t90;
    t122 = t79*t79;
    jacmKDRT[0][6] = t111*t122*t113+t115*t122*t117;
    jacmKDRT[1][6] = t119*t122*t76*t90;
    jacmKDRT[0][7] = 2.0*t111*t60*t94+t115*t105;
    jacmKDRT[1][7] = t119*t105;
    jacmKDRT[0][8] = t111*t99+2.0*t115*t60*t94;
    t137 = t60*t69;
    jacmKDRT[1][8] = 2.0*t119*t137*t76;
    t140 = t122*t79;
    jacmKDRT[0][9] = t111*t140*t113+t115*t140*t117;
    jacmKDRT[1][9] = t119*t140*t76*t90;
    t147 = 1/t8;
    t148 = t147*t9;
    t150 = -t148*t1+t11;
    t152 = t147*t15;
    t154 = -t152*t1-t13;
    t156 = t13*t147;
    t158 = -t156*t1+t15;
    t160 = -t150*t18-t154*t25-t158*t32;
    t162 = -t150;
    t164 = t147*t11;
    t166 = -t164*t1-t9;
    t171 = t166*t18+t154*t32-t158*t25;
    t176 = t166*t25+t158*t18-t150*t32;
    t178 = -t158;
    t183 = t32*t166+t150*t25-t154*t18;
    t185 = -t154;
    t187 = t160*t35+t34*t162+t166*t45+t41*t171+t176*t51+t50*t178-t183*t57-t56*
t185;
    t188 = t137*t187;
    t190 = 1/t68/t67;
    t191 = t61*t190;
    t200 = t160*t51+t34*t178+t166*t56+t41*t183+t171*t57+t45*t185-t176*t35-t50*
t162;
    t201 = t191*t200;
    t210 = t160*t57+t34*t185+t166*t50+t41*t176+t183*t35+t56*t162-t171*t51-t45*
t178;
    t211 = t94*t210;
    t212 = t77*t190;
    t213 = t212*t200;
    t215 = 2.0*t188-2.0*t201+2.0*t211-2.0*t213;
    t222 = t215*t86+t79*(t215*t84+t79*t215*t82);
    t227 = t69*t200;
    t232 = t190*t76;
    t233 = t232*t200;
    t236 = t69*t210;
    t265 = t222*t76*t90+t88*t210*t90-t102*t227+t92*(2.0*t188-2.0*t201+6.0*t211
-6.0*t213)+2.0*t97*t187*t94-4.0*t107*t233+2.0*t107*t236;
    jacmKDRT[0][10] = t111*(t222*t60*t90+t88*t187*t90-t89*t227+2.0*t92*t187*t94
-4.0*t93*t233+2.0*t93*t236+t97*(6.0*t188-6.0*t201+2.0*t211-2.0*t213))+t115*t265
;
    jacmKDRT[1][10] = t119*t265;
    t268 = -t148*t3+t13;
    t271 = -t152*t3+t11;
    t274 = -t156*t3-t9;
    t276 = -t268*t18-t271*t25-t274*t32;
    t278 = -t268;
    t281 = -t164*t3-t15;
    t286 = t281*t18+t271*t32-t274*t25;
    t291 = t281*t25+t274*t18-t32*t268;
    t293 = -t274;
    t298 = t281*t32+t268*t25-t271*t18;
    t300 = -t271;
    t302 = t276*t35+t34*t278+t281*t45+t41*t286+t291*t51+t50*t293-t298*t57-t56*
t300;
    t303 = t137*t302;
    t312 = t276*t51+t34*t293+t281*t56+t41*t298+t286*t57+t45*t300-t291*t35-t50*
t278;
    t313 = t191*t312;
    t322 = t276*t57+t34*t300+t281*t50+t41*t291+t298*t35+t56*t278-t286*t51-t45*
t293;
    t323 = t94*t322;
    t324 = t212*t312;
    t326 = 2.0*t303-2.0*t313+2.0*t323-2.0*t324;
    t333 = t326*t86+t79*(t326*t84+t79*t326*t82);
    t338 = t69*t312;
    t343 = t312*t232;
    t346 = t69*t322;
    t375 = t333*t76*t90+t88*t322*t90-t338*t102+t92*(2.0*t303-2.0*t313+6.0*t323
-6.0*t324)+2.0*t97*t302*t94-4.0*t107*t343+2.0*t346*t107;
    jacmKDRT[0][11] = t111*(t333*t60*t90+t88*t302*t90-t89*t338+2.0*t92*t302*t94
-4.0*t93*t343+2.0*t93*t346+t97*(6.0*t303-6.0*t313+2.0*t323-2.0*t324))+t115*t375
;
    jacmKDRT[1][11] = t119*t375;
    t378 = -t148*t5-t15;
    t381 = -t152*t5+t9;
    t384 = -t156*t5+t11;
    t386 = -t378*t18-t25*t381-t384*t32;
    t388 = -t378;
    t391 = -t164*t5-t13;
    t396 = t391*t18+t381*t32-t384*t25;
    t401 = t25*t391+t384*t18-t378*t32;
    t403 = -t384;
    t408 = t391*t32+t378*t25-t381*t18;
    t410 = -t381;
    t412 = t35*t386+t388*t34+t391*t45+t41*t396+t401*t51+t50*t403-t408*t57-t56*
t410;
    t413 = t137*t412;
    t422 = t386*t51+t34*t403+t56*t391+t41*t408+t396*t57+t45*t410-t401*t35-t50*
t388;
    t423 = t191*t422;
    t432 = t386*t57+t34*t410+t391*t50+t41*t401+t408*t35+t56*t388-t396*t51-t45*
t403;
    t433 = t94*t432;
    t434 = t212*t422;
    t436 = 2.0*t413-2.0*t423+2.0*t433-2.0*t434;
    t443 = t436*t86+t79*(t436*t84+t79*t436*t82);
    t448 = t69*t422;
    t453 = t232*t422;
    t456 = t69*t432;
    t485 = t443*t76*t90+t88*t432*t90-t102*t448+t92*(2.0*t413-2.0*t423+6.0*t433
-6.0*t434)+2.0*t97*t412*t94-4.0*t107*t453+2.0*t107*t456;
    jacmKDRT[0][12] = t111*(t443*t60*t90+t88*t412*t90-t89*t448+2.0*t92*t412*t94
-4.0*t93*t453+2.0*t93*t456+t97*(6.0*t413-6.0*t423+2.0*t433-2.0*t434))+t115*t485
;
    jacmKDRT[1][12] = t119*t485;
    t491 = t69*t82;
    t496 = 2.0*t137*t86+t79*(2.0*t137*t84+2.0*t79*t60*t491);
    t499 = t88*t90;
    t501 = t92*t69*t76;
    t503 = t107*t69;
    t510 = 2.0*t93*t69;
    t513 = 2.0*t97*t69*t76;
    t514 = t496*t76*t90+t510+t513;
    jacmKDRT[0][13] = t111*(t496*t60*t90+t499+2.0*t501+6.0*t503)+t115*t514;
    jacmKDRT[1][13] = t119*t514;
    t523 = 2.0*t94*t86+t79*(2.0*t94*t84+2.0*t120*t491);
    t532 = t523*t76*t90+t499+6.0*t501+2.0*t503;
    jacmKDRT[0][14] = t111*(t523*t60*t90+t510+t513)+t115*t532;
    jacmKDRT[1][14] = t119*t532;
    t535 = -2.0*t191-2.0*t212;
    t542 = t535*t86+t79*(t535*t84+t79*t535*t82);
    t563 = t542*t76*t90-t102*t69+t92*(-2.0*t191-6.0*t212)-4.0*t107*t232;
    jacmKDRT[0][15] = t111*(t542*t60*t90-t89*t69-4.0*t93*t232+t97*(-6.0*t191
-2.0*t212))+t115*t563;
    jacmKDRT[1][15] = t119*t563;
    return;
  }
}

#include <math.h>
void calcDistImgProjJacS(double a[5],double kc[5],double qr0[4],double v[3],
double t[3],double M[3],double jacmS[2][3])
{
  double t1;
  double t10;
  double t100;
  double t101;
  double t102;
  double t104;
  double t108;
  double t110;
  double t112;
  double t114;
  double t12;
  double t121;
  double t123;
  double t126;
  double t129;
  double t130;
  double t132;
  double t136;
  double t137;
  double t138;
  double t14;
  double t141;
  double t144;
  double t153;
  double t158;
  double t16;
  double t169;
  double t174;
  double t177;
  double t18;
  double t180;
  double t181;
  double t182;
  double t185;
  double t186;
  double t187;
  double t189;
  double t19;
  double t190;
  double t191;
  double t192;
  double t194;
  double t2;
  double t201;
  double t206;
  double t211;
  double t214;
  double t243;
  double t247;
  double t248;
  double t25;
  double t250;
  double t251;
  double t254;
  double t255;
  double t256;
  double t258;
  double t26;
  double t265;
  double t270;
  double t275;
  double t278;
  double t3;
  double t307;
  double t32;
  double t33;
  double t35;
  double t36;
  double t4;
  double t42;
  double t46;
  double t5;
  double t51;
  double t52;
  double t57;
  double t58;
  double t6;
  double t61;
  double t67;
  double t68;
  double t69;
  double t7;
  double t70;
  double t71;
  double t72;
  double t74;
  double t75;
  double t76;
  double t77;
  double t79;
  double t80;
  double t81;
  double t82;
  double t85;
  double t86;
  double t9;
  double t92;
  double t93;
  double t94;
  double t97;
  double t98;
  double t99;
  {
    t1 = a[0];
    t2 = v[0];
    t3 = t2*t2;
    t4 = v[1];
    t5 = t4*t4;
    t6 = v[2];
    t7 = t6*t6;
    t9 = sqrt(1.0-t3-t5-t7);
    t10 = qr0[1];
    t12 = qr0[0];
    t14 = qr0[3];
    t16 = qr0[2];
    t18 = t9*t10+t12*t2+t4*t14-t6*t16;
    t19 = M[0];
    t25 = t9*t16+t12*t4+t10*t6-t2*t14;
    t26 = M[1];
    t32 = t9*t14+t12*t6+t2*t16-t4*t10;
    t33 = M[2];
    t35 = -t18*t19-t25*t26-t32*t33;
    t36 = -t18;
    t42 = t9*t12-t2*t10-t4*t16-t6*t14;
    t46 = t42*t19+t25*t33-t32*t26;
    t51 = t42*t26+t32*t19-t18*t33;
    t52 = -t32;
    t57 = t42*t33+t26*t18-t25*t19;
    t58 = -t25;
    t61 = t35*t36+t42*t46+t51*t52-t57*t58+t[0];
    t67 = t35*t52+t42*t57+t46*t58-t51*t36+t[2];
    t68 = t67*t67;
    t69 = 1/t68;
    t70 = t61*t69;
    t71 = t36*t36;
    t72 = t42*t42;
    t74 = t58*t58;
    t75 = t71+t72+t32*t52-t74;
    t76 = t70*t75;
    t77 = t61*t61;
    t79 = 1/t68/t67;
    t80 = t79*t77;
    t81 = t36*t52;
    t82 = t58*t42;
    t85 = t81+2.0*t82-t32*t36;
    t86 = t80*t85;
    t92 = t35*t58+t51*t42+t57*t36-t46*t52+t[1];
    t93 = t92*t69;
    t94 = t36*t58;
    t97 = t42*t52;
    t98 = 2.0*t94+t42*t32-t97;
    t99 = t93*t98;
    t100 = t92*t92;
    t101 = t100*t79;
    t102 = t101*t85;
    t104 = 2.0*t76-2.0*t86+2.0*t99-2.0*t102;
    t108 = t77*t69+t69*t100;
    t110 = kc[4];
    t112 = kc[1]+t108*t110;
    t114 = kc[0]+t112*t108;
    t121 = t104*t114+t108*(t112*t104+t108*t104*t110);
    t123 = 1/t67;
    t126 = 1.0+t108*t114;
    t129 = t126*t61;
    t130 = t69*t85;
    t132 = kc[2];
    t136 = t132*t61;
    t137 = t79*t92;
    t138 = t137*t85;
    t141 = t69*t98;
    t144 = kc[3];
    t153 = a[4];
    t158 = t126*t92;
    t169 = t144*t61;
    t174 = t121*t92*t123+t126*t98*t123-t158*t130+t132*(2.0*t76-2.0*t86+6.0*t99
-6.0*t102)+2.0*t144*t75*t93-4.0*t138*t169+2.0*t141*t169;
    jacmS[0][0] = t1*(t121*t61*t123+t126*t75*t123-t129*t130+2.0*t132*t75*t93
-4.0*t136*t138+2.0*t136*t141+t144*(6.0*t76-6.0*t86+2.0*t99-2.0*t102))+t153*t174
;
    t177 = t1*a[3];
    jacmS[1][0] = t177*t174;
    t180 = t94+2.0*t97-t58*t18;
    t181 = t70*t180;
    t182 = t52*t58;
    t185 = t36*t42;
    t186 = 2.0*t182+t42*t18-t185;
    t187 = t80*t186;
    t189 = t52*t52;
    t190 = t74+t72+t36*t18-t189;
    t191 = t93*t190;
    t192 = t101*t186;
    t194 = 2.0*t181-2.0*t187+2.0*t191-2.0*t192;
    t201 = t114*t194+t108*(t112*t194+t108*t194*t110);
    t206 = t69*t186;
    t211 = t137*t186;
    t214 = t69*t190;
    t243 = t201*t92*t123+t126*t190*t123-t158*t206+t132*(2.0*t181-2.0*t187+6.0*
t191-6.0*t192)+2.0*t144*t180*t93-4.0*t211*t169+2.0*t169*t214;
    jacmS[0][1] = t1*(t201*t61*t123+t126*t180*t123-t129*t206+2.0*t132*t180*t93
-4.0*t136*t211+2.0*t136*t214+t144*(6.0*t181-6.0*t187+2.0*t191-2.0*t192))+t153*
t243;
    jacmS[1][1] = t177*t243;
    t247 = 2.0*t81+t42*t25-t82;
    t248 = t70*t247;
    t250 = t189+t72+t25*t58-t71;
    t251 = t80*t250;
    t254 = t182+2.0*t185-t25*t52;
    t255 = t93*t254;
    t256 = t101*t250;
    t258 = 2.0*t248-2.0*t251+2.0*t255-2.0*t256;
    t265 = t258*t114+t108*(t258*t112+t108*t258*t110);
    t270 = t250*t69;
    t275 = t137*t250;
    t278 = t254*t69;
    t307 = t265*t92*t123+t126*t254*t123-t270*t158+t132*(2.0*t248-2.0*t251+6.0*
t255-6.0*t256)+2.0*t144*t247*t93-4.0*t169*t275+2.0*t169*t278;
    jacmS[0][2] = t1*(t265*t61*t123+t126*t247*t123-t129*t270+2.0*t132*t247*t93
-4.0*t136*t275+2.0*t136*t278+t144*(6.0*t248-6.0*t251+2.0*t255-2.0*t256))+t153*
t307;
    jacmS[1][2] = t307*t177;
    return;
  }
}

