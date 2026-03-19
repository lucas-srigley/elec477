
#include <cstdlib>
#include <iostream>
#include <chrono>
#include <thread>

#include "dds/dds.hpp"

#include "statekey.hpp"

using namespace std;
using namespace org::eclipse::cyclonedds;


// set this to your team number
uint32_t domainID = 0;
char * programName;

int main(int argc, char * argv[]) {
    int count = 0;
    
    programName = argv[0];
    
    // create the main DDS entities Participant, Topic, Subscriber and DataReader
    dds::domain::DomainParticipant participant(domainID);
    dds::topic::Topic<State::Update> topic(participant, "Flights");
    dds::sub::Subscriber subscriber(participant);
    dds::sub::DataReader<State::Update> reader(subscriber, topic);

    // set up waitsets. The conditions are that dat is availalbe, or there has been
    // a change in the numbe of publishers that are matched with our subscription.
    dds::core::cond::WaitSet waitset;
    dds::core::cond::StatusCondition rsc(reader);
    rsc.enabled_statuses(dds::core::status::StatusMask::data_available()| dds::core::status::StatusMask::subscription_matched());
    waitset.attach_condition(rsc);
    
    while(1){
        try{
            // wait for more data or for the publisher to end.
	    waitset.wait(dds::core::Duration::infinite());
	} 
	catch (const dds::core::Exception &e){
            cerr << programName << ": Subscriber excption while waiting for data: \"" << e.what() << "\"." << endl;
            break;
	}
        
        // if the publisher is gone, exit.
        if (reader.subscription_matched_status().current_count() == 0) {
            break;
        }
        
        // take the samples and print them
	dds::sub::LoanedSamples<State::Update> samples;

	samples = reader.take();

	if (samples.length() > 0){
	    dds::sub::LoanedSamples<State::Update>::const_iterator sample_iter;
	    for (sample_iter = samples.begin(); sample_iter < samples.end(); ++sample_iter) {
		const State::Update& msg = sample_iter->data();
		const dds::sub::SampleInfo& info = sample_iter->info();
                // note not all samples may be valid.
		if (info.valid()) {
		    cout << "**** subscriber received: " << ++count << endl;
		    cout << "    icao24  : " << msg.icao24() << endl;
		    cout << "    callsign : \"" << msg.callsign() << "\"" << endl;
		    cout << "    timestamp : \"" << msg.timestamp() << "\"" << endl;
		}

	    }
	} else {
	    cout << programName << ": no samples?" << endl;
	}
    }

}
