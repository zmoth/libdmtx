/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2008, 2009 Mike Laughton. All rights reserved.
 * Copyright 2012-2016 Vadim A. Misbakh-Soloviov. All rights reserved.
 *
 * See LICENSE file in the main project directory for full
 * terms of use and distribution.
 *
 * Contact:
 * Vadim A. Misbakh-Soloviov <dmtx@mva.name>
 * Mike Laughton <mike@dragonflylogic.com>
 *
 * \file dmtxvector2.c
 * \brief 二维向量数学运算
 */

#include <assert.h>
#include <math.h>

#include "dmtx.h"
#include "dmtxstatic.h"

/**
 * \brief 向量相加
 */
extern DmtxVector2 *dmtxVector2AddTo(INOUT DmtxVector2 *v1, const DmtxVector2 *v2)
{
    v1->x += v2->x;
    v1->y += v2->y;

    return v1;
}

/**
 * \brief 向量相加
 */
extern DmtxVector2 *dmtxVector2Add(OUT DmtxVector2 *vOut, const DmtxVector2 *v1, const DmtxVector2 *v2)
{
    *vOut = *v1;

    return dmtxVector2AddTo(vOut, v2);
}

/**
 * \brief 向量相减
 */
extern DmtxVector2 *dmtxVector2SubFrom(INOUT DmtxVector2 *v1, const DmtxVector2 *v2)
{
    v1->x -= v2->x;
    v1->y -= v2->y;

    return v1;
}

/**
 * \brief 向量相减
 */
extern DmtxVector2 *dmtxVector2Sub(OUT DmtxVector2 *vOut, const DmtxVector2 *v1, const DmtxVector2 *v2)
{
    *vOut = *v1;

    return dmtxVector2SubFrom(vOut, v2);
}

/**
 * \brief 向量数乘
 */
extern DmtxVector2 *dmtxVector2ScaleBy(INOUT DmtxVector2 *v, double s)
{
    v->x *= s;
    v->y *= s;

    return v;
}

/**
 * \brief 向量数乘
 */
extern DmtxVector2 *dmtxVector2Scale(OUT DmtxVector2 *vOut, const DmtxVector2 *v, double s)
{
    *vOut = *v;

    return dmtxVector2ScaleBy(vOut, s);
}

/**
 * \brief 二维向量叉积
 */
extern double dmtxVector2Cross(const DmtxVector2 *v1, const DmtxVector2 *v2)
{
    return (v1->x * v2->y) - (v1->y * v2->x);
}

/**
 * \brief 归一化
 */
extern double dmtxVector2Norm(INOUT DmtxVector2 *v)
{
    double mag;

    mag = dmtxVector2Mag(v);

    if (mag <= DmtxAlmostZero) {
        return -1.0; /* XXX this doesn't look clean */
    }

    dmtxVector2ScaleBy(v, 1 / mag);

    return mag;
}

/**
 * \brief 二维向量点积
 */
extern double dmtxVector2Dot(const DmtxVector2 *v1, const DmtxVector2 *v2)
{
    return (v1->x * v2->x) + (v1->y * v2->y);
}

/**
 * \brief 二维向量的模
 */
extern double dmtxVector2Mag(const DmtxVector2 *v)
{
    return sqrt(v->x * v->x + v->y * v->y);
}

/**
 * \brief 计算点到直线的垂直距离
 */
extern double dmtxDistanceFromRay2(const DmtxRay2 *r, const DmtxVector2 *q)
{
    DmtxVector2 vSubTmp;

    double mag = dmtxVector2Mag(&(r->v));
    DmtxAssert(fabs(mag) > DmtxAlmostZero);

    return dmtxVector2Cross(&(r->v), dmtxVector2Sub(&vSubTmp, q, &(r->p))) / mag;
}

/**
 *
 *
 */
extern double dmtxDistanceAlongRay2(const DmtxRay2 *r, const DmtxVector2 *q)
{
    DmtxVector2 vSubTmp;

#ifdef DEBUG
    /* Assumes that v is a unit vector */
    if (fabs(1.0 - dmtxVector2Mag(&(r->v))) > DmtxAlmostZero) {
        ; /* XXX big error goes here */
    }
#endif

    return dmtxVector2Dot(dmtxVector2Sub(&vSubTmp, q, &(r->p)), &(r->v));
}

/**
 * \brief 判断两条直线是否相交，并计算交点。
 *
 * 用向量叉乘求直线交点 https://www.cnblogs.com/zhb2000/p/vector-cross-product-solve-intersection.html
 *
 * \param point 交点坐标（如果存在）
 * \param p0 第一条直线
 * \param p1 第二条直线
 *
 * \return 返回DmtxPass表示射线相交，并已计算出交点；返回DmtxFail表示射线不相交或几乎平行。
 */
extern DmtxPassFail dmtxRay2Intersect(OUT DmtxVector2 *point, const DmtxRay2 *p0, const DmtxRay2 *p1)
{
    double numer, denom;
    DmtxVector2 w;

    denom = dmtxVector2Cross(&(p1->v), &(p0->v));
    if (fabs(denom) <= DmtxAlmostZero) {
        return DmtxFail;
    }

    dmtxVector2Sub(&w, &(p1->p), &(p0->p));
    numer = dmtxVector2Cross(&(p1->v), &w);

    return dmtxPointAlongRay2(point, p0, numer / denom);
}

/**
 * \brief 计算直线上特定位置的点
 *
 * I = p + t·v
 *
 * \param[out] point 点I的坐标。
 * \param[in] r 直线
 * \param[in] t 表示沿射线方向的位置参数（从射线起点开始测量的距离）。
 */
extern DmtxPassFail dmtxPointAlongRay2(OUT DmtxVector2 *point, const DmtxRay2 *r, double t)
{
    DmtxVector2 vTmp;

    /* Ray should always have unit length of 1 */
    DmtxAssert(fabs(1.0 - dmtxVector2Mag(&(r->v))) <= DmtxAlmostZero);  // XXX: 是必须的吗？为什么不用dmtxVector2Norm

    dmtxVector2Scale(&vTmp, &(r->v), t);
    dmtxVector2Add(point, &(r->p), &vTmp);

    return DmtxPass;
}
