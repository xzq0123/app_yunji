#include "MqttClientServer.hpp"

#include "cmdline.hpp"
#include "AppLog.hpp"
#include "AiSwitchConfig.hpp"
#include "DiskHelper.hpp"
#include "ElapsedTimer.hpp"
#include "AiCardMstBuilder.hpp"

#include "httplib.h"
#include "nlohmann/json.hpp"

#define MQTT_CLIENT "MQTT_CLIENT"

#define ZLM_IP      "127.0.0.1"
#define ZLM_API_URL "http://" ZLM_IP ":80"
#define ZLM_SECRET  "81DBE7AF-ACD5-47D8-A692-F4B27456E6FD"

#define ALARM_IMG_PATH "/mnt/nfs/ZLMediaKit/www/alarm"

using namespace std;
using json = nlohmann::json;

static int arrivedcount = 0;
CTransferHelper* m_pTransferHelper = nullptr;

static std::shared_ptr<IPStack> ipstack_ = nullptr;
static std::shared_ptr<MQTT::Client<IPStack, Countdown>> client_ = nullptr;

static std::map<std::string, std::string> gb28181_discovery_;

std::vector<AX_BOOL> stream_status_;

static int MqttGetTemperature(int &temp) {

    std::ifstream temp_file("/sys/class/thermal/thermal_zone0/temp");
    if (!temp_file) {
        std::cerr << "Cannot open temperature file" << std::endl;
        temp = -1;
        return -1;
    }

    std::string temp_line;
    if (!std::getline(temp_file, temp_line)) {
        std::cerr << "Cannot read temperature" << std::endl;
        temp = -1;
        return -1;
    }

    long tempMicroCelsius = std::stol(temp_line);
    temp = static_cast<int>(tempMicroCelsius / 1000.0);
    return 0;
}

static int MqttGetVersion(std::string& version) {

    std::ifstream temp_file("/proc/ax_proc/version");
    if (!temp_file) {
        std::cerr << "Cannot open temperature file" << std::endl;
        return -1;
    }

    std::string temp_line;
    if (!std::getline(temp_file, temp_line)) {
        std::cerr << "Cannot read temperature" << std::endl;
        return -1;
    }

    version = std::move(temp_line);
    return 0;
}

static int MqttGetSystime(std::string& timeString) {
    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    std::tm* localTime = std::localtime(&currentTime);

    std::ostringstream oss;
    oss << std::put_time(localTime, "%Y-%m-%d %H:%M:%S");
    timeString = oss.str();
    return 0;
}

int MqttGetIP(const std::string interfaceName, std::string &ip_addr) {
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return -1;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) 
            continue;

        family = ifa->ifa_addr->sa_family;

        // Check if interface is valid and IPv4
        if ((family == AF_INET) && (strcmp(ifa->ifa_name, interfaceName.c_str()) == 0)) {
            s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                continue;
            }
            
            LOG_M_D(MQTT_CLIENT, "Interface %s has IP address: %s", interfaceName.c_str(), host);
            ip_addr = std::string(host);
            freeifaddrs(ifaddr);
            return 0;
        }
    }

    freeifaddrs(ifaddr);
    return -1;
}

static int MqttMemoryInfo(MemoryInfo &memInfo) {
    std::ifstream meminfoFile("/proc/meminfo");
    if (!meminfoFile.is_open()) {
        std::cerr << "Unable to open /proc/meminfo" << std::endl;
        return -1;
    }

    std::string line;
    while (std::getline(meminfoFile, line)) {
        std::istringstream iss(line);
        std::string key;
        long value;
        std::string unit;
        iss >> key >> value >> unit;

        if (key == "MemTotal:") {
            memInfo.totalMem = value;
        } else if (key == "MemFree:") {
            memInfo.freeMem = value;
        } else if (key == "MemAvailable:") {
            memInfo.availableMem = value;
        } else if (key == "Buffers:") {
            memInfo.buffers = value;
        } else if (key == "Cached:") {
            memInfo.cached = value;
        }
    }
    meminfoFile.close();
    return 0;
}

static int getDiskUsage(const std::string& path, FlashInfo &falsh_info) {
    struct statvfs stat;

    if (statvfs(path.c_str(), &stat) != 0) {
        // 错误处理
        perror("statvfs");
        return -1;
    }

    falsh_info.total = stat.f_blocks * stat.f_frsize;
    falsh_info.free = stat.f_bfree * stat.f_frsize;
    return 0;
}

