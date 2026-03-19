#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

using namespace std;

class Aircraft
{
    public:
        time_t time = 0;
	string icao24 = "";
	double lat = 0.0;
	double lon = 0.0;
	float velocity = 0.0;
	float heading = 0.0;
	float vertrate = 0.0;
	string callsign = "";
	bool onground = false;
	bool alert = false;
	bool spi = false;
	int squawk = 0;
	float baroaltitude = 0.0;
	float geoaltitude = 0.0;
	time_t lastposupdate = 0;
	time_t lastcontact = 0; 

        void readNextRow(istream& str);

    private:
        string         m_line;
	void initVals();
};

istream& operator>>(istream& str, Aircraft& data);
