#pragma once

class hls_array : public parameter {

public:
	hls_array(string name, string function, int range_val);

};

typedef list<hls_array> array_list;
