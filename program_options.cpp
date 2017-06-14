/* 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 * Authors:
 * 2017 Damien Nguyen <damien.nguyen@alumni.epfl.ch>
 */

#include "program_options.hpp"

#include <iterator>

using internal_::OptionValueBase;
using internal_::program_option_type;

// =============================================================================

void phl_op(const OptionValueBase* p)
{
     p->print_help_line();
}
void hn_op(const OptionValueBase* p)
{
     std::cout << " " << p->help_name();
}
void sln_op(const OptionValueBase* p)
{
     std::cout << " " << p->usage_name();
}

// -----------------------------------------------------------------------------

bool hn_sort (OptionValueBase* a, OptionValueBase* b)
{
     return a->help_name() < b->help_name();
}

bool sln_sort (OptionValueBase* a, OptionValueBase* b)
{
     if (a->short_name().empty()
	 || b->short_name().empty()) {
	  return a->long_name() < b->long_name();
     }
     else {
	  return a->short_name() < b->short_name();
     }
}

// =============================================================================

class back_insert_args
{
public:
     back_insert_args(const std::vector<OptionValueBase*>& opts,
		      program_option_type& argvv)
	  : opts_(opts)
	  , argvv_(argvv)
	  {}
     
     void operator() (const std::string& arg)
	  {
	       for (unsigned int p(0); p < opts_.size(); ++p) {
		    if (opts_[p]->match_short_name(arg)
			&& arg.size() > 2) {
			 argvv_.push_back(arg.substr(0,2));
			 argvv_.push_back(arg.substr(2));
			 return;
		    }
	       }
	       argvv_.push_back(arg);
	  }

private:
     const std::vector<OptionValueBase*>& opts_;
     program_option_type& argvv_;
};

// =============================================================================

void print_argvv(const program_option_type& argvv)
{
     std::cerr << "ERROR: ";
     for (program_option_type::const_iterator it(argvv.begin())
	       ; it != argvv.end() ; ++it) {
	  std::cerr << "'" << *it << "' ";
     }
     std::cerr << std::endl;
}

// =============================================================================
     
void ProgramOptionManager::usage()
{
     std::cout << "usage: " << prog_name_;
     std::for_each(positionals_.begin(), positionals_.end(), hn_op);
     std::for_each(opts_.begin(), opts_.end(), sln_op);
     std::cout << std::endl;
}

void ProgramOptionManager::print_help()
{
     usage();
     if (!desc_.empty()) {
	  std::cout << std::endl << desc_ << std::endl;
     }
     std::cout << "\nList of options:\n";
     std::for_each(positionals_.begin(), positionals_.end(), phl_op);
     std::for_each(opts_.begin(), opts_.end(), phl_op);
     std::cout << std::endl;
}

// -----------------------------------------------------------------------------

int ProgramOptionManager::process_arguments(int argc, char** argv)
{
     bool help(false);
     add_option("h", "help", help, "Show this help and exit");

     // std::sort(positionals_.begin(), positionals_.end(), hn_sort);
     std::sort(opts_.begin(), opts_.end(), sln_sort);

     program_option_type argvv;
     std::for_each(argv+1, argv + argc, back_insert_args(opts_, argvv));

     for (unsigned int p(0); p < opts_.size(); ++p) {
	  if (!opts_[p]->consume(argvv)) {
	       std::cerr << "ERROR: something bad happened while parsing named arguments!:\n";
	       print_argvv(argvv);
	       return -1;
	  }
     }

     for (unsigned int p(0); p < positionals_.size(); ++p) {
	  positionals_[p]->consume(argvv); // cannot fail
     }

     if (help) {
	  print_help();
	  return 0;
     }
     else if (!argvv.empty()) {
	  std::cerr << "ERROR: some arguments I could not process:" << std::endl;

	  print_argvv(argvv);
	  return -1;
     }
     else {
	  for (const_iterator it(positionals_.begin()) ;
	       it < positionals_.end() ; ++it) {
	       if ((*it)->required() && !(*it)->consumed()) {
		    std::cerr << "ERROR: missing a value for: "
			      << (*it)->help_name()
			      << std::endl;
		    return -1;
	       }
	  }
	  
	  for (const_iterator it(opts_.begin()) ;
	       it < opts_.end() ; ++it) {
	       if ((*it)->required() && !(*it)->consumed()) {
		    std::string opt_name((*it)->long_name().empty()
					 ? (*it)->help_name()
					 : "--" + (*it)->long_name());
		    std::cerr << "ERROR: missing the "
			      << opt_name
			      << " command line option"
			      << std::endl;
		    return -1;
	       }
	  }
     }

     return 1;
}     
