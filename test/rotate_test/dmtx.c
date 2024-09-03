#include "callback.h"

#undef CALLBACK_POINT_PLOT
#define CALLBACK_POINT_PLOT(a, b, c, d) PlotPointCallback(a, b, c, d)

#undef CALLBACK_POINT_XFRM
#define CALLBACK_POINT_XFRM(a, b, c, d) XfrmPlotPointCallback(a, b, c, d)

#undef CALLBACK_MODULE
#define CALLBACK_MODULE(a, b, c, d, e) PlotModuleCallback(a, b, c, d, e)

#undef CALLBACK_MATRIX
#define CALLBACK_MATRIX(a) BuildMatrixCallback2(a)

#undef CALLBACK_FINAL
#define CALLBACK_FINAL(a, b) FinalCallback(a, b)

#include "../../dmtx.c"
