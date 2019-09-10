class func : public parameter {
	vector<string> argument_names;

public:
	func(string name,vector<string> args);
	vector<string> get_argument_names();

};

typedef list<func> func_list;
