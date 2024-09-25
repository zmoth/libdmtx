#include "dmtx.h"
#include "dmtxstatic.h"

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

extern void dmtxCallbackPlotModule(DmtxCallbackPlotModule cb)
{
    cbPlotModule = cb;
}

void dmtxCallbackFinal(DmtxCallbackFinal cb)
{
    cbFinal = cb;
}
