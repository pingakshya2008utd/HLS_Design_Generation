#pragma once

class loop : public parameter {

public:
	loop(string name, string function, int range_val);
};

typedef list<loop> loop_list;
