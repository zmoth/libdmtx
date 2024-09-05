#include "dmtx.h"
#include "dmtxstatic.h"

#ifdef DEBUG_CALLBACK

void dmtxCallbackBuildMatrixRegion(DmtxCallbackBuildMatrixRegion cb)
{
    cbBuildMatrixRegion = cb;
}

void dmtxCallbackBuildMatrix(DmtxCallbackBuildMatrix cb)
{
    cbBuildMatrix = cb;
}

void dmtxCallbackPlotPoint(DmtxCallbackPlotPoint cb)
{
    cbPlotPoint = cb;
}

void dmtxCallbackXfrmPlotPoint(DmtxCallbackXfrmPlotPoint cb)
{
    cbXfrmPlotPoint = cb;
}

void dmtxCallbackFinal(DmtxCallbackFinal cb)
{
    cbFinal = cb;
}

#endif