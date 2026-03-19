#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <thread>

#include "dds/dds.hpp"
#include "statekey.hpp"

using namespace std;
using namespace org::eclipse::cyclonedds;

static const uint32_t DOMAIN_ID = 7;

static void rtrim(string& s)
{
    s.erase(find_if(s.rbegin(), s.rend(), [](unsigned char c) {
        return !isspace(c);
    }).base(), s.end());
}

int main(int argc, char* argv[]) {
    dds::domain::DomainParticipant participant(DOMAIN_ID);

    dds::topic::Topic<State::Update> topic(participant, "Flights");

    dds::sub::qos::DataReaderQos drQos;
    drQos << dds::core::policy::History::KeepLast(1);

    dds::sub::Subscriber subscriber(participant);
    dds::sub::DataReader<State::Update> reader(subscriber, topic, drQos);

    bool running = true;

    dds::core::cond::WaitSet bgWaitset;
    dds::core::cond::StatusCondition bgCond(reader);
    bgCond.enabled_statuses(dds::core::status::StatusMask::data_available());
    bgWaitset.attach_condition(bgCond);

    thread drainThread([&](){
        while (running) {
            try {
                bgWaitset.wait(dds::core::Duration(1, 0));
            } catch (...) {
                continue;
            }
            try { reader.read(); } catch (...) {}
        }
    });

    cout << "enter callsign or 'quit'" << endl;

    string input;
    while (true) {
        cout << "> ";
        if (!getline(cin, input))
            break;
        rtrim(input);
        if (input.empty())
            continue;
        if (input == "quit" || input == "exit")
            break;

        vector<string> params = { "'" + input + "'" };

        dds::sub::cond::QueryCondition qc(reader, "callsign = %0", params, dds::sub::status::DataState::any());

        auto samples = reader.select().content(qc).read();

        bool found = false;
        for (auto it = samples.begin(); it != samples.end(); ++it) {
            if (!it->info().valid())
                continue;
            const State::Update& msg = it->data();
            found = true;
            cout << "callsign: " << msg.callsign() << endl;
            cout << "icao24: " << msg.icao24() << endl;
            cout << "timestamp: " << msg.timestamp() << endl;
            cout << "lat/lon: " << msg.lat() << " / " << msg.lon() << endl;
            cout << "velocity: " << msg.velocity() << " m/s" << endl;
            cout << "heading: " << msg.heading() << " deg" << endl;
            cout << "vert rate: " << msg.vertrate() << " m/s" << endl;
            cout << "squawk: " << msg.squawk() << endl;
            cout << "baro alt: " << msg.baroaltitude() << " m" << endl;
            cout << "geo alt: " << msg.geoaltitude() << " m" << endl;
            break;
        }

        if (!found)
            cout << "no data for " << input << endl;
    }

    running = false;
    drainThread.join();
    return 0;
}