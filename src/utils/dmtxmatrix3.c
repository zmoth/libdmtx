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
 * @file dmtxmatrix3.c
 * @brief 二维矩阵(3x3)数学运算
 */

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "dmtx.h"
#include "dmtxstatic.h"

/**
 * @brief 复制3x3矩阵
 *
 * 该函数将一个 3x3 矩阵的内容复制到另一个矩阵。
 *
 * @param[out] m0 指向目标矩阵的指针，用于存储复制的结果
 * @param[in] m1 指向源矩阵的指针，从中读取数据进行复制
 */
extern void dmtxMatrix3Copy(OUT DmtxMatrix3 m0, DmtxMatrix3 m1)
{
    memcpy(m0, m1, sizeof(DmtxMatrix3));
}

/**
 * @brief 生成单位变换矩阵
 *
 *      | 1  0  0 |
 *  m = | 0  1  0 |
 *      | 0  0  1 |
 *
 *                  Transform "m"
 *            (doesn't change anything)
 *                       |\
 *  (0,1)  x----o     +--+ \    (0,1)  x----o
 *         |    |     |     \          |    |
 *         |    |     |     /          |    |
 *         +----*     +--+ /           +----*
 *  (0,0)     (1,0)      |/     (0,0)     (1,0)
 *
 */
extern void dmtxMatrix3Identity(OUT DmtxMatrix3 m)
{
    static DmtxMatrix3 tmp = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    dmtxMatrix3Copy(m, tmp);
}

/**
 * @brief 生成平移变换矩阵
 * @param[out] m 生成的矩阵
 * @param[in] tx 沿 x 轴的平移量
 * @param[in] ty 沿 y 轴的平移量
 *
 *      | 1  0  0 |
 *  m = | 0  1  0 |
 *      | tx ty 1 |
 *
 *                  Transform "m"
 *                      _____    (tx,1+ty)  x----o  (1+tx,1+ty)
 *                      \   |               |    |
 *  (0,1)  x----o       /   |      (0,1)  +-|--+ |
 *         |    |      /  /\|             | +----*  (1+tx,ty)
 *         |    |      \ /                |    |
 *         +----*       `                 +----+
 *  (0,0)     (1,0)                (0,0)     (1,0)
 *
 */
void dmtxMatrix3Translate(OUT DmtxMatrix3 m, double tx, double ty)
{
    dmtxMatrix3Identity(m);
    m[2][0] = tx;
    m[2][1] = ty;
}

/**
 * @brief 生成旋转变换矩阵
 * @param[out] m 生成的矩阵
 * @param[in] angle 旋转的角度
 *
 *     |  cos(a)  sin(a)  0 |
 * m = | -sin(a)  cos(a)  0 |
 *     |  0       0       1 |
 *                                       o
 *                  Transform "m"      /   `
 *                       ___         /       `
 *  (0,1)  x----o      |/   \       x          *  (cos(a),sin(a))
 *         |    |      '--   |       `        /
 *         |    |        ___/          `    /  a
 *         +----*                        `+  - - - - - -
 *  (0,0)     (1,0)                     (0,0)
 *
 */
extern void dmtxMatrix3Rotate(OUT DmtxMatrix3 m, double angle)
{
    double sinAngle, cosAngle;

    sinAngle = sin(angle);
    cosAngle = cos(angle);

    dmtxMatrix3Identity(m);
    m[0][0] = cosAngle;
    m[0][1] = sinAngle;
    m[1][0] = -sinAngle;
    m[1][1] = cosAngle;
}

/**
 * @brief 生成缩放变换矩阵
 * @param[out] m 生成的矩阵
 * @param[in] sx x轴的缩放因子
 * @param[in] sy y轴的缩放因子
 *
 *     | sx 0  0 |
 * m = | 0  sy 0 |
 *     | 0  0  1 |
 *
 *                  Transform "m"
 *                      _____     (0,sy)  x-------o (sx,sy)
 *                      \   |             |       |
 *  (0,1)  x----o       /   |      (0,1)  +----+  |
 *         |    |      /  /\|             |    |  |
 *         |    |      \ /                |    |  |
 *         +----*       `                 +----+--*
 *  (0,0)     (1,0)                (0,0)            (sx,0)
 *
 */
