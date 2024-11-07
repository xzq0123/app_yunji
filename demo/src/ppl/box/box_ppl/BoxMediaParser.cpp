// /**************************************************************************************************
//  *
//  * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
//  *
//  * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
//  * may not be copied or distributed in any isomorphic form without the prior
//  * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
//  *
//  **************************************************************************************************/
#include "AppLogApi.h"
#include "BoxMediaParser.hpp"
#include <fstream>

using namespace std;

#define TAG "PARSER"

template <typename T>
static AX_VOID SET_VALUE(picojson::value &argObj, const string &name, T v)
{
    argObj.get<picojson::object>()[name] = picojson::value(v);
}

template <typename T>
static AX_BOOL GET_VALUE(const picojson::value &argObj, const string &name, T &v)
{
    const picojson::value& obj = argObj.get(name);
    if (obj.is<T>()) {
        v = obj.get<T>();
        return AX_TRUE;
    }
    LOG_M_E(TAG, "get remote device faield <%s> failed.", name);
    return AX_FALSE;
}

std::vector<MEDIA_INFO_T> CBoxMediaParser::GetMediasMap(AX_U32 *nMediaCnt, const std::string &strPath) {

    std::vector<MEDIA_INFO_T> vecMedia;
    LOG_M_D(TAG, "[%s][%d] +++ ", __func__, __LINE__);

    do {
        m_strPath = std::move(strPath);
        std::ifstream file(m_strPath);
        if (!file.is_open()) {
            LOG_M_E(TAG, "Failed to open json config file: %s", m_strPath.c_str());
            break;
        }

        picojson::value json;
        file >> json;
        string err = picojson::get_last_error();
        if (!err.empty()) {
            LOG_M_E(TAG, "Failed to load json config file: %s", m_strPath.c_str());
            break;
        }

        if (!json.is<picojson::object>()) {
            LOG_M_E(TAG, "Loaded config file is not a well-formatted JSON.");
            break;
        }
        // parse remote device
        else {
            double nCnt = 0;
            if (!GET_VALUE(json, "count", nCnt)) {
                break;
            }

            *nMediaCnt = (AX_U32)nCnt;

            const picojson::value& device_list_value = json.get("medias");
            if (device_list_value.is<picojson::array>()) {
                const picojson::array& arr_device = device_list_value.get<picojson::array>();
                if (*nMediaCnt > arr_device.size()) {
                    LOG_M_W(TAG, "device size is invalid.");
                    *nMediaCnt = arr_device.size();
                }

                double dValue = 0.0;
                string strValue = "";
                for (auto device_value : arr_device) {

                    MEDIA_INFO_T devInfo;
                    if (!GET_VALUE(device_value, "channel", dValue)) break;
                    devInfo.nMediaId = dValue;

                    if (!GET_VALUE(device_value, "disable", dValue)) break;
                    devInfo.nMediaDisable = dValue;

                    if (!GET_VALUE(device_value, "url", strValue)) break;
                    strcpy(devInfo.szMediaUrl, strValue.c_str());

                    if (!GET_VALUE(device_value, "name", strValue)) break;
                    strcpy(devInfo.szMediaName, strValue.c_str());

                    if (!GET_VALUE(device_value, "desc", strValue)) break;
                    strcpy(devInfo.szMediaDesc, strValue.c_str());

                    vecMedia.emplace_back(devInfo);
                }
            }
        }
    } while (0);

    LOG_M_D(TAG, "[%s][%d] --- ", __func__, __LINE__);
    return vecMedia;
}

AX_BOOL CBoxMediaParser::SetMediasMap(std::vector<MEDIA_INFO_T>& vecMedia) {
    AX_BOOL bRet = AX_FALSE;
    LOG_M_D(TAG, "[%s][%d] +++ ", __func__, __LINE__);

    do {
        picojson::object obj;
        obj["count"] = picojson::value((double)vecMedia.size());

        picojson::array arr;
        for (auto &info : vecMedia) {
            picojson::object objDev;
            objDev["channel"] = picojson::value((double)info.nMediaId);
            objDev["disable"] = picojson::value((double)info.nMediaDisable);
            objDev["url"] = picojson::value(string(info.szMediaUrl));
            objDev["name"] = picojson::value(string(info.szMediaName));
            objDev["desc"] = picojson::value(string(info.szMediaDesc));

            arr.push_back(picojson::value(objDev));
        }
        obj["medias"] = picojson::value(arr);

        std::ofstream file(m_strPath);
        if (!file.is_open()) {
            LOG_M_E(TAG, "Failed to open json config file: %s", m_strPath.c_str());
            break;
        }
        file << picojson::value(obj).serialize(true);
        string err = picojson::get_last_error();
        if (!err.empty()) {
            LOG_M_E(TAG, "Failed to save json config file: %s", m_strPath.c_str());
            break;
        }

        bRet = AX_TRUE;

    } while (0);

    LOG_M_D(TAG, "[%s][%d] --- ", __func__, __LINE__);
    return bRet;
}

string CBoxMediaParser::GetExecPath(AX_VOID) {
    string strPath;
    AX_CHAR szPath[260] = {0};
    ssize_t sz = readlink("/proc/self/exe", szPath, sizeof(szPath));
    if (sz <= 0) {
        strPath = "./";
    } else {
        strPath = szPath;
        strPath = strPath.substr(0, strPath.rfind('/') + 1);
    }

    return strPath;
}