static bool SendMsg(const char *topic, const char *msg, size_t len) {
    printf("SendMsg ++++\n");

    MQTT::Message message;
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.id = 0;
    message.payload = (void *)msg;
    message.payloadlen = len;
    if (client_) {
        int ret = client_->publish(topic, message);
        if (ret != 0) {
            printf("MQTT CLIENT publish [%s] message [%s] failed ...\n", topic, msg);
        }
    } else {
        printf("MQTT CLIENT can not get client to publish\n");
    }

    printf("SendMsg ----\n");

    return true;
}

static void OnLogin(std::string& account, std::string& password) {
    printf("OnLogin ++++\n");
    json child;
    child["type"] = "login";

    json root;
    root["data"] = child;
    if (account == "admin" && password == "admin") {
        root["result"] = 0;
        root["msg"] = "success";
    } else {
        root["result"] = -1;
        root["msg"] = "The account or password is incorrect";
    }

    std::string payload = root.dump();

    SendMsg("web-message", payload.c_str(), payload.size());

    printf("OnLogin ----\n");
}

static void OnRebootAiBox() {
    printf("OnRebootAiBox ++++\n");

    json child;
    child["type"] = "rebootAiBox";

    system("reboot");

    json root;
    root["result"] = 0;
    root["msg"] = "success";
    root["data"] = child;

    std::string payload = root.dump();

    SendMsg("web-message", payload.c_str(), payload.size());
    system("reboot");

    printf("OnRebootAiBox ----\n");
}

static void OnRestartMasterApp() {
    printf("OnRestartMaster ++++\n");

    json child;
    child["type"] = "restartMasterApp";

    json root;
    root["result"] = 0;
    root["msg"] = "success";
    root["data"] = child;

    std::string payload = root.dump();

    SendMsg("web-message", payload.c_str(), payload.size());

    printf("OnRestartMaster ----\n");
}

static void OnGetDashBoardInfo() {
    printf("OnDashBoardInfo ++++\n");

    int temperature = -1;
    int ret = MqttGetTemperature(temperature);
    if (ret == -1) {
        LOG_MM_D(MQTT_CLIENT, "MqttGetTemperature fail.");
    }
    
    const std::string interfaceName = "eth0";
    std::string ipAddress;
    ret = MqttGetIP(interfaceName, ipAddress);
    if (ret == -1) {
        LOG_MM_D(MQTT_CLIENT, "MqttGetIP fail.");
    }

    MemoryInfo memInfo = {0};
    ret = MqttMemoryInfo(memInfo);
    if (ret == -1) {
        LOG_MM_D(MQTT_CLIENT, "MqttMemoryInfo fail.");
    }

    FlashInfo falsh_info = {0};
    ret = getDiskUsage("/", falsh_info);

    std::string currentTimeStr;
    MqttGetSystime(currentTimeStr);

    std::string version;
    MqttGetVersion(version);

    json child = {
        {"type", "getDashBoardInfo"}, 
        {"BoardId", "YJ-AIBOX-001"}, 
        {"BoardIp", ipAddress},
        {"BoardPlatform", "AX650"},
        {"BoardTemp", temperature},
        {"BoardType", "LAN"},
        {"BoardAuthorized", "已授权"},
        {"Time", currentTimeStr},
        {"Version", version},
        {"HostDisk", { // 当前设备硬盘情况 kB
            {"Total", falsh_info.total}, // 总量
            {"Available", falsh_info.free} // 已用
        }},
        {"HostMemory", { // 当前设备内存使用情况
            {"Total", memInfo.totalMem}, // 总量
            {"Available", memInfo.availableMem}, // 总量
        }},
        {"Tpu", { // 当前设备的算力资源使用情况（0.0.46+引入）
            { 
                {"mem_total", 2048}, // 内存总量
                {"mem_usage", 1 + rand()%(2048)}, // 内存使用情况
                {"tpu_usage", 1 + rand()%(100)} // TPU 使用情况
            }
        }},
    };

    json root;
    root["result"] = 0;
    root["msg"] = "success";
    root["data"] = child;

    std::string payload = root.dump();

    SendMsg("web-message", payload.c_str(), payload.size());

    printf("OnDashBoardInfo ----\n");
}