extern void dmtxMatrix3Scale(OUT DmtxMatrix3 m, double sx, double sy)
{
    dmtxMatrix3Identity(m);
    m[0][0] = sx;
    m[1][1] = sy;
}

/**
 * @brief 生成剪切变换矩阵
 *
 * 剪切变换(shear transformation)是空间线性变换之一，是仿射变换的一种原始变换。
 * 它指的是类似于四边形不稳定性那种性质，方形变平行四边形，任意一边都可以被拉长的过程。
 *
 * XXX: 这里的X和Y方向是不是弄反了
 *
 * @param[out] m 生成的矩阵
 * @param[in] shx x轴方向的剪切因子
 * @param[in] shy y轴方向的剪切因子
 *
 *     | 0    shy  0 |
 * m = | shx  0    0 |
 *     | 0    0    1 |
 */
extern void dmtxMatrix3Shear(OUT DmtxMatrix3 m, double shx, double shy)
{
    dmtxMatrix3Identity(m);
    m[1][0] = shx;
    m[0][1] = shy;
}

/**
 * @brief 生成顶部线倾斜变换矩阵
 *
 * 该函数用于创建一个变换矩阵，该矩阵用于对图像进行顶部线的倾斜变换。
 * 这种变换通常用于校正图像中的透视失真。
 *
 * @param[out] m 生成的矩阵
 * @param[in] b0 基线起点的 y 坐标
 * @param[in] b1 基线终点的 y 坐标
 * @param[in] sz 变换后的高度
 *
 *     | b1/b0    0    (b1-b0)/(sz*b0) |
 * m = |   0    sz/b0         0        |
 *     |   0      0           1        |
 *
 *     (sz,b1)  o
 *             /|    Transform "m"
 *            / |
 *           /  |        +--+
 *          /   |        |  |
 * (0,b0)  x    |        |  |
 *         |    |      +-+  +-+
 * (0,sz)  +----+       \    /    (0,sz)  x----o
 *         |    |        \  /             |    |
 *         |    |         \/              |    |
 *         +----+                         +----+
 *  (0,0)    (sz,0)                (0,0)    (sz,0)
 *
 */
extern void dmtxMatrix3LineSkewTop(OUT DmtxMatrix3 m, double b0, double b1, double sz)
{
    DmtxAssert(b0 >= DmtxAlmostZero);

    dmtxMatrix3Identity(m);
    m[0][0] = b1 / b0;
    m[1][1] = sz / b0;
    m[0][2] = (b1 - b0) / (sz * b0);
}

/**
 * @brief Generate top line skew transformation (inverse)
 * @param[out] m
 * @param[in] b0
 * @param[in] b1
 * @param[in] sz
 */
extern void dmtxMatrix3LineSkewTopInv(OUT DmtxMatrix3 m, double b0, double b1, double sz)
{
    DmtxAssert(b1 >= DmtxAlmostZero);

    dmtxMatrix3Identity(m);
    m[0][0] = b0 / b1;
    m[1][1] = b0 / sz;
    m[0][2] = (b0 - b1) / (sz * b1);
}

/**
 * @brief Generate side line skew transformation
 * @param[out] m
 * @param[in] b0
 * @param[in] b1
 * @param[in] sz
 */
extern void dmtxMatrix3LineSkewSide(OUT DmtxMatrix3 m, double b0, double b1, double sz)
{
    DmtxAssert(b0 >= DmtxAlmostZero);

    dmtxMatrix3Identity(m);
    m[0][0] = sz / b0;
    m[1][1] = b1 / b0;
    m[1][2] = (b1 - b0) / (sz * b0);
}

/**
 * @brief Generate side line skew transformation (inverse)
 * @param[out] m
 * @param[in] b0
 * @param[in] b1
 * @param[in] sz
 */
