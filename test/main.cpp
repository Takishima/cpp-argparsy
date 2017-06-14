/* 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 * Authors:
 * 2017 Damien Nguyen <damien.nguyen@alumni.epfl.ch>
 */

#include "program_options.hpp"

#include <string>
#include <vector>

enum some_type {
     ONE = 1,
     TWO = 2,
};

void print(const std::vector<std::string>& v)
{
     for (unsigned int i(0); i < v.size(); ++i) {
	  std::cout << " " << v[i];
     }
}

int main(int argc, char *argv[])
{
     bool b(false);
     unsigned int l(0), count(0);
     std::vector<int> v;
     some_type st;
     std::vector<std::string> values;
     
     ProgramOptionManager args(argv[0], "This is a demonstration program");
     args.add_option("count", count, "a number");
     args.add_option("pos2", values, count_depends_on("count"), "a multiple-valued positional");
     
     args.add_option("u", "uint", l, "an argument with a single");
     args.add_option("v", "vector", v, 3, "an argument with 3 values");
     args.add_option("b", "bool", b, "a boolean flag");
     args.add_option("O", "ONE", st, ONE, "an flag with specific value ONE");
     args.add_option("T", "TWO", st, TWO, "an flag with specific value TWO");

     int retval = args.process_arguments(argc, argv);
     if (retval <= 0) {
	  return retval;
     }

     if (v.empty()) {
	  v.push_back(0);
	  v.push_back(0);
	  v.push_back(0);
     }

     if (v.size() != 3) {
	  std::cerr << "ERROR: size of v should be 3!\n";
	  return -1;
     }

     std::cout << "count = " << count << std::endl
	       << "values =";
     print(values);
     std::cout << std::endl
	       << "b = " << std::boolalpha << b << std::endl
	       << "l = " << l << std::endl
	       << "v = " << v[0] << " " << v[1] << " " << v[2] << std::endl
	<< "st = " << st << std::endl
     << std::endl;

     return 0;
}
