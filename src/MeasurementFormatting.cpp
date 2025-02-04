#include <Sensirion_UPT_Core.h>

static void defaultMeasurementToMessage(Measurement m, char * msgBuffer){
    sprintf(msgBuffer, R"({"time_offset_ms": %lu, "value": %.4f})", m.dataPoint.t_offset,m.dataPoint.value);
}

static void defaultMeasurementToTopicSuffix(Measurement m, char * topicBuffer){
    const char* sensorFamily_ =
        deviceLabel(m.metaData.platform, m.metaData.deviceType);
    const char* sensorName_ =
        deviceLabel(m.metaData.platform, m.metaData.deviceType);
    const uint64_t sensorID_ = m.metaData.deviceID;
    const char* physQty_ = quantityOf(m.signalType);

    sprintf(topicBuffer,"%s/%s/%" PRIu64 "/%s/%s", sensorFamily_, sensorName_, sensorID_,
            physQty_, sensorFamily_);
}