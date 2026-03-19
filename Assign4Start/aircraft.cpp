#include "aircraft.hpp"

// taken from https://stackoverflow.com/questions/216823/how-to-trim-a-stdstring
inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

void Aircraft::readNextRow(istream& str) {

    initVals();

    getline(str, m_line);

    stringstream lineStream(m_line);
    string cell;

// time,icao24,lat,lon,velocity,heading,vertrate,callsign,onground,alert,spi,squawk,baroaltitude,geoaltitude,lastposupdate,lastcontact

    // time
    if(getline(lineStream,cell,',')) {
	time = stoll(cell,nullptr,10);	        
    } 

    // icao24
    getline(lineStream,icao24,',');

    // lat
    if(getline(lineStream,cell,',')) {
	lat = stod(cell);	        
    }

    // lon
    if(getline(lineStream,cell,',')) {
	lon = stod(cell);	        
    } 

    // velocity
    if(getline(lineStream,cell,',')) {
	if (cell.length() > 0){
	    velocity = stof(cell);	        
	} 
    }

    // heading
    if(getline(lineStream,cell,',')) {
	if (cell.length() > 0){
	    heading = stof(cell);	        
	}
    } 

    // vertrate
    if(getline(lineStream,cell,',')) {
	if (cell.length() > 0){
	    vertrate = stof(cell);	        
	}
    }

// time,icao24,lat,lon,velocity,heading,vertrate,callsign,onground,alert,spi,squawk,baroaltitude,geoaltitude,lastposupdate,lastcontact

    // callsign
    getline(lineStream,callsign,',');
    rtrim(callsign);
    if (callsign.empty()) callsign = "unknown";

    // onground
    if(getline(lineStream,cell,',')) {
       if (cell.compare("True")==0){
	    onground = true;
	}
    }

    // alert
    if(getline(lineStream,cell,',')) {
       if (cell.compare("True")==0){
	    alert = true;
	}
    }

    // spi
    if(getline(lineStream,cell,',')) {
       if (cell.compare("True")==0){
	    spi = true;
	}
    }

    // squawk - acutally octal value, but we treat it as decimal for convineience
    if(getline(lineStream,cell,',')) {
	if (cell.length() > 0){
	    squawk = stol(cell);	        
	}
    }

    // baroaltitude
    if(getline(lineStream,cell,',')) {
	if (cell.length() > 0){
	    baroaltitude = stof(cell);	        
	} 
    }

    // geoaltitude
    if(getline(lineStream,cell,',')) {
	if (cell.length() > 0){
	    geoaltitude = stof(cell);	        
	} 
    }

    // lastupdate
    if(getline(lineStream,cell,',')) {
	lastposupdate = stoll(cell,nullptr,10);
    }

    // lastcontact
    if(getline(lineStream,cell,',')) {
	lastcontact = stoll(cell,nullptr,10);
    }
}

void Aircraft::initVals(){
    time = 0;
    icao24 = "";
    lat = 0.0;
    lon = 0.0;
    velocity = 0.0;
    heading = 0.0;
    vertrate = 0.0;
    callsign = "";
    onground = false;
    alert = false;
    spi = false;
    squawk = 0;
    baroaltitude = 0.0;
    geoaltitude = 0.0;
    lastposupdate = 0;
    lastcontact = 0; 
}

istream& operator>>(istream& str, Aircraft& data)
{
    data.readNextRow(str);
    return str;
}   
