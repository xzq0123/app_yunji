/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#pragma once
#include <string.h>
#include <mutex>
#include <map>
#include <vector>
#include "AXFrame.hpp"
#include "AXThread.hpp"
#include "ax_vo_api.h"
#include "ax_ivps_api.h"
#include "haltype.hpp"
#include "IObserver.h"
#include "detectResult.h"

#define MAX_VO_CHN_NUM (AX_VO_CHN_MAX)

typedef struct VO_CHN_INFO_S {
    AX_U32 nCount;                            /* number of valid arrChns, [1 - MAX_VO_CHN_NUM]  */
    AX_F32 arrFps[MAX_VO_CHN_NUM];            /* if PTS of 1st frame is 0, VO is in charge of calculate PTS according fps */
    AX_BOOL arrHidden[MAX_VO_CHN_NUM];        /* if PTS of 1st frame is 0, VO is in charge of calculate PTS according fps */
    AX_VO_CHN_ATTR_T arrChns[MAX_VO_CHN_NUM]; /* rectage: x y w h shoule be even alignment, width stride should align to 16 */
    AX_VO_RECT_T arrCropRect[MAX_VO_CHN_NUM];
    VO_CHN_INFO_S(AX_VOID) {
        memset(this, 0, sizeof(*this));
        for (AX_U32 i = 0; i < MAX_VO_CHN_NUM; ++i) {
            arrChns[i].bKeepPrevFr = AX_TRUE;
            memset(&arrCropRect[i], 0, sizeof(AX_VO_RECT_T));
        }
    }
} VO_CHN_INFO_T;

typedef struct VO_ATTR_S {
    VO_DEV dev;            /* [IN ] 0: HDMI0  1: HDMI1 */
    VO_LAYER voLayer;      /* [OUT] layer is generated by VO */
    GRAPHIC_LAYER uiLayer; /* [IN ] if invalid uiLayer value (AX650: only 1 graphic layer valued 0), no graphic layer is binded to device */
    AX_S32 s32FBIndex[2];  /* [IN ] 0: QT 1: Overlay */
    AX_VO_INTF_TYPE_E enIntfType;     /* [IN ] default: AX_VO_INTF_HDMI */
    AX_VO_INTF_SYNC_E enIntfSync;     /* [IN ] default: AX_VO_OUTPUT_1080P60 */
    AX_BOOL bForceDisplay;            /* [IN ] AX_TRUE: force display ignore EDID, default: AX_TRUE */
    AX_BOOL bLinkVo2Disp;             /* [IN ] AX_FALSE: Unlink mode gives users the ability to get layer frame before send to DISP */
    AX_VO_ENGINE_MODE_E enEngineMode; /* [IN ] if nEngineId == 3, set to AX_VO_ENGINE_MODE_FORCE, default: AX_VO_ENGINE_MODE_AUTO */
    AX_U32 nEngineId;                 /* [IN ] 0: VO0 1: VO1, 3: TDP */
    AX_U32 nBgClr;                    /* [IN ] background color, fmt: RGB888 */
    AX_U32 nLayerDepth;               /* [IN ] layer depth, [3 - ], if less than 3, set to 3 */
    VO_CHN_INFO_T stChnInfo;          /* [IN ] */
    AX_VO_PART_MODE_E enDrawMode;     /* [IN ] draw mode, PIP pipe: single */
    AX_VO_MODE_E  enVoMode;           /* [IN ] vo mode, online or offline, default: AX_VO_MODE_OFFLINE*/
    AX_U32 nDetectW;                  /* [IN ] width of detect channel for converting region position */
    AX_U32 nDetectH;                  /* [IN ] height of detect channel for converting region position */
    AX_U32 nImageW;
    AX_U32 nImageH;
    AX_U32 arrImageW[MAX_VO_CHN_NUM];
    AX_U32 arrImageH[MAX_VO_CHN_NUM];
    VO_CHN pipChn;
    AX_VO_LAYER_SYNC_MODE_E enSyncMode;

    VO_ATTR_S(AX_VOID) {
        dev = 0;
        voLayer = (AX_U32)-1;
        uiLayer = (AX_U32)-1;
        s32FBIndex[0] = -1;
        s32FBIndex[1] = -1;
        enIntfType = AX_VO_INTF_HDMI;
        enIntfSync = AX_VO_OUTPUT_1080P60;
        bForceDisplay = AX_TRUE;
        enEngineMode = AX_VO_ENGINE_MODE_AUTO;
        nEngineId = 0;
        nBgClr = 0x000000; /* black */
        nLayerDepth = 3;
        enDrawMode = AX_VO_PART_MODE_MULTI;
        enVoMode = AX_VO_MODE_OFFLINE;
        nDetectW = 512;
        nDetectH = 288;
        pipChn = MAX_VO_CHN_NUM;
        enSyncMode = AX_VO_LAYER_SYNC_NORMAL;
    }
} VO_ATTR_T;