static void OnGetMediaChannelInfo() {
    printf("OnMediaChannelInfo ++++\n");
    json arr = nlohmann::json::array();
    STREAM_CONFIG_T streamConfig = CAiCardMstConfig::GetInstance()->GetStreamConfig();

    // init stream status
    if (0 == stream_status_.size()) {
        const AX_U32 nCount = streamConfig.v.size();
        printf(">>>> init stream status vector size: %u <<<<\n", nCount);

        stream_status_.resize(nCount);
        for (size_t i = 0; i < stream_status_.size(); i++) {
            stream_status_[i] = AX_TRUE;
        }
    }

    for (size_t i = 0; i < streamConfig.v.size(); i++) {
        arr.push_back({
            {"channelId", i}, 
            {"channelName", i + 1}, 
            {"channelStatus", stream_status_[i]==AX_TRUE ? 1 : 0}, 
            {"protocol", "rtsp"}, 
            {"channelUrl", streamConfig.v[i]}
        });
    }

    int device_count = 0;
    auto itr = gb28181_discovery_.begin();
	for (; itr != gb28181_discovery_.end(); ++itr) {
        arr.push_back({
            {"channelId", device_count}, 
            {"channelName", device_count + 1}, 
            {"channelStatus", 1}, 
            {"protocol", "gb28181"}, 
            {"channelUrl", itr->second}
        });
        device_count++;
	}

    json child;
    child["type"] = "getMediaChannelInfo";
    child["channels"] = arr;

    json root;
    root["result"] = 0;
    root["msg"] = "success";
    root["data"] = child;

    std::string payload = root.dump();

    SendMsg("web-message", payload.c_str(), payload.size());

    printf("OnMediaChannelInfo ----\n");
}

static void OnSetMediaChannelInfo(int channelId, std::string& streamUrl) {
    printf("OnSetMediaChannelInfo ++++\n");
    STREAM_CONFIG_T streamConfig = CAiCardMstConfig::GetInstance()->GetStreamConfig();

    json child;
    child["type"] = "setMediaChannelInfo";

    json root;
    root["data"] = child;
    if (channelId < (int)streamConfig.v.size()) {
        streamConfig.v[channelId] = streamUrl;

        // update ai card mst config stream url
        if (CAiCardMstConfig::GetInstance()->SetStreamUrl(channelId + 1, streamUrl)) {
            if (CAiCardMstBuilder::GetInstance()->StopStream(channelId)) {
                if (CAiCardMstBuilder::GetInstance()->StartStream(channelId)) {
                    stream_status_[channelId] = AX_TRUE;

                    root["result"] = 0;
                    root["msg"] = "success";
                } else {
                    root["result"] = -1;
                    root["msg"] = "start stream failed!";
                }
            } else {
                root["result"] = -1;
                root["msg"] = "stop stream failed!";
            }
        } else {
            root["result"] = -1;
            root["msg"] = "set stream url failed!";
        }
    } else {
        root["result"] = -1;
        root["msg"] = "invalid channel id";
    }

    std::string payload = root.dump();

    SendMsg("web-message", payload.c_str(), payload.size());

    printf("OnSetMediaChannelInfo ----\n");
}

static void OnDelMediaChannelInfo(int channelId) {
    printf("OnDelMediaChannelInfo ++++\n");
    STREAM_CONFIG_T streamConfig = CAiCardMstConfig::GetInstance()->GetStreamConfig();

    json child;
    child["type"] = "delMediaChannelInfo";

    json root;
    root["data"] = child;
    if (channelId < (int)streamConfig.v.size()) {
        if (CAiCardMstBuilder::GetInstance()->StopStream(channelId)) {
            stream_status_[channelId] = AX_FALSE;

            root["result"] = 0;
            root["msg"] = "success";
        } else {
            root["result"] = -1;
            root["msg"] = "stop stream failed!";
        }
    } else {
        root["result"] = -1;
        root["msg"] = "invalid channel id";
    }

    std::string payload = root.dump();

    SendMsg("web-message", payload.c_str(), payload.size());

    printf("OnDelMediaChannelInfo ----\n");
}

