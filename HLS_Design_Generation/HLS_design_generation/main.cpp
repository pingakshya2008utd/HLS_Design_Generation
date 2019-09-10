#include "general.h"
#include "parameter.h"
#include "loop.h"
#include "hls_array.h"
#include "func.h"
#include "generate_ds.h"
#include "dse.h"


default_random_engine dre(chrono::steady_clock::now().time_since_epoch().count());
int random(int lim) {
	uniform_int_distribution<int> uid{ 0,lim };   // help dre to generate nos from 0 to lim (lim included);
	return uid(dre);    // pass dre as an argument to uid to generate the random no
}



loop::loop(string name, string function, int range_val) {
	parameter_name = name;
	function_name = function;
	parameter_range = range_val;
}


hls_array::hls_array(string name, string function, int range_val) {
	parameter_name = name;
	function_name = function;
	parameter_range = range_val;
}

func::func(string name, vector<string> args) {
	parameter_name = name;
	argument_names = args;
}

string parameter::get_name() {
	return parameter_name;
}

int parameter::get_range_val() {
	return parameter_range;
}


string parameter::get_func_name() {
	return function_name;
}

vector<string> func::get_argument_names() {

	return argument_names;
}


generate_ds::generate_ds(string file_name) {

	ifstream in("O:/HLS_Projects/matrix_mult_2019/loop2.lib");
	string sentence;


	while (std::getline(in, sentence)) {

		//	cout << sentence << endl;
		istringstream iss(sentence);
		vector<string> tokens{ istream_iterator<string>{iss}, istream_iterator<string>{} };

		if (tokens.size() == 0)
			continue;

		if (tokens[0] == "loop") {
			string name1 = tokens[2];
			string funct1 = tokens[1];
			int range1 = stoi(tokens[3]);
			loop loop1(name1, funct1, range1);
			loop_names.push_back(loop1);
		}

		else if (tokens[0] == "array") {
			string name1 = tokens[1];
			string funct1 = tokens[3];
			int range1 = stoi(tokens[2]);
			hls_array array1(name1, funct1, range1);
			array_names.push_back(array1);
		}

		else if (tokens[0] == "function") {
			string name1 = tokens[1];
			int vec_len = tokens.size() - 2;
			vector<string> var_names(vec_len);
			copy(tokens.begin() + 2, tokens.begin() + int(tokens.size()), var_names.begin());
			func function1(name1, var_names);
			function_names.push_back(function1);

		}

	}
}



loop_list generate_ds::print_loop_list() {

	loop_list::iterator it;

	for (it = loop_names.begin(); it != loop_names.end(); it++)
	{
		loop l1 = *it;
		cout << "name= " << l1.get_name() << endl;
	}

	return loop_names;

}




array_list generate_ds::print_array_list() {

	array_list::iterator it;

	for (it = array_names.begin(); it != array_names.end(); it++)
	{
		hls_array a1 = *it;
		cout << "name= " << a1.get_name() << endl;
	}

	return array_names;

}


func_list generate_ds::print_func_list() {

	func_list::iterator it;

	for (it = function_names.begin(); it != function_names.end(); it++)
	{
		func a1 = *it;
		cout << "name= " << a1.get_name() << endl;
	}

	return function_names;

}





void dse::shuffle_list(loop_list &loop_names, array_list &array_names, func_list &function_names) {
	vector<loop> V(loop_names.begin(), loop_names.end());
	random_shuffle(V.begin(), V.end());
	loop_names.assign(V.begin(), V.end());

	vector<hls_array> V1(array_names.begin(), array_names.end());
	random_shuffle(V1.begin(), V1.end());
	array_names.assign(V1.begin(), V1.end());

	vector<func> V2(function_names.begin(), function_names.end());
	random_shuffle(V2.begin(), V2.end());
	function_names.assign(V2.begin(), V2.end());
}


