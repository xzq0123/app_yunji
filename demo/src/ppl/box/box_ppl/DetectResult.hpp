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
#include <map>
#include <mutex>
#include <iostream>
#include <set>
#include <vector>
#include <unordered_set>
#include "AXSingleton.h"
#include "ax_skel_type.h"

#define MAX_DETECT_RESULT_COUNT (32)
#define MAX_CHANNEL_SIZE (16)
#define MAX_RESULT_SIZE (5)

//临时写一个算法列表，火焰，动物，手势，抽烟
typedef enum {
    DETECT_TYPE_PEOPLE = 0,
    DETECT_TYPE_VEHICLE = 1,
    DETECT_TYPE_FACE = 2,
    DETECT_TYPE_FIRE = 3,
    DETECT_TYPE_TOTAL
} DETECT_TYPE_E;

typedef struct {
    DETECT_TYPE_E eType;
    AX_U64 nTrackId;
    AX_SKEL_RECT_T tBox;

    /*
    0到1之间的值，表示人脸质量，越高越好
    */
    AX_F32 quality;

    /*
    人体状态： 0：正面， 1：侧面，2：背面， 3：非人
    */
    AX_S32 status;

    /*
    车辆类型: 0：UNKNOWN 1：SEDAN 2：SUV 3：BUS 4：MICROBUS 5：TRUCK
    */
    AX_S32 cartype;
    /*
    如果 b_is_track_plate = 1，则表示当前帧没有识别到车牌，返回的是历史上 track_id 上一次识别到的车牌结果
    如果 b_is_track_plate = 0，且 len_plate_id > 0, 则表示当前帧识别到了车牌
    如果 b_is_track_plate = 0，且 len_plate_id = 0, 则表示当前帧没有识别到车牌，且是历史上 track_id 也没有结果
    */
    AX_S32 b_is_track_plate;
    AX_S32 len_plate_id;
    AX_S32 plate_id[16];

} DETECT_RESULT_ITEM_T;

typedef struct DETECT_RESULT_S {
    AX_U64 nSeqNum;
    AX_U32 nW;
    AX_U32 nH;
    AX_U32 nCount;
    AX_S32 nGrpId;
    AX_U32 nAlgoType;
    AX_U32 result_diff;
    DETECT_RESULT_ITEM_T item[MAX_DETECT_RESULT_COUNT];

    DETECT_RESULT_S(AX_VOID) {
        memset(this, 0, sizeof(*this));
    }

} DETECT_RESULT_T;

/**
 * @brief
 *
 */
class CDetectResult : public CAXSingleton<CDetectResult> {
    friend class CAXSingleton<CDetectResult>;

public:
    // 需要在这里进行组合
    AX_BOOL Set(AX_S32 nGrp, const DETECT_RESULT_T& cur_result) {
        std::lock_guard<std::mutex> lck(m_mtx);
        auto &last_result = m_mapRlts[nGrp];

        DETECT_RESULT_T new_result;
        DETECT_RESULT_T few_result;
        // 说明是推理当前帧的多个算法
        if (last_result.nSeqNum == cur_result.nSeqNum && last_result.nAlgoType != cur_result.nAlgoType) {
            // 先判断差异，找到最多的，从最多的增加。
            if (cur_result.nCount >= last_result.nCount) {
                new_result = cur_result;
                few_result = last_result;
            } else {
                new_result = last_result;
                few_result = cur_result;
                new_result.nAlgoType = cur_result.nAlgoType;
            }

            int i = new_result.nCount, j = 0;
            int temp_count = new_result.nCount + last_result.nCount;
            int sum_count = temp_count < MAX_DETECT_RESULT_COUNT ? temp_count : MAX_DETECT_RESULT_COUNT;
            for (; i < sum_count; i++ && j++) {
                new_result.item[i].eType = few_result.item[j].eType;
                new_result.item[i].nTrackId  = few_result.item[j].nTrackId;
                new_result.item[i].tBox  = few_result.item[j].tBox;
            }
            new_result.nCount = sum_count;
        } else {
            new_result = cur_result;
        }

#ifdef __USE_AX_ALGO
        for (AX_U32 i = 0; i < new_result.nCount; ++i) {
            auto id = new_result.item[i].nTrackId;
            if (last_tracked_ids.find(id) == last_tracked_ids.end()) {
                new_result.result_diff = true;
            }
            last_tracked_ids.insert(id);
        }

        while (last_tracked_ids.size() > 50) {
            // 删除最小的元素（set.begin() 指向最小的元素）
            last_tracked_ids.erase(last_tracked_ids.begin());
        }
#else
        //现在的问题：检测容易漏检，导致跟踪算法容易跟丢,容易出现新的track id
        //如果某一帧跟丢的话，判断上一帧的结果，下一帧肯定找不到上一帧的track id
        //两次的数量相同，说明当前是较稳定的,把这个结果保存起来。
        if (last_result.nCount == new_result.nCount) {
            //这里的result就是表示上一次稳定目标的结果
            auto result = channel_result[nGrp];

            if (result.nCount == 0) {
                new_result.result_diff = true;
            } else {
                std::unordered_set<int> last_track_ids = track_id_set(result);
                //当前结果与上一次稳定结果相比较
                new_result.result_diff = has_difference(last_track_ids, new_result);
            }

            if (new_result.result_diff == true) {
                channel_result[nGrp] = new_result;
            }
        }
#endif

        m_mapRlts[nGrp] = new_result;

        // 统计每个类型的总数量
        for (AX_U32 i = 0; i < new_result.nCount; ++i) {
            ++m_arrCount[new_result.nAlgoType];
        }

        return AX_TRUE;
    }


    AX_BOOL Get(AX_S32 nGrp, DETECT_RESULT_T& result) {
        std::lock_guard<std::mutex> lck(m_mtx);
        if (m_mapRlts.end() == m_mapRlts.find(nGrp)) {
            return AX_FALSE;
        }

        result = m_mapRlts[nGrp];
        m_mapRlts.erase(nGrp);
        return AX_TRUE;
    }

    AX_U64 GetTotalCount(DETECT_TYPE_E eType) {
        std::lock_guard<std::mutex> lck(m_mtx);
        return m_arrCount[eType];
    }

    AX_VOID Clear(AX_VOID) {
        std::lock_guard<std::mutex> lck(m_mtx);
        memset(m_arrCount, 0, sizeof(m_arrCount));
        m_mapRlts.clear();
    }

protected:
    CDetectResult(AX_VOID)  = default;
    virtual ~CDetectResult(AX_VOID) = default;

private:
    std::mutex m_mtx;
    std::map<AX_S32, DETECT_RESULT_T> m_mapRlts;
    std::set<int> last_tracked_ids;

    AX_U64 m_arrCount[DETECT_TYPE_TOTAL] = {0};
};