typedef struct VO_POINT_ATTR {
    AX_U32 nX;
    AX_U32 nY;

    VO_POINT_ATTR() {
        nX = 0;
        nY = 0;
    }

    VO_POINT_ATTR(AX_U32 _x, AX_U32 _y) {
        nX = _x;
        nY = _y;
    }
} VO_POINT_ATTR_T;

typedef struct VO_LINE_ATTR {
    VO_POINT_ATTR_T ptStart;
    VO_POINT_ATTR_T ptEnd;

    VO_LINE_ATTR() {
        ptStart = {0, 0};
        ptEnd = {0, 0};
    }

    VO_LINE_ATTR(AX_U32 nX1, AX_U32 nY1, AX_U32 nX2, AX_U32 nY2) {
        ptStart.nX = nX1;
        ptStart.nY = nY1;
        ptEnd.nX = nX2;
        ptEnd.nY = nY2;
    }
} VO_LINE_ATTR_T;

typedef struct VO_RECT_ATTR {
    AX_U32 nX;
    AX_U32 nY;
    AX_U32 nW;
    AX_U32 nH;

    VO_RECT_ATTR() {
        memset(this, 0, sizeof(VO_RECT_ATTR));
    }

    VO_RECT_ATTR(AX_U32 _x, AX_U32 _y, AX_U32 _w, AX_U32 _h) {
        nX = _x;
        nY = _y;
        nW = _w;
        nH = _h;
    }
} VO_RECT_ATTR_T;

typedef struct VO_REGION_INFO {
    AX_BOOL bValid {AX_FALSE};
    AX_S32 nChn {-1};
    std::vector<VO_LINE_ATTR_T> vecLines;
    std::vector<VO_RECT_ATTR_T> vecRects;

    VO_REGION_INFO() {
        bValid = AX_FALSE;
        vecLines.clear();
        vecRects.clear();
    }

    AX_VOID Clear() {
        vecLines.clear();
        vecRects.clear();
    }
} VO_REGION_INFO_T;

class CVO {
public:
    static CVO* CreateInstance(VO_ATTR_T& stAttr);
    AX_BOOL Destory(AX_VOID);

    /**
     * @brief Start/Stop flow:
     *    enable  device -> enable  layer -> enable  chns -> ...
     *    disable device <- disable layer <- disable chns <-
     *
     * @return AX_TRUE if success, otherwise failure
     */
    AX_BOOL Start(AX_VOID);
    AX_BOOL Stop(AX_VOID);

    /**
     * @brief update all video channels of whole layer, example: 2x2 to 8x8
     *
     * @return AX_TRUE if success, otherwise failure
     */
    AX_BOOL UpdateChnInfo(CONST VO_CHN_INFO_T& stChnInfo);

    AX_BOOL UpdateChnCropRect(VO_CHN voChn, CONST AX_IVPS_RECT_T& stCropRect);

    /* set video channel fps */
    AX_BOOL SetChnFps(VO_CHN voChn, AX_F32 fps);

    AX_BOOL SendFrame(VO_CHN voChn, CAXFrame& axFrame, AX_S32 nTimeOut = INFINITE);

    /* video channel control */
    AX_BOOL EnableChn(VO_CHN voChn, CONST AX_VO_CHN_ATTR_T& stChnAttr);
    AX_BOOL DisableChn(VO_CHN voChn);

    AX_BOOL PauseChn(VO_CHN voChn);
    AX_BOOL RefreshChn(VO_CHN voChn);
    AX_BOOL StepChn(VO_CHN voChn);
    AX_BOOL ResumeChn(VO_CHN voChn);

    AX_BOOL ShowChn(VO_CHN voChn);
    AX_BOOL HideChn(VO_CHN voChn);
    AX_BOOL IsHidden(VO_CHN voChn);

    AX_BOOL ClearChn(VO_CHN voChn, AX_BOOL bClrAll = AX_TRUE);

    AX_BOOL GetChnPTS(VO_CHN voChn, AX_U64& pts);

    AX_VOID SetPipRect(const AX_VO_RECT_T& tRect);
    AX_BOOL CoordinateConvert(VO_CHN voChn, const DETECT_RESULT_T& tResult);
    AX_VOID ClearRegions(VO_CHN voChn);
    AX_BOOL UpdateChnResolution(VO_CHN voChn, AX_U32 nWidth, AX_U32 nHeight);

