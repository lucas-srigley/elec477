#include <iostream>
#include <map>
#include <cmath>
#include <string>
#include <thread>
#include <chrono>

#include "dds/dds.hpp"
#include "statekey.hpp"
#include "transfer.hpp"

using namespace std;
using namespace org::eclipse::cyclonedds;

static const uint32_t DOMAIN_ID = 7;
static const double AIRPORT_LAT = 43.6777;
static const double AIRPORT_LON = -79.6248;
static const double METRES_PER_NM = 1852.0;
static const double AD_RADIUS_M = 8.0 * METRES_PER_NM;
static const float AD_CEILING_FT = 2500.0f;

static double haversine(double lat1, double lon1, double lat2, double lon2)
{
    const double R = 6371000.0;
    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;
    double a = sin(dLat/2)*sin(dLat/2) +
               cos(lat1*M_PI/180.0)*cos(lat2*M_PI/180.0)*
               sin(dLon/2)*sin(dLon/2);
    return R * 2.0 * atan2(sqrt(a), sqrt(1.0-a));
}

static inline float metresToFeet(float m) { return m * 3.28084f; }

struct AircraftState {
    double lastDist = -1.0;
    float lastAlt = -1.0;
    bool handedOff = false;
};

int main(int argc, char* argv[])
{
    dds::domain::DomainParticipant participant(DOMAIN_ID);

    dds::topic::Topic<State::Update> flightTopic(participant, "Flights");
    dds::topic::Topic<Transfer::Handoff> handoffTopic(participant, "Handoffs");

    dds::pub::Publisher handoffPub(participant);
    dds::pub::DataWriter<Transfer::Handoff> handoffWriter(handoffPub, handoffTopic);

    dds::sub::Subscriber sub(participant);
    dds::sub::DataReader<State::Update> flightReader(sub, flightTopic);
    dds::sub::DataReader<Transfer::Handoff> handoffReader(sub, handoffTopic);

    cout << "Centre: waiting for data" << endl;

    map<string, AircraftState> aircraft;

    while (true) {
        auto handoffSamples = handoffReader.take();
        for (auto it = handoffSamples.begin(); it != handoffSamples.end(); ++it) {
            if (!it->info().valid()) continue;
            if (it->data().destination() != Transfer::Zone::TORONTO_CENTRE) continue;
            aircraft[it->data().callsign()].handedOff = false;
            cout << "Centre: received handoff for " << it->data().callsign() << endl;
        }

        auto flightSamples = flightReader.take();
        for (auto it = flightSamples.begin(); it != flightSamples.end(); ++it) {
            if (!it->info().valid()) continue;
            const State::Update& msg = it->data();

            double dist = haversine(msg.lat(), msg.lon(), AIRPORT_LAT, AIRPORT_LON);
            float alt = metresToFeet(msg.baroaltitude());
            AircraftState& st = aircraft[msg.callsign()];

            if (!st.handedOff) {
                if (st.lastDist > 0 && alt <= AD_CEILING_FT
                    && st.lastDist > AD_RADIUS_M && dist <= AD_RADIUS_M) {
                    cout << "Centre: handoff to AD (8nm) " << msg.callsign() << endl;
                    Transfer::Handoff h(msg.callsign(), msg.icao24(),
                        Transfer::Zone::TORONTO_CENTRE, Transfer::Zone::TORONTO_AD,
                        msg.timestamp());
                    handoffWriter.write(h);
                    st.handedOff = true;
                } else if (st.lastAlt > 0 && dist <= AD_RADIUS_M
                           && st.lastAlt > AD_CEILING_FT && alt <= AD_CEILING_FT) {
                    cout << "Centre: handoff to AD (2500ft) " << msg.callsign() << endl;
                    Transfer::Handoff h(msg.callsign(), msg.icao24(),
                        Transfer::Zone::TORONTO_CENTRE, Transfer::Zone::TORONTO_AD,
                        msg.timestamp());
                    handoffWriter.write(h);
                    st.handedOff = true;
                }
            }

            st.lastDist = dist;
            st.lastAlt = alt;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return 0;
}