static void OnGetMediaChannelTaskInfo() {
    printf("OnMediaChannelTaskInfo ++++\n");

    json child;
    child["type"] = "getMediaChannelTaskInfo";

    json root;
    root["result"] = 0;
    root["msg"] = "success";
    root["data"] = child;

    std::string payload = root.dump();

    SendMsg("web-message", payload.c_str(), payload.size());

    printf("OnMediaChannelTaskInfo ----\n");
}

static void OnGetAiModelList() {
    printf("OnGetAiModelList ++++\n");

    json arr = nlohmann::json::array();
    arr.push_back({
        {"modelId", 0}, 
        {"modePath", "/opt/etc/skelModels/1024x576/part/ax_ax650_hvcfp_algo_model_V1.0.2.axmodel"}, 
        {"modeName", "人车非"}, 
    });
    arr.push_back({
        {"modelId", 1}, 
        {"modePath", "/opt/etc/skelModels/1024x576/part/ax_ax650_fire_algo_model_V1.0.2.axmodel"}, 
        {"modeName", "火焰检测"}, 
    });

    json child;
    child["type"] = "getAiModelList";
    child["models"] = arr;

    json root;
    root["result"] = 0;
    root["msg"] = "success";
    root["data"] = child;

    std::string payload = root.dump();

    SendMsg("web-message", payload.c_str(), payload.size());

    printf("OnGetAiModelList ----\n");
}

static void OnSwitchChannelAiModel() {
    printf("OnwitchChannelAiModel ++++\n");

    json child;
    child["type"] = "switchChannelAiModel";

    AI_CARD_AI_SWITCH_ATTR_T tAiAttr;
    CAiSwitchConfig::GetInstance()->GetNextAttr(tAiAttr);
    if (m_pTransferHelper != nullptr) {
        m_pTransferHelper->SendAiAttr(tAiAttr);
    }

    json root;
    root["result"] = 0;
    root["msg"] = "success";
    root["data"] = child;

    std::string payload = root.dump();

    SendMsg("web-message", payload.c_str(), payload.size());

    printf("OnwitchChannelAiModel ----\n");
}

static void OnStartRtspPreview(std::string& streamUrl) {
    printf("OnStartRtspPreview ++++\n");

    time_t t = time(nullptr);

    httplib::Client httpclient(ZLM_API_URL);
    httplib::Logger logger([](const httplib::Request &req, const httplib::Response &res) {
        printf("=====================================================================\n");
        printf("http request path=%s, body=%s\n", req.path.c_str(), req.body.c_str());
        printf("=====================================================================\n");
        printf("http response body=\n%s", res.body.c_str());
        printf("=====================================================================\n"); });
    httpclient.set_logger(logger);

    char api[128] = {0};
    sprintf(api, "/index/api/addStreamProxy?secret=%s&vhost=%s&app=%s&stream=%ld&url=%s", ZLM_SECRET, ZLM_IP, "live", t, streamUrl.c_str());

    json child;
    child["type"] = "startRtspPreview";
    child["streamUrl"] = streamUrl;

    json root;
    httplib::Result result = httpclient.Get(api);
    if (result && result->status == httplib::OK_200) {
        auto jsonRes = nlohmann::json::parse(result->body);
        int code = jsonRes["code"];
        if (code == 0) {
            std::string key = jsonRes["data"]["key"];
            child["key"] = key;

            root["result"] = 0;
            root["msg"] = "success";
        } else {
            root["result"] = -1;
            root["msg"] = jsonRes["msg"];
        }
    }
    root["data"] = child;

    std::string payload = root.dump();

    SendMsg("web-message", payload.c_str(), payload.size());

    printf("OnStartRtspPreview ----\n");
}