extern void dmtxMatrix3LineSkewSideInv(OUT DmtxMatrix3 m, double b0, double b1, double sz)
{
    DmtxAssert(b1 >= DmtxAlmostZero);

    dmtxMatrix3Identity(m);
    m[0][0] = b0 / sz;
    m[1][1] = b0 / b1;
    m[1][2] = (b0 - b1) / (sz * b1);
}

/**
 * @brief 矩阵相乘
 *
 * 该函数用于将两个 3x3 矩阵相乘，并将结果存储在输出矩阵中。
 *
 * @param[out] mOut 指向输出矩阵的指针，用于存储乘法结果
 * @param[in] m0 指向第一个输入矩阵的指针
 * @param[in] m1 指向第二个输入矩阵的指针
 */
extern void dmtxMatrix3Multiply(OUT DmtxMatrix3 mOut, DmtxMatrix3 m0, DmtxMatrix3 m1)
{
    int i, j, k;
    double val;

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            val = 0.0;
            for (k = 0; k < 3; k++) {
                val += m0[i][k] * m1[k][j];
            }
            mOut[i][j] = val;
        }
    }
}

/**
 * @brief 矩阵相乘
 *
 * 该函数用于将两个 3x3 矩阵相乘，并将结果存储在第一个矩阵中。
 *
 * @param[in,out] m0 指向第一个输入矩阵的指针，其内容将被修改为乘法的结果
 * @param[in] m1 指向第二个输入矩阵的指针
 */
extern void dmtxMatrix3MultiplyBy(INOUT DmtxMatrix3 m0, DmtxMatrix3 m1)
{
    DmtxMatrix3 mTmp;

    dmtxMatrix3Copy(mTmp, m0);
    dmtxMatrix3Multiply(m0, mTmp, m1);
}

/**
 * @brief 将向量与矩阵相乘
 *
 * 该函数用于将一个 3x3 矩阵与一个 2D 向量相乘，并将结果存储在输出向量中。
 *
 * @param[out] vOut 指向输出向量的指针，用于存储乘法结果
 * @param[in] vIn 指向输入向量的指针
 * @param[in] m 要与向量相乘的 3x3 矩阵（仿射变换矩阵）
 * @return DmtxPass | DmtxFail
 */
extern DmtxPassFail dmtxMatrix3VMultiply(OUT DmtxVector2 *vOut, DmtxVector2 *vIn, DmtxMatrix3 m)
{
    double w;

    w = vIn->X * m[0][2] + vIn->Y * m[1][2] + m[2][2];
    if (fabs(w) <= DmtxAlmostZero) {
        vOut->X = FLT_MAX;
        vOut->Y = FLT_MAX;
        return DmtxFail;
    }

    vOut->X = (vIn->X * m[0][0] + vIn->Y * m[1][0] + m[2][0]) / w;
    vOut->Y = (vIn->X * m[0][1] + vIn->Y * m[1][1] + m[2][1]) / w;

    return DmtxPass;
}

/**
 * @brief 将向量与矩阵相乘
 *
 * 此函数将输入向量`v`与给定矩阵`m`相乘，并直接更新输入向量`v`为乘法的结果。
 *
 * @param[in,out] v 输入向量同时也是输出向量，乘法操作后存储结果。
 * @param[in] m 用于乘法运算的3x3矩阵。
 * @return DmtxPass | DmtxFail
 */
extern DmtxPassFail dmtxMatrix3VMultiplyBy(INOUT DmtxVector2 *v, DmtxMatrix3 m)
{
    DmtxPassFail success;
    DmtxVector2 vOut;

    success = dmtxMatrix3VMultiply(&vOut, v, m);
    *v = vOut;

    return success;
}

/**
 * @brief Print matrix contents to STDOUT
 * @param[in] m
 */
extern void dmtxMatrix3Print(DmtxMatrix3 m)
{
    dmtxLogInfo("%8.8f\t%8.8f\t%8.8f\n", m[0][0], m[0][1], m[0][2]);
    dmtxLogInfo("%8.8f\t%8.8f\t%8.8f\n", m[1][0], m[1][1], m[1][2]);
    dmtxLogInfo("%8.8f\t%8.8f\t%8.8f\n", m[2][0], m[2][1], m[2][2]);
    dmtxLogInfo("\n");
}
