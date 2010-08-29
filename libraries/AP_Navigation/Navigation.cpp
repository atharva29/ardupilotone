#include "Navigation.h"

Navigation::Navigation(AP_GPS *withGPS) : 
		_gps(withGPS),
		_hold_course(-1)
{
}

void
Navigation::update_gps()
{
	location.alt 		= _gps->altitude;
	location.lng 		= _gps->longitude;
	location.lat 		= _gps->lattitude;
	
	// target_bearing is where we should be heading 
	bearing 			= get_bearing(&location, &next_wp);

	// waypoint distance from plane
	distance 			= get_distance(&location, &next_wp);
	
	calc_bearing_error();
	calc_altitude_error();
	altitude_above_home = location.alt - home.alt;
			
	// check if we have missed the WP
	_loiter_delta 		= (bearing - _old_bearing) / 100;
	
	// reset the old value
	_old_bearing 		= bearing;
	
	// wrap values
	if (_loiter_delta > 170) _loiter_delta -= 360;
	if (_loiter_delta < -170) _loiter_delta += 360;
	loiter_sum += abs(_loiter_delta);

	if (distance <= 0){
		distance = -1;
		Serial.print("MSG wp error  ");
		Serial.println(distance, DEC);
	}
}

void
Navigation::set_home(Waypoints::WP loc)
{
	home 		= loc;
	location 	= home;
}

void
Navigation::set_next_wp(Waypoints::WP loc)
{
	prev_wp = next_wp;
	next_wp = loc;
	
	if(_scaleLongDown == 0)
	calc_long_scaling(loc.lat);

	total_distance 	= get_distance(&location, &next_wp);
	distance			= total_distance;
	
	bearing 			= get_bearing(&location, &next_wp);
	_old_bearing 		= bearing;

	// clear loitering code
	_loiter_delta 		= 0;
	loiter_sum 			= 0;	

	// set a new crosstrack bearing
	// ----------------------------
	reset_crosstrack();
}
void 
Navigation::calc_long_scaling(int32_t lat)
{
	// this is used to offset the shrinking longitude as we go towards the poles	
	float rads 			= (abs(lat) / T7) * 0.0174532925;
	_scaleLongDown 		= cos(rads);
	_scaleLongUp 		= 1.0f / cos(rads);
}

void
Navigation::hold_course(int8_t b)
{
	if(b)
		_hold_course = bearing;
	else
		_hold_course = -1;
}

void
Navigation::calc_distance_error()
{	
	//distance_estimate 	+= (float)_gps->ground_speed * .0002 * cos(radians(bearing_error * .01));
	//distance_estimate 	-= DST_EST_GAIN * (float)(distance_estimate - GPS_distance);
	//distance  		= max(distance_estimate,10);
}

void 
Navigation::calc_bearing_error(void)
{
	if(_hold_course == -1){
		bearing_error = wrap_360(bearing - _gps->ground_course);
	}else{
		bearing_error = _hold_course;
	}
}

int32_t
Navigation::wrap_360(int32_t error)
{
	if (error > 18000)	error -= 36000;
	if (error < -18000)	error += 36000;
	return error;
}

/*****************************************
 * Altitude error with Airspeed correction
 *****************************************/
void
Navigation::calc_altitude_error(void) 
{
	// limit climb rates
	_target_altitude = next_wp.alt - ((float)((distance -20) * _offset_altitude) / (float)(total_distance - 20));
	if(prev_wp.alt > next_wp.alt){
		_target_altitude = constrain(_target_altitude, next_wp.alt, prev_wp.alt);
	}else{
		_target_altitude = constrain(_target_altitude, prev_wp.alt, next_wp.alt);
	}
	altitude_error 	= _target_altitude - location.alt;
}

void
Navigation::update_crosstrack(void)
{
	// Crosstrack Error
	// ----------------
	if (abs(bearing - _crosstrack_bearing) < 4500) {	 // If we are too far off or too close we don't do track following
		_crosstrack_error = sin(radians((bearing - _crosstrack_bearing) / 100)) * distance;	 // Meters we are off track line
		bearing += constrain(_crosstrack_error * XTRACK_GAIN, -XTRACK_ENTRY_ANGLE, XTRACK_ENTRY_ANGLE);
	}
}

void
Navigation::reset_crosstrack(void)
{
	_crosstrack_bearing 	= get_bearing(&location, &next_wp);	// Used for track following
}

long
Navigation::get_distance(Waypoints::WP *loc1, Waypoints::WP *loc2)
{
	if(loc1->lat == 0 || loc1->lng == 0) 
		return -1;
	if(loc2->lat == 0 || loc2->lng == 0) 
		return -1;
	if(_scaleLongDown == 0)
		calc_long_scaling(loc2->lat);
	float dlat 	= (float)(loc2->lat - loc1->lat);
	float dlong = ((float)(loc2->lng - loc1->lng)) * _scaleLongDown;
	return sqrt(sq(dlat) + sq(dlong)) * .01113195;
}

long 
Navigation::get_bearing(Waypoints::WP *loc1, Waypoints::WP *loc2)
{
	if(loc1->lat == 0 || loc1->lng == 0) 
		return -1;
	if(loc2->lat == 0 || loc2->lng == 0) 
		return -1;
	if(_scaleLongDown == 0)
		calc_long_scaling(loc2->lat);
	long off_x 			= loc2->lng - loc1->lng;
	long off_y 			= (loc2->lat - loc1->lat) * _scaleLongUp;
	long bearing 		=	9000 + atan2(-off_y, off_x) * 5729.57795;
	if (bearing < 0) 
		bearing 		+= 36000;
	return bearing;
}