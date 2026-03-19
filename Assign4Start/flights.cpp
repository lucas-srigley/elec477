#include <iterator>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <map>

#include "aircraft.hpp"

#include "dds/dds.hpp"
#include "statekey.hpp"


using namespace std;
using namespace org::eclipse::cyclonedds;

// set this to your team number
uint32_t domainID = 0;
char * programName;

int main(int argc, char * argv[])
{
    time_t lastUsedTime = 0;
    const string fileName = "../Toronto_2202-06-27-12.csv";
    ifstream file(fileName);
    Aircraft aircraft;
    string headings; // variable to read the first line of the CSV file
    map<string,dds::core::InstanceHandle> allAircraft;
    
    // save program name
    programName = argv[0];
    
    // check that file opened correctly.
    if (file.fail()){
        cerr << programName << ": could not open file " << fileName << endl;
        exit(1);
    }
    
    // create the main DDS entities Participant, Topic, Publisher and DataWriter
    cout << programName << ": Domain is " << domainID << endl;
    dds::domain::DomainParticipant participant(domainID);
    dds::topic::Topic<State::Update> topic(participant, "Flights");
    dds::pub::Publisher publisher(participant);
    dds::pub::DataWriter<State::Update> writer(publisher, topic);

    // Type variable to use when writing
    State::Update msg(
        "abc",  // icao24
        0,	// timestamp.
	0.0,    // lat
	0.0,    // lon
	0.0,    // vel
	0.0,    // heading
	0.0,    // vertrat
	"abc",  // calsign
	7400,   // squawk
	0.0,	// baroaltitude
	0.0);   // geoaltigude

    // wait for subscriber before we start the loop.
    cout << programName << ": waiting for subscriber to start" << endl;
    dds::core::cond::WaitSet waitset;
    dds::core::cond::StatusCondition wsc(writer);
    wsc.enabled_statuses(dds::core::status::StatusMask::publication_matched());
    waitset.attach_condition(wsc);
    try{
        waitset.wait(dds::core::Duration::infinite());
    }
    catch (const dds::core::Exception &e){
        cerr << programName << ": encountered an exception while waiting for subscriber: \"" << e.what() << "\"" << endl;
        exit(1);
    }
    
    //
    // Process the csv file line by line
    //
    
    // first line is the column headings, discard them.
    getline(file,headings);

    // aircraft is a helper class that knows how to read
    // the columns in the CSV file.
    
    cout << programName << ": starting the data loop" << endl;
    
    int count = 0;
    while(file >> aircraft)
    {
        if (lastUsedTime == 0){
           // if it the first aircraft, we don't have a last used time
           // to get the real time intervals right.
           lastUsedTime = aircraft.time;
        }
        
        if (aircraft.time > lastUsedTime){
            // if the timestamp of the current aircraft is greater than the current time
            // then we have to wait until the current time catches up.
            // in practice there are groups of samples several seconds apart in the data file.
            
            // for fast testing
            //std::this_thread::sleep_for(std::chrono::milliseconds(20));
            std::this_thread::sleep_for(std::chrono::seconds(aircraft.time - lastUsedTime));

        }
        
        // trace output
        cout << "Count " << count++ << endl;
        cout << "time " << aircraft.time << endl;
        cout << "icao24 " << aircraft.icao24 << endl;
        cout << "callsign " << aircraft.callsign << endl;
        
        // transfer data from helper class to message class
    	msg.timestamp(aircraft.time);
    	msg.icao24(aircraft.icao24);
	msg.lat(aircraft.lat);
	msg.lon(aircraft.lon);
	msg.velocity(aircraft.velocity);
	msg.heading(aircraft.heading);
	msg.vertrate(aircraft.vertrate);
	msg.callsign(aircraft.callsign);
	msg.squawk(aircraft.squawk);
	msg.baroaltitude(aircraft.baroaltitude);
	msg.geoaltitude(aircraft.geoaltitude);
        
        // explictly manage instances.
        dds::core::InstanceHandle h;
        map<string,dds::core::InstanceHandle>::iterator it = allAircraft.find(aircraft.callsign);
        if (it == allAircraft.end()){
            // not there
            h = writer.register_instance(msg);
            // need to add to the map.
            allAircraft[aircraft.callsign] = h;
        } else {
            h = it->second;
        }

        // send the sample
	writer.write(msg,h);
        
        // only needed for unkeyed version.
        //std::this_thread::sleep_for(std::chrono::milliseconds(20));
        
        // update the time stamp of the last aircraft
        lastUsedTime = aircraft.time;
        
        // limit number of updates for testing
        //if (count > 100){
        //    break;
        //}
    }
    
    // unregister all of the aircraft
    map<string,dds::core::InstanceHandle>::iterator it;
    for (it = allAircraft.begin(); it != allAircraft.end(); it++){
        cout << "unregistering flight " << it->first << endl;
        dds::core::InstanceHandle handle = it->second;
        writer.unregister_instance(handle);
    }

    // done
    file.close();
}
