class dse {
	friend class generate_ds;
	vector <int> loop_factor = { 0,2,4,8,16,20 };
	vector <int> array_factor = {2,4,6,8};
	vector <string> partition_type = { "block","cyclic", "complete" };
	vector <string> inline_options = { "","off", "recursive" };

public:
	
	void start_dse(generate_ds ds1);
	void shuffle_list(loop_list &loop_names, array_list &array_names, func_list &function_names);

};
#pragma once
