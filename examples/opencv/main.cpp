#include <assert.h>
#include <dmtx.h>
#include <math.h>
#include <stdint.h>

#include <opencv2/opencv.hpp>

#define DMTX_DISPLAY_SQUARE 1
#define DMTX_DISPLAY_POINT 2
#define DMTX_DISPLAY_CIRCLE 3

cv::Mat image, img, show, pointShow;
DmtxImage *gImage = NULL;

cv::Scalar hsv2rgb(float h, float s = 1.0F, float v = 1.0F)
{
    double c, h_, x;
    double r, g, b;

    h = fmod(h, 360.0F);

    c = s;
    h_ = h / 60;
    x = c * (1 - abs(fmod(h_, 2) - 1));
    r = g = b = v - c;

    if (h_ < 1) {
        r += c;
        g += x;
    } else if (h_ < 2) {
        r += x;
        g += c;
    } else if (h_ < 3) {
        g += c;
        b += x;
    } else if (h_ < 4) {
        g += x;
        b += c;
    } else if (h_ < 5) {
        r += x;
        b += c;
    } else if (h_ < 6) {
        r += c;
        b += x;
    }

    return CV_RGB(r * 255, g * 255, b * 255);
}

void buildMatrixRegion(DmtxRegion *reg)
{
    cv::Mat s = img.clone();
    cv::rectangle(s, cv::Point(reg->boundMin.x, gImage->height - reg->boundMin.y),
                  cv::Point(reg->boundMax.x, gImage->height - reg->boundMax.y), CV_RGB(0, 0, 255));

    cv::line(s, cv::Point(reg->leftLine.locPos.x, gImage->height - reg->leftLine.locPos.y),
             cv::Point(reg->leftLine.locNeg.x, gImage->height - reg->leftLine.locNeg.y), CV_RGB(255, 0, 0));
    cv::line(s, cv::Point(reg->bottomLine.locPos.x, gImage->height - reg->bottomLine.locPos.y),
             cv::Point(reg->bottomLine.locNeg.x, gImage->height - reg->bottomLine.locNeg.y), CV_RGB(0, 255, 0));

    pointShow = img.clone();

    cv::namedWindow("buildMatrixRegion", cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO);
    cv::imshow("buildMatrixRegion", s);
    cv::waitKey();
}

void plotPoint(DmtxPixelLoc loc, float colorHue, int paneNbr, int dispType)
{
    cv::Scalar color;
    DmtxVector2 point;
    std::string winname;

    point.x = loc.x;
    point.y = loc.y;

    switch (paneNbr) {
        case 1:
            winname = "plotPoint1";
            break;
        case 2:
            winname = "plotPoint2";
            break;
        case 3:
            winname = "plotPoint3";
            break;
        case 4:
            winname = "plotPoint4";
            break;
        case 5:
            winname = "plotPoint5";
            break;
        case 6:
            winname = "plotPoint6";
            break;
        default:
            winname = "showPlotPoint";
    }

    color = hsv2rgb(colorHue);

    if (dispType == DMTX_DISPLAY_SQUARE) {
        if (abs(colorHue) < 0.0001F) {
            pointShow = img.clone();
        }
        cv::circle(pointShow, cv::Point((int)point.x, gImage->height - (int)point.y), 3, color, 2);
    } else if (dispType == DMTX_DISPLAY_POINT) {
        int x = (int)(point.x + 0.5);
        int y = (int)(gImage->height - point.y + 0.5);
        cv::circle(pointShow, cv::Point(x, y), 1, color, -1);
    }

    cv::namedWindow(winname, cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO);
    cv::imshow(winname, pointShow);
    cv::waitKey(1);
}

// 计算交点
cv::Point findIntersection(cv::Point p1, double theta1, cv::Point p2, double theta2)
{
    // 将角度转换为弧度
    double rad1 = theta1 * M_PI / 180.0;
    double rad2 = theta2 * M_PI / 180.0;

    // 处理特殊情况：垂直直线
    bool isVertical1 = std::abs(rad1) == M_PI / 2;
    bool isVertical2 = std::abs(rad2) == M_PI / 2;

    // 处理垂直直线的情况
    if (isVertical1 && isVertical2) {
        throw std::invalid_argument("Both lines are vertical, no unique intersection.");
    } else if (isVertical1) {
        // 第一条直线垂直，第二条直线非垂直
        double x = p1.x;
        double m2 = std::tan(rad2);
        double b2 = p2.y - m2 * p2.x;
        double y = m2 * x + b2;
        return cv::Point((int)x, (int)y);
    } else if (isVertical2) {
        // 第二条直线垂直，第一条直线非垂直
        double x = p2.x;
        double m1 = std::tan(rad1);
        double b1 = p1.y - m1 * p1.x;
        double y = m1 * x + b1;
        return cv::Point((int)x, (int)y);
    }

    // 计算斜率
    double m1 = std::tan(rad1);
    double m2 = std::tan(rad2);

    // 如果斜率相同，则两条直线平行或重合，无法计算交点
    if (m1 == m2) {
        throw std::invalid_argument("Lines are parallel or coincident.");
    }

    // 求解直线方程中的常数项
    double b1 = p1.y - m1 * p1.x;
    double b2 = p2.y - m2 * p2.x;

    // 求解 x
    double x = (b2 - b1) / (m1 - m2);

    // 求解 y
    double y = m1 * x + b1;

    return cv::Point((int)x, (int)y);
}