static void OnStopRtspPreview(std::string& key) {
    printf("OnStopRtspPreview ++++\n");

    httplib::Client httpclient(ZLM_API_URL);
    httplib::Logger logger([](const httplib::Request &req, const httplib::Response &res) {
        printf("=====================================================================\n");
        printf("http request path=%s, body=%s\n", req.path.c_str(), req.body.c_str());
        printf("=====================================================================\n");
        printf("http response body=\n%s", res.body.c_str());
        printf("=====================================================================\n"); });
    httpclient.set_logger(logger);

    char api[128] = {0};
    sprintf(api, "/index/api/delStreamProxy?secret=%s&key=%s", ZLM_SECRET, key.c_str());

    json child;
    child["type"] = "stopRtspPreview";

    json root;
    root["data"] = child;
    httplib::Result result = httpclient.Get(api);
    if (result && result->status == httplib::OK_200) {
        auto jsonRes = nlohmann::json::parse(result->body);
        int code = jsonRes["code"];
        if (code != 0) {
            root["result"] = -1;
            root["msg"] = jsonRes["msg"];
        } else {
            root["result"] = 0;
            root["msg"] = "success";
        }
    }

    std::string payload = root.dump();

    SendMsg("web-message", payload.c_str(), payload.size());

    printf("OnStopRtspPreview ----\n");
}

static void messageArrived(MQTT::MessageData& md) {
    printf("messageArrived ++++\n");

    MQTT::Message &message = md.message;
    LOG_MM_D(MQTT_CLIENT, "Message %d arrived: qos %d, retained %d, dup %d, packetid %d.",
            ++arrivedcount, message.qos, message.retained, message.dup, message.id);
    LOG_MM_D(MQTT_CLIENT, "Payload %.*s.", (int)message.payloadlen, (char*)message.payload);

    std::string recv_msg((char *)message.payload, message.payloadlen);
    printf("recv msg %s\n", recv_msg.c_str());

    auto jsonRes = nlohmann::json::parse(recv_msg);
    std::string type = jsonRes["type"];
    printf("msg type %s\n", type.c_str());

    if (type == "login") {
        std::string account  = jsonRes["account"];
        std::string password = jsonRes["password"];
        OnLogin(account, password);
    } else if (type == "rebootAiBox") {
        OnRebootAiBox();
    } else if (type == "restartMasterApp") {
        OnRestartMasterApp();
    } else if (type == "getDashBoardInfo") {
        OnGetDashBoardInfo();
    } else if (type == "getMediaChannelInfo") {
        OnGetMediaChannelInfo();
    } else if (type == "setMediaChannelInfo") {
        int channelId  = jsonRes["channelId"];
        std::string streamUrl = jsonRes["streamUrl"];
        OnSetMediaChannelInfo(channelId, streamUrl);
    } else if (type == "delMediaChannelInfo") {
        int channelId  = jsonRes["channelId"];
        OnDelMediaChannelInfo(channelId);
    } else if (type == "getMediaChannelTaskInfo") {
        OnGetMediaChannelTaskInfo();
    } else if (type == "getAiModelList") {
        OnGetAiModelList();
    } else if (type == "switchChannelAiModel") {
        OnSwitchChannelAiModel();
    }  else if (type == "startRtspPreview") {
        std::string streamUrl = jsonRes["streamUrl"];
        OnStartRtspPreview(streamUrl);
    } else if (type == "stopRtspPreview") {
        std::string key = jsonRes["key"];
        OnStopRtspPreview(key);
    } else if (type == "gb28181-discovery") { // add gb28181 stream
        std::string deviceId = jsonRes["deviceId"];
        std::string streamUrl = jsonRes["streamUrl"];
        gb28181_discovery_[deviceId] = streamUrl;
    } else if (type == "gb28181b-bye") { // delete gb28181 stream
        std::string deviceId = jsonRes["deviceId"];
        if (gb28181_discovery_.count(deviceId) > 0) {
            gb28181_discovery_.erase(deviceId);
        }
    }

    printf("messageArrived ----\n");
}

AX_VOID MqttClientServer::BindTransfer(CTransferHelper* pInstance) {
    m_pTransferHelper = pInstance;
}

