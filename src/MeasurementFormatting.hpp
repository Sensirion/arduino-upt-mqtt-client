#include <Sensirion_UPT_Core.h>
#include <iostream>   
#include <sstream>    

namespace sensirion::upt::mqtt
{

    struct DefaultMeasurementFormatter
    {
        std::string operator () (const sensirion::upt::core::Measurement& m){
            std::stringstream stream{};
            stream.precision(4);
            stream << "{\"time_offset_ms\":";
            stream << m.dataPoint.t_offset;
            stream << ", \"value\":";
            stream << m.dataPoint.value;
            stream << "}";
            return stream.str();
        }
    };

    struct FullMeasurementFormatter
    {
        std::string operator () (const sensirion::upt::core::Measurement& m){
            std::stringstream stream{};
            stream.precision(4);
            stream << "{\"time_offset_ms\":";
            stream << m.dataPoint.t_offset;
            stream << ", \"value\":";
            stream << m.dataPoint.value;
            stream << ", \"device_id\":";
            stream << m.metaData.deviceID;
            stream << ", \"device_type\":";
            stream << m.metaData.deviceType;
            stream << ", \"signal\":";
            stream << quantityOf(m.signalType);
            stream << ", \"signal_unit\":";
            stream << unitOf(m.signalType);
            stream << "}";
            return stream.str();
        }
    };

    struct DefaultMeasurementToTopicSuffix{
        std::string operator() (const sensirion::upt::core::Measurement& m){
            
            std::stringstream stream{};
            stream << core::deviceLabel( m.metaData.deviceType); 
            stream << " " << m.metaData.deviceID 
                   << " " << sensirion::upt::core::quantityOf(m.signalType);
            return stream.str();
        }
    };
    
    struct MeasurementToTopicSuffixTree{
        std::string operator() (const sensirion::upt::core::Measurement& m){
            
            std::stringstream stream{};
            stream << core::deviceLabel(m.metaData.deviceType); 
            stream << "/" << m.metaData.deviceID 
                   << "/" << sensirion::upt::core::quantityOf(m.signalType);
            return stream.str();
        }
    };

    struct MeasurementToTopicSuffixEmpty{
        std::string operator() (const sensirion::upt::core::Measurement& m){
            return "";
        }
    };

} // end namespace