void dse::start_dse(generate_ds ds1) {

	loop_list loop_names = ds1.print_loop_list();
	array_list array_names = ds1.print_array_list();
	func_list function_names = ds1.print_func_list();
	int number_of_loop = loop_names.size();
	srand(time(0));

	loop_list::iterator it;
	array_list::iterator it_a;
	func_list::iterator it_f;
	for (int i = 0; i < 100; i++) {

		string file_path_name = "O:/HLS_Projects/matrix_mult_2019/untimate_new_directive" + to_string(i) + ".tcl";
		std::ofstream outfile(file_path_name);
		shuffle_list(loop_names, array_names, function_names);

		/*****************Loop Pragmas all designs***************************************/
		if (!loop_names.empty()) {

			int num_loop_pipeline = random(number_of_loop);
			int num_loop_unrolled = number_of_loop - num_loop_pipeline;
			srand(time(0));
			int j = 0;


			for (it = loop_names.begin(); it != loop_names.end(); ++it) {

				if (loop_names.empty())
					break;
				loop curr_loop = *it;
				int range = curr_loop.get_range_val();
				string func_name = curr_loop.get_func_name();
				string loop_name = curr_loop.get_name();

				if (j < num_loop_unrolled) {
					/*----------------------- Loop Unrolling-------------------------*/
					loop_factor.push_back(range);
					int rand_number = random(loop_factor.size() - 1);
					int unroll_factor = loop_factor[rand_number];
					int actual_unroll_fact = min(unroll_factor, range);
					cout << "loop unrol factor= " << unroll_factor << "  ";
					outfile << "set_directive_unroll -factor " << actual_unroll_fact << " \"" << func_name << "/" << loop_name << "\"" << endl;
					loop_factor.pop_back();
				}
				else {
					/*----------------------- Pipelining-------------------------*/
					srand(time(0));
					int II = random(3);
					cout << "II= " << II << " ";
					outfile << "set_directive_pipeline -II " << II << " \"" << func_name << "/" << loop_name << "\"" << endl;
				}
				j++;

			}
		}



		/********************* Array Pragmas all designs----------------------*/
		if (!array_names.empty()) {
			for (it_a = array_names.begin(); it_a != array_names.end(); ++it_a) {
				if (array_names.empty())
					break;
				hls_array curr_array = *it_a;
				int array_range = curr_array.get_range_val();
				string func_name = curr_array.get_func_name();
				string array_name = curr_array.get_name();

				srand(time(NULL));
				int rand_index = random(2);
				string part_type = partition_type[rand_index];
				int rand_number = random(array_factor.size() - 1);

				cout << "partition type= " << partition_type[rand_index] << " partiton factor= " << rand_number << endl;
				int partition_factor = min(array_range, array_factor[rand_number]);

				if (part_type == "complete") {
					outfile << "set_directive_array_reshape -type " << part_type << " -dim 1 \"" << func_name << "\" " << array_name << endl;
				}
				else {
					outfile << "set_directive_array_reshape -type " << part_type << " -factor " << partition_factor << " -dim 1 \"" << func_name << "\" " << array_name << endl;
				}
			}
		}


		/********************* Function Pragmas----------------------*/

		if (!function_names.empty()) {
			int num_of_funcs = function_names.size();
			int num_func_inline = random(num_of_funcs - 1);
			int p = 0;

			for (it_f = function_names.begin(); it_f != function_names.end(); ++it_f) {
				func curr_function = *it_f;
				string func_name = curr_function.get_name();
				vector <string> var_list = curr_function.get_argument_names();
				int len_arg_list = var_list.size();

				/*****************Function Inline**************************************/
				if (p < num_func_inline || var_list.empty()) {
					int rand_index = random(2);
					string inline_type = inline_options[rand_index];

					if (rand_index != 0)
						outfile << "set_directive_inline -" << inline_type << " \"" << func_name << "\"" << endl;
					else
						outfile << "set_directive_inline " << inline_type << " \"" << func_name << "\"" << endl;
				}
				else {

					/*****************Function Instantiate**************************************/
					int rand_index = random(len_arg_list - 1);
					string dependency_var = var_list[rand_index];
					outfile << "set_directive_function_instantiate  \"" << func_name << "\" " << dependency_var << endl;

				}

				p++;
			}
		}




		outfile.close();

	}


	//system("O:/HLS_Projects/commands_blowfish.bat");


}




int main() {

	generate_ds ds1("O:/HLS_Projects/apdcm/loop_fuct.lib");
	//ds1.print_loop_list();
	dse mydse;
	mydse.start_dse(ds1);
	system("pause");
	return 0;
}