//回调不能出现耗时过久
AX_BOOL MqttClientServer::OnRecvData(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, AX_VOID* pData) {
    if (E_OBS_TARGET_TYPE_JENC == eTarget) {
        AX_VENC_PACK_T* pVencPack = &((AX_VENC_STREAM_T*)pData)->stPack;
        if (nullptr == pVencPack->pu8Addr || 0 == pVencPack->u32Len) {
            LOG_M_E(MQTT_CLIENT, "Invalid Jpeg data(chn=%d, buff=0x%08X, len=%d).", nChn, pVencPack->pu8Addr, pVencPack->u32Len);
            return AX_FALSE;
        }

        QUEUE_T jpg_info;
        jpg_info.jpg_buf = new AX_U8[MAX_BUF_LENGTH];
        jpg_info.buf_length = pVencPack->u32Len;
        memcpy(jpg_info.jpg_buf, pVencPack->pu8Addr, jpg_info.buf_length);

        auto &tJpegInfo = jpg_info.tJpegInfo;
        tJpegInfo.tCaptureInfo.tHeaderInfo.nSnsSrc = nGrp;
        tJpegInfo.tCaptureInfo.tHeaderInfo.nChannel = nChn;
        tJpegInfo.tCaptureInfo.tHeaderInfo.nWidth = 1920;
        tJpegInfo.tCaptureInfo.tHeaderInfo.nHeight = 1080;
        CElapsedTimer::GetLocalTime(tJpegInfo.tCaptureInfo.tHeaderInfo.szTimestamp, 16, '-', AX_FALSE);

        arrjpegQ->Push(jpg_info);
    }

    return AX_TRUE;
}

/* TODO: need web support file copy, then show in web*/
AX_BOOL MqttClientServer::SaveJpgFile(AX_VOID* data, AX_U32 size, JPEG_DATA_INFO_T* pJpegInfo) {
    std::lock_guard<std::mutex> guard(m_mtxConnStatus);

    /* Data file parent directory format: </XXX/DEV_XX/YYYY-MM-DD> */
    AX_CHAR szDateBuf[16] = {0};
    CElapsedTimer::GetLocalDate(szDateBuf, 16, '-');

    AX_CHAR szDateDir[128] = {0};
    sprintf(szDateDir, "%s/DEV_%02d/%s", ALARM_IMG_PATH, pJpegInfo->tCaptureInfo.tHeaderInfo.nSnsSrc + 1, szDateBuf);

    if (CDiskHelper::CreateDir(szDateDir, AX_FALSE)) {
        sprintf(pJpegInfo->tCaptureInfo.tHeaderInfo.szImgPath, "%s/%s.jpg", szDateDir, pJpegInfo->tCaptureInfo.tHeaderInfo.szTimestamp);

        // Open file to write
        std::ofstream outFile(pJpegInfo->tCaptureInfo.tHeaderInfo.szImgPath, std::ios::binary);
        if (!outFile) {
            std::cerr << "Failed to open file for writing: " << pJpegInfo->tCaptureInfo.tHeaderInfo.szImgPath << std::endl;
            return AX_FALSE;
        }

        // Write the actual data to the file
        outFile.write(reinterpret_cast<const char*>(data), size);
        outFile.close();

        LOG_MM_C(MQTT_CLIENT, ">>>> Save jpg file: %s <<<<", pJpegInfo->tCaptureInfo.tHeaderInfo.szImgPath);
    } else {
        LOG_MM_E(MQTT_CLIENT, "[%d][%d] Create date(%s) directory failed.", pJpegInfo->tCaptureInfo.tHeaderInfo.nSnsSrc, pJpegInfo->tCaptureInfo.tHeaderInfo.nChannel, szDateDir);
    }

    return AX_TRUE;
}

AX_VOID MqttClientServer::SendAlarmMsg(MQTT::Message &message) {
    AX_U32 nCount = arrjpegQ->GetCount();
    if (nCount > 0) {
        QUEUE_T jpg_info;
        if (arrjpegQ->Pop(jpg_info, 0)) {
            SaveJpgFile(jpg_info.jpg_buf, jpg_info.buf_length, &(jpg_info.tJpegInfo));

            std::string currentTimeStr;
            MqttGetSystime(currentTimeStr);

            AX_CHAR szDateBuf[16] = {0};
            CElapsedTimer::GetLocalDate(szDateBuf, 16, '-');

            AX_CHAR szDateDir[128] = {0};
            sprintf(szDateDir, "%s/DEV_%02d/%s", ALARM_IMG_PATH, jpg_info.tJpegInfo.tCaptureInfo.tHeaderInfo.nSnsSrc + 1, szDateBuf);

            AX_CHAR szImgPath[256] = {0};
            sprintf(szImgPath, "%s/%s.jpg", szDateDir, jpg_info.tJpegInfo.tCaptureInfo.tHeaderInfo.szTimestamp);

            json child = {
                {"type", "alarmMsg"},    {"BoardId", "YJ-AIBOX-001"}, {"Time", currentTimeStr},
                {"AlarmType", "people"}, {"AlarmStatus", "success"},  {"AlarmContent", "alarm test test ..."},
                {"Path", szImgPath},  // jpg_info.tJpegInfo.tCaptureInfo.tHeaderInfo.szImgPath},
                {"channleId", 1},     // jpg_info.tJpegInfo.tCaptureInfo.tHeaderInfo.nChannel},
            };

            json root;
            root["result"] = 0;
            root["msg"] = "success";
            root["data"] = child;

            std::string payload = root.dump();

            SendMsg("web-message", payload.c_str(), payload.size());
        }
    }
}