    AX_S32 CrtcOff();
    AX_S32 CrtcOn();
    AX_S32 RegCallbackFunc(AX_VSYNC_CALLBACK_FUNC_T& stVoCallbackFun);
    AX_S32 UnRegCallbackFunc();

    /* property */
    CONST VO_ATTR_T& GetAttr(AX_VOID) CONST;

    /* !!! not invoke directly, invoke CreateInstance and Destory API !!! */
    AX_BOOL Init(VO_ATTR_T& stAttr);
    AX_BOOL DeInit(AX_VOID);

protected:
    AX_BOOL GetDispInfoFromIntfSync(AX_VO_INTF_SYNC_E eIntfSync, AX_VO_RECT_T& stArea, AX_U32& nHz);
    AX_POOL CreateLayerPool(VO_DEV dev, AX_U32 nBlkSize, AX_U32 nBlkCnt);
    AX_BOOL DestoryLayerPool(AX_POOL& pool);

    AX_BOOL EnableVideoLayer(AX_VOID);
    AX_BOOL DisableVideoLayer(AX_VOID);

    AX_BOOL BatchUpdate(AX_BOOL bEnable, CONST VO_CHN_INFO_T& stChnInfo);

    AX_BOOL CheckAttr(VO_ATTR_T& stAttr);
    AX_BOOL CheckChnInfo(CONST VO_CHN_INFO_T& stChnInfo);

    AX_VOID PaintRegions(AX_VIDEO_FRAME_T* pLayerFrame);
    AX_BOOL GenRegionsForPipRect(VO_RECT_ATTR_T& tRect, std::vector<VO_RECT_ATTR_T>& vecOutConvertedRect,
                                 std::vector<VO_LINE_ATTR_T>& vecOutConvertedLines);
    AX_VOID LayerFrameGetThread(AX_VOID* pArg);

    AX_BOOL PointInRect(VO_POINT_ATTR_T& tPoint, VO_RECT_ATTR_T& tRect);

private:
    CVO(AX_VOID);
    CVO(CONST CVO&) = delete;
    CVO& operator=(CONST CVO&) = delete;

private:
    VO_ATTR_T m_stAttr;
    AX_BOOL m_bInited = {AX_FALSE};
    AX_BOOL m_bStarted = {AX_FALSE};
    AX_BOOL m_bChnEnable[MAX_VO_CHN_NUM];
    AX_POOL m_LayerPool = {AX_INVALID_POOLID};
    std::vector<VO_REGION_INFO_T> m_vecRegionInfo;
    VO_RECT_ATTR_T m_tPipRect;
    CAXThread m_thread;
    std::mutex m_mtxRegions;
    AX_VSYNC_CALLBACK_FUNC_T m_stVoCallbackFun;
};

inline CONST VO_ATTR_T& CVO::GetAttr(AX_VOID) CONST {
    return m_stAttr;
}


class CVOLayerRegionObserver : public IObserver {
public:
    CVOLayerRegionObserver(CVO* pSink) : m_pSink(pSink){};

    AX_BOOL OnRecvData(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, AX_VOID* pData) override {
        std::map<AX_S32, AX_S32>::iterator itFinder = m_mapSrcGrp2VoChn.find(nGrp);
        if (itFinder == m_mapSrcGrp2VoChn.end() || -1 == itFinder->second) {
            return AX_TRUE;
        }

        DETECT_RESULT_T tRect = *(DETECT_RESULT_T*)pData;
        if (!m_pSink) {
            return AX_FALSE;
        }

        m_pSink->CoordinateConvert(itFinder->second, tRect);
        return AX_TRUE;
    }

    AX_BOOL OnRegisterObserver(OBS_TARGET_TYPE_E, AX_U32, AX_U32, OBS_TRANS_ATTR_PTR pParams) override {
        return AX_TRUE;
    }

    AX_VOID RegisterSrcGrp(AX_S32 nSrcGrp, AX_S32 nVoChn) {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_mapSrcGrp2VoChn[nSrcGrp] = nVoChn;
    }

    AX_VOID UnRegisterSrcGrp(AX_S32 nSrcGrp) {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::map<AX_S32, AX_S32>::iterator itFinder = m_mapSrcGrp2VoChn.find(nSrcGrp);
        if (itFinder == m_mapSrcGrp2VoChn.end() || -1 == itFinder->second) {
            return;
        }

        m_pSink->ClearRegions(itFinder->second);
        m_mapSrcGrp2VoChn[nSrcGrp] = -1;
    }

private:
    CVO* m_pSink{nullptr};
    std::mutex m_mutex;
    std::map<AX_S32, AX_S32> m_mapSrcGrp2VoChn;
};