int main(int argc, char *argv[])
{
    dmtxCallbackBuildMatrixRegion(buildMatrixRegion);
    dmtxCallbackPlotPoint(plotPoint);

    DmtxDecode *dec;
    DmtxRegion *reg;
    DmtxMessage *msg;

    // image = cv::imread("images/test_image01.bmp");
    // image = cv::imread("images/test_image02.bmp");
    // image = cv::imread("images/test_image03.bmp");
    // image = cv::imread("images/test_image04.bmp");
    // image = cv::imread("images/test_image05.bmp");
    // image = cv::imread("images/test_image06.bmp");
    // image = cv::imread("images/test_image07.bmp");
    // image = cv::imread("images/test_image08.bmp");
    // image = cv::imread("images/test_image09.bmp");
    // image = cv::imread("images/test_image10.bmp");
    // image = cv::imread("images/test_image11.bmp");
    // image = cv::imread("images/test_image12.bmp");
    // image = cv::imread("images/test_image13.bmp");
    // image = cv::imread("images/test_image14.bmp");
    // image = cv::imread("images/test_image15.bmp");
    image = cv::imread("images/test_image16.bmp");
    // image = cv::imread("images/test_image17.bmp");
    // image = cv::imread("images/test_image18.bmp");

    img = image.clone();
    pointShow = img.clone();
    show = img.clone();

    int pack = DmtxPack32bppXRGB;
    auto f = image.type();
    if (f == CV_32FC3) {
        pack = DmtxPack32bppXRGB;
    } else if (f == CV_8UC3) {
        pack = DmtxPack24bppRGB;
    } else if (f == CV_8UC1) {
        pack = DmtxPack8bppK;
    } else {
        return {};
    }

    gImage = dmtxImageCreate(image.data, image.cols, image.rows, pack);
    assert(gImage != NULL);

    /* Pixels from glReadPixels are Y-flipped according to libdmtx */
    // dmtxImageSetProp(gImage, DmtxPropImageFlip, DmtxFlipY);

    /* Start fresh scan */
    dec = dmtxDecodeCreate(gImage, 1);
    assert(dec != NULL);

    dmtxDecodeSetProp(dec, DmtxPropSymbolSize, DmtxSymbolShapeAuto);
    dmtxDecodeSetProp(dec, DmtxPropSquareDevn, 50);
    dmtxDecodeSetProp(dec, DmtxPropEdgeThresh, 10);
    // dmtxDecodeSetProp(dec, DmtxPropEdgeMin, 30);
    // dmtxDecodeSetProp(dec, DmtxPropEdgeMax, 100);

    for (;;) {
        reg = dmtxRegionFindNext(dec, NULL);
        if (reg == NULL) {
            break;
        }

        msg = dmtxDecodeMatrixRegion(dec, reg, DmtxUndefined);
        if (msg != NULL) {
            std::cout << msg->outputIdx << " - " << msg->output << std::endl;

            cv::circle(show, cv::Point(reg->leftLoc.x, gImage->height - reg->leftLoc.y), 2, CV_RGB(255, 0, 0), -1);
            cv::circle(show, cv::Point(reg->bottomLoc.x, gImage->height - reg->bottomLoc.y), 2, CV_RGB(0, 255, 0), -1);

            cv::circle(show, cv::Point(reg->topLoc.x, gImage->height - reg->topLoc.y), 2, CV_RGB(255, 255, 0), -1);
            cv::circle(show, cv::Point(reg->rightLoc.x, gImage->height - reg->rightLoc.y), 2, CV_RGB(0, 255, 255), -1);

            cv::Point bl, br, tl, tr;
            bl = findIntersection(cv::Point(reg->leftLoc.x, gImage->height - reg->leftLoc.y), 180 - reg->leftAngle,
                                  cv::Point(reg->bottomLoc.x, gImage->height - reg->bottomLoc.y),
                                  180 - reg->bottomAngle);

            br = findIntersection(cv::Point(reg->rightLoc.x, gImage->height - reg->rightLoc.y), 180 - reg->rightAngle,
                                  cv::Point(reg->bottomLoc.x, gImage->height - reg->bottomLoc.y),
                                  180 - reg->bottomAngle);

            tl = findIntersection(cv::Point(reg->leftLoc.x, gImage->height - reg->leftLoc.y), 180 - reg->leftAngle,
                                  cv::Point(reg->topLoc.x, gImage->height - reg->topLoc.y), 180 - reg->topAngle);

            tr = findIntersection(cv::Point(reg->rightLoc.x, gImage->height - reg->rightLoc.y), 180 - reg->rightAngle,
                                  cv::Point(reg->topLoc.x, gImage->height - reg->topLoc.y), 180 - reg->topAngle);

            cv::line(show, bl, br, CV_RGB(0, 255, 0));
            cv::line(show, br, tr, CV_RGB(0, 255, 255));
            cv::line(show, tr, tl, CV_RGB(0, 255, 255));
            cv::line(show, tl, bl, CV_RGB(255, 0, 0));

            dmtxMessageDestroy(&msg);

            break;
        }
        dmtxRegionDestroy(&reg);
    }

    dmtxDecodeDestroy(&dec);
    dmtxImageDestroy(&gImage);

    // cv::namedWindow("pointShow", cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO);
    // cv::imshow("pointShow", pointShow);
    // cv::waitKey();

    cv::namedWindow("show", cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO);
    cv::imshow("show", show);
    cv::waitKey();
}