AX_BOOL MqttClientServer::Init(MQTT_CONFIG_T &mqtt_config) {
    ipstack_ = std::make_unique<IPStack>();
    client_ = std::make_unique<MQTT::Client<IPStack, Countdown>>(*ipstack_);
    topic = mqtt_config.topic;

    //实现加锁队列，主要是多线程
    arrjpegQ = std::make_unique<CAXLockQ<QUEUE_T>>();
    if (!arrjpegQ) {
        LOG_MM_E(MQTT_CLIENT, "alloc queue fail");
        return AX_FALSE;
    } else {
        arrjpegQ->SetCapacity(10);
    }

    LOG_M_C(MQTT_CLIENT, "Mqtt Version is %d, topic is %s", mqtt_config.version, topic.c_str());
    LOG_M_C(MQTT_CLIENT, "Connecting to %s:%d", mqtt_config.hostname.c_str(), mqtt_config.port);

    int rc = ipstack_->connect(mqtt_config.hostname.c_str(), mqtt_config.port);
	if (rc != 0) {
        LOG_M_E(MQTT_CLIENT, "rc from TCP connect fail, rc = %d", rc);
    } else {
        MQTTPacket_connectData data = MQTTPacket_connectData_initializer;       
        data.MQTTVersion = mqtt_config.version;
        data.clientID.cstring = mqtt_config.client_name.c_str();
        data.username.cstring = mqtt_config.client_name.c_str();
        data.password.cstring = mqtt_config.client_passwd.c_str();
        rc = client_->connect(data);
        if (rc != 0) {
            LOG_M_E(MQTT_CLIENT, "rc from MQTT connect fail, rc is %d\n", rc);
        } else {
            LOG_M_D(MQTT_CLIENT, "MQTT connected sucess");
            rc = client_->subscribe(topic.c_str(), MQTT::QOS0, messageArrived);   
            if (rc != 0)
                LOG_M_E(MQTT_CLIENT, "rc from MQTT subscribe is %d\n", rc);
        }
    }

    return AX_TRUE;
}

AX_BOOL MqttClientServer::DeInit(AX_VOID) {

    int rc = client_->unsubscribe(topic.c_str());
    if (rc != 0)
        LOG_M_E(MQTT_CLIENT, "rc from unsubscribe was %d", rc);

    rc = client_->disconnect();
    if (rc != 0)
        LOG_M_E(MQTT_CLIENT, "rc from disconnect was %d", rc);

    ipstack_->disconnect();

    LOG_M_C(MQTT_CLIENT, "Finishing with messages received");
    
    return AX_TRUE;
}

AX_BOOL MqttClientServer::Start(AX_VOID) {

    if (!m_threadWork.Start([this](AX_VOID* pArg) -> AX_VOID { WorkThread(pArg); }, nullptr, "MQTT_CLIENT")) {
        LOG_MM_E(MQTT_CLIENT, "Create ai switch thread fail.");
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL MqttClientServer::Stop(AX_VOID) {
    if (arrjpegQ) {
        arrjpegQ->Wakeup();
    }

    m_threadWork.Stop();
    m_threadWork.Join();

    return AX_TRUE;
}

AX_VOID MqttClientServer::WorkThread(AX_VOID* pArg) {
    LOG_MM_I(MQTT_CLIENT, "+++");

    while (m_threadWork.IsRunning()) {
        //process alarm message
        MQTT::Message alarm_message;
        SendAlarmMsg(alarm_message);

        client_->yield(20 * 1000UL); // sleep 1 seconds
    }

    LOG_MM_I(MQTT_CLIENT, "---");
}
