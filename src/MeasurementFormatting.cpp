#include <Sensirion_UPT_Core.h>

static void defaultMeasurementToMessage(const sensirion::upt::core::Measurement& m, char * msgBuffer){
    sprintf(msgBuffer, R"({"time_offset_ms": %lu, "value": %.4f})", m.dataPoint.t_offset,m.dataPoint.value);
}

static void defaultMeasurementToTopicSuffix(const sensirion::upt::core::Measurement& m, char * topicBuffer){
    const char* sensorName_ =
        sensirion::upt::core::deviceLabel( m.metaData.deviceType);
    const uint64_t sensorID_ = m.metaData.deviceID;
    const char* physQty_ = sensirion::upt::core::quantityOf(m.signalType);

    sprintf(topicBuffer,"%s/%" PRIu64 "/%s", sensorName_, sensorID_, physQty_);
}