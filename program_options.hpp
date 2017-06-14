#ifndef PROGRAM_OPTIONS_HPP_INCLUDED
#define PROGRAM_OPTIONS_HPP_INCLUDED

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace internal_ {
     // Inspired from Boost::TypeTraits
     template <bool val>
     struct integral_constant
     {
	  typedef bool value_type;
	  typedef integral_constant<val> type;
	  static const bool value = val;
	  operator bool()const { return val; }
     };

     template <bool val>
     bool const integral_constant<val>::value;

     typedef integral_constant<true> true_type;
     typedef integral_constant<false> false_type;

     template <typename T> struct is_numeric : public false_type {};

     template <> struct is_numeric<short int> : public true_type {};
     template <> struct is_numeric<int> : public true_type {};
     template <> struct is_numeric<long int> : public true_type {};
     template <> struct is_numeric<unsigned short int> : public true_type {};
     template <> struct is_numeric<unsigned int> : public true_type {};
     template <> struct is_numeric<unsigned long int> : public true_type {};
     template <> struct is_numeric<float> : public true_type {};
     template <> struct is_numeric<double> : public true_type {};
     template <> struct is_numeric<long double> : public true_type {};

     
     template <bool is_num>
     struct do_assign
     {
	  template <typename U, typename T>
	  static bool apply(U&, const T&) { return false; }
     };
     template <>
     struct do_assign<true>
     {
	  template <typename U, typename T>
	  static bool apply(U& u, const T& t) { u = t; return true; }
     };


     template <typename T>
     struct traits
     {
	  template <typename U>
	  static bool assign_to(U&u, const T& t)
	       {
		    return do_assign<is_numeric<T>::value
				     && is_numeric<U>::value>::apply(u, t); 
	       }
     };

     // ========================================================================

     typedef std::vector<std::string> program_option_type;

     //! Class wrap a reference for storage in STL containers
     template<class T> class ReferenceWrapper
     {
     public:
	  //! Typedef to recover template parameter
	  typedef T type;

	  //! Constructor
	  explicit ReferenceWrapper(T& t): t_(&t) {}
	  //! Convertion to reference operator
	  operator T& () const { return *t_; }
	  //! Simple getter function
	  T& get() const { return *t_; }
	  //! Simple getter function
	  T* get_pointer() const { return t_; }

     private:
	  T* t_;
     };

     // ========================================================================

     //! Base class for all options
     /** Options are basically defined by:
      *    - short name:  typically one char (may be empty)
      *    - long name:   longer more meaningful name
      *    - help name:   named used for the value in the help output
      *                   (similar to Python.argparse)
      */
     class OptionValueBase
     {
     public:
	  //! Size of first column in help output
	  enum {HELP_PAD = 40};
     
	  //! Constructor for flags and values options
	  /** \param short_name Short name for the option (without leading -)
	   *  \param long_name Long name for the option (without leading --)
	   *  \param desc Description of the option
	   *  \param required Whether the option must be present or not
	   *
	   * Long name will be used as help name
	   */
	  OptionValueBase(const char* short_name,
			  const char* long_name,
			  const char* desc,
			  bool required)
	       : short_name_(short_name)
	       , long_name_(long_name)
	       , help_name_(long_name)
	       , desc_(desc)
	       , consumed_(false)
	       , required_(required)
	       {}
     
	  //! Constructor for flags and value options with user-defined help name
	  /** \param short_name Short name for the option (without leading -)
	   *  \param long_name Long name for the option (without leading --)
	   *  \param help_name Name to display in the help output
	   *  \param desc Description of the option
	   *  \param required Whether the option must be present or not
	   */
	  OptionValueBase(const char* short_name,
			  const char* long_name,
			  const char* help_name,
			  const char* desc,
			  bool required)
	       : short_name_(short_name)
	       , long_name_(long_name)
	       , help_name_(help_name)
	       , desc_(desc)
	       , consumed_(false)
	       , required_(required)
	       {}

	  //! Constructor for positional options (ie. without short or long names)
	  /** \param help_name Name of the option
	   *  \param desc Description of the option
	   *  \param required Whether the option must be present or not
	   */
	  OptionValueBase(const char* help_name,
			  const char* desc,
			  bool required)
	       : short_name_()
	       , long_name_()
	       , help_name_(help_name)
	       , desc_(desc)
	       , consumed_(false)
	       , required_(required)
	       {}

	  //! Destructor
	  virtual ~OptionValueBase() {}

	  //! Checks the short name (with leading -) matches the beginning of arg
	  bool match_short_name(const std::string& arg)
	       {
		    return (arg[0] == '-'
			    && arg.size() > 1
			    && arg[1] != '-'
			    && arg.substr(1,1) == short_name_);
	       }

	  //! Process a list of argument
	  /** \param opts List of input arguments
	   *  \return False if an error is detected, true otherwise
	   */
	  virtual bool consume(program_option_type& opts) = 0;

	  //! Checks whether an argument has been consumed or not
	  bool consumed() const { return consumed_; }
	  //! Checks whether an argument is required or not
	  bool required() const { return required_; }

	  //! Prints a line with parameter name and description
	  virtual void print_help_line() const = 0;

	  const std::string& short_name() const { return short_name_; }
	  const std::string& long_name() const { return long_name_; }
	  const std::string& help_name() const { return help_name_; }
	  const std::string& desc() const { return desc_; }

	  virtual std::string usage_name() const
	       {
		    std::string ret("[-");
		    if (!short_name_.empty()) {
			 ret += short_name_ + "]";
		    }
		    else {
			 ret += "-" + long_name_ + "]";
		    }
		    return ret;
	       }

	  virtual bool uint_assign_to(unsigned int&) const = 0;

     protected:
	  std::string short_name_;
	  std::string long_name_;
	  std::string help_name_;
	  std::string desc_;
	  bool consumed_;
	  bool required_;
     };

     // ========================================================================

     struct CountDependentOption
     {
	  CountDependentOption(const char* name_a,
			       bool force_exact_count_a = true)
	       : name(name_a)
	       , dependent(NULL)
	       , force_exact_count(force_exact_count_a)
		 {}

	  std::string name;
	  OptionValueBase* dependent;
	  bool force_exact_count;
     };
     
     inline CountDependentOption count_depends_on(const char* name)
	  {
	       return CountDependentOption(name);
	  }
     
     // ------------------------------------------------------------------------

     //! Sub-class for valued options
     template <typename T>
     class NameValue : public OptionValueBase
     {
     public:
	  NameValue(const char* s_name,
		    const char* l_name,
		    T& val,
		    const char* desc,
		    bool required = false)
	       : OptionValueBase(s_name, l_name, desc, required)
	       , value_(val)
	       {}
	  NameValue(const char* s_name,
		    const char* l_name,
		    const char* h_name,
		    T& val,
		    const char* desc,
		    bool required = false)
	       : OptionValueBase(s_name, l_name, h_name, desc, required)
	       , value_(val)
	       {}
     
	  bool consume(program_option_type& opts)
	       {
		    typedef program_option_type::iterator iter_type;
	       
		    for (iter_type it(opts.begin()); it != opts.end(); ++it) {
			 if ((it->size() == 2 && it->substr(1) == short_name_)
			     || (it->size() > 1 && it->substr(2) == long_name_)) {
			      if (it+1 == opts.end()
				  || (*(it+1))[0] == '-') {
				   return false;
			      }
			 
			      std::istringstream ssin(*(it+1));			 
			      ssin >> value_.get();
			      if (!ssin.good() && !ssin.eof()) {
				   /* 
				    * not being able to fully consume an
				    * argument is considered an error
				    * (could be failed conversion)
				    */
				   return false;
			      }
			      consumed_ = true;
			      opts.erase(it, it+2);
			      return true;
			 }
		    }
		    return true;
	       }

	  std::string usage_name() const
	       {
		    std::ostringstream ssout;
		    ssout << "[-";
		    if (!short_name_.empty()) {
			 ssout << short_name_ << " " << short_name_;
		    }
		    else {
			 ssout << "-" << long_name_ << " "
			       << toupper(long_name_[0]);
		    }
		    ssout << "]";
		    return ssout.str();
	       }

	  void print_help_line() const
	       {
		    if (!short_name_.empty()) {
			 std::string up(help_name_);
			 std::transform(up.begin(), up.end(),
					up.begin(),
					::toupper);
			 std::cout << std::left << std::setw(HELP_PAD)
				   << "-" + short_name_
			      + " [ --" + long_name_ + " ] "
			      + up
				   << desc_ << std::endl;
		    }
		    else {
			 std::cout << std::left << std::setw(HELP_PAD)
				   << "--" + long_name_ + " " + long_name_
				   << desc_ << std::endl;
		    }
	       }

	  bool uint_assign_to(unsigned int& val) const
	       {
		    return traits<T>::assign_to(val, value_.get());
	       }
     private:
	  ReferenceWrapper<T> value_;     
     };

     // ------------------------------------------------------------------------

     //! Sub-class for valued options with more than one value
     template <typename T>
     class NameValue< std::vector<T> > : public OptionValueBase
     {
     public:
	  typedef T value_type;
	  typedef std::vector<T> container_type;
	  NameValue(const char* s_name,
		    const char* l_name,
		    container_type& val,
		    unsigned int count,
		    const char* desc,
		    bool required = false)
	       : OptionValueBase(s_name, l_name, desc, required)
	       , value_(val)
	       , max_count_(count)
	       {}
	  NameValue(const char* s_name,
		    const char* l_name,
		    const char* h_name,
		    container_type& val,
		    unsigned int count,
		    const char* desc,
		    bool required = false)
	       : OptionValueBase(s_name, l_name, h_name, desc, required)
	       , value_(val)
	       , max_count_(count)
	       {}
     
	  bool consume(program_option_type& opts)
	       {
		    typedef program_option_type::iterator iter_type;
		    for (iter_type it(opts.begin()); it != opts.end(); ++it) {
			 if ((it->size() == 2 && it->substr(1) == short_name_)
			     || (it->size() > 1 && it->substr(2) == long_name_)) {

			      iter_type begin(it);
			      
			      unsigned int count(0);
			      while (it+1 != opts.end()
				     && (*(it+1))[0] != '-'
				     && count < max_count_) {
				   
				   if (it+1 == opts.end()
				       || (*(it+1))[0] == '-') {
					return false;
				   }

				   value_type tmp;
				   std::istringstream ssin(*(it+1));
				   ssin >> tmp;
				   value_.get().push_back(tmp);
				   
				   if (!ssin.good() && !ssin.eof()) {
					/* 
					 * not being able to fully consume an
					 * argument is considered an error
					 * (could be failed conversion)
					 */
					return false;
				   }
				   
				   ++count;
				   ++it;
			      }

			      if (count != max_count_) {
				   return false;
			      }

			      consumed_ = true;			 
			      opts.erase(begin, it+1);
			      it = opts.begin(); // iterators were invalidated by vector::erase()
			 }
		    }
		    return true;
	       }

	  std::string usage_name() const
	       {
		    std::ostringstream ssout;
		    ssout << "[-";
		    if (!short_name_.empty()) {
			 ssout << short_name_;

			 if (max_count_ < 5) {
			      for (unsigned int c(0); c < max_count_; ++c) {
				   ssout << " " << short_name_;
			      }
			 }
			 else {
			      ssout << " " << short_name_ << " " << max_count_ << "x";
			 }
		    }
		    else {
			 ssout << "-" << long_name_;
			 
			 char c(toupper(long_name_[0]));
			 if (max_count_ < 5) {
			      for (unsigned int c(0); c < max_count_; ++c) {
				   ssout << " " << c;
			      }
			 }
			 else {
			      ssout << " " << c << " " << max_count_ << "x";
			 }
		    }
		    ssout << "]";
		    return ssout.str();
	       }

	  void print_help_line() const
	       {
		    if (!short_name_.empty()) {
			 std::string up(help_name_);
			 std::transform(up.begin(), up.end(),
					up.begin(),
					::toupper);


			 std::ostringstream ssout;

			 ssout << "-" << short_name_
			       << " [ --" << long_name_ << " ] "
			       << up;

			 if (max_count_ > 1) {
			      ssout << " (" << max_count_ << "x)";
			 }
			 
			 std::cout << std::left << std::setw(HELP_PAD)
				   << ssout.str()
				   << desc_ << std::endl;
		    }
		    else {
			 std::cout << std::left << std::setw(HELP_PAD)
				   << "--" + long_name_ + " " + long_name_
				   << desc_ << std::endl;
		    }
	       }

	  bool uint_assign_to(unsigned int&) const { return false; }

     private:
	  ReferenceWrapper<container_type> value_;
	  unsigned int max_count_;	  
     };

     // ------------------------------------------------------------------------

     //! Sub-class for flag options
     template <>
     class NameValue<bool> : public OptionValueBase
     {
     public:
	  NameValue(const char* s_name,
		    const char* l_name,
		    bool& val,
		    const char* desc,
		    bool required = false)
	       : OptionValueBase(s_name, l_name, desc, required)
	       , value_(val)
	       {}
	  NameValue(const char* s_name,
		    const char* l_name,
		    const char* h_name,
		    bool& val,
		    const char* desc,
		    bool required = false)
	       : OptionValueBase(s_name, l_name, h_name, desc, required)
	       , value_(val)
	       {}

	  bool consume(program_option_type& opts)
	       {
		    typedef program_option_type::iterator iter_type;
		    for (iter_type it(opts.begin()); it != opts.end(); ++it) {
			 if ((it->size() == 2 && it->substr(1) == short_name_)
			     || (it->size() > 1 && it->substr(2) == long_name_)) {
			      opts.erase(it);
			      value_.get() = true;
			      consumed_ = true;
			      return true;
			 }
		    }
		    return true;
	       }

	  void print_help_line() const
	       {
		    if (!short_name_.empty()) {
			 std::cout << std::left << std::setw(HELP_PAD)
				   << "-" + short_name_
			      + " [ --" + long_name_ + " ] "
				   << desc_ << std::endl;
		    }
		    else {
			 std::cout << std::left << std::setw(HELP_PAD)
				   << "--" + long_name_
				   << desc_ << std::endl;
		    }
	       }

	  bool uint_assign_to(unsigned int&) const { return false; }
     private:
	  ReferenceWrapper<bool> value_;
     };

     // ------------------------------------------------------------------------

     //! Sub-class for flag options with particular value to assign
     template <typename T>
     class FlagValue: public OptionValueBase
     {
     public:
	  FlagValue(const char* s_name,
		    const char* l_name,
		    T& val,
		    T val_to_assign,
		    const char* desc,
		    bool required = false)
	       : OptionValueBase(s_name, l_name, desc, required)
	       , value_(val)
	       , value_to_assign_(val_to_assign)
	       {}
	  FlagValue(const char* s_name,
		    const char* l_name,
		    const char* h_name,
		    bool& val,
		    T val_to_assign,
		    const char* desc,
		    bool required = false)
	       : OptionValueBase(s_name, l_name, h_name, desc, required)
	       , value_(val)
	       , value_to_assign_(val_to_assign)
	       {}

	  bool consume(program_option_type& opts)
	       {
		    typedef program_option_type::iterator iter_type;
		    for (iter_type it(opts.begin()); it != opts.end(); ++it) {
			 if ((it->size() == 2 && it->substr(1) == short_name_)
			     || (it->size() > 1 && it->substr(2) == long_name_)) {
			      opts.erase(it);
			      value_.get() = value_to_assign_;
			      consumed_ = true;
			      return true;
			 }
		    }
		    return true;
	       }

	  void print_help_line() const
	       {
		    if (!short_name_.empty()) {
			 std::cout << std::left << std::setw(HELP_PAD)
				   << "-" + short_name_
			      + " [ --" + long_name_ + " ] "
				   << desc_ << std::endl;
		    }
		    else {
			 std::cout << std::left << std::setw(HELP_PAD)
				   << "--" + long_name_
				   << desc_ << std::endl;
		    }
	       }
     
	  bool uint_assign_to(unsigned int&) const { return false; }
	  
     private:
	  ReferenceWrapper<T> value_;
	  T value_to_assign_;
     };

     // ------------------------------------------------------------------------

     //! Sub-class for positional options with single value
     template <typename T>
     class PositionalValue : public OptionValueBase
     {
     public:
	  PositionalValue(const char* h_name,
			  T& val,
			  const char* desc,
			  bool required = true)
	       : OptionValueBase(h_name, desc, required)
	       , value_(val)
	       {}
     
	  bool consume(program_option_type& opts)
	       {
		    typedef program_option_type::iterator iter_type;
		    for (iter_type it(opts.begin()); it != opts.end(); ++it) {
			 if ((*it)[0] != '-') {
			      std::istringstream ssin(*it);
			      ssin >> value_.get();
			      opts.erase(it);
			      consumed_ = true;
			      return true;
			 }
		    }
		    return true;
	       }

	  void print_help_line() const
	       {
		    std::cout << std::left << std::setw(HELP_PAD)
			      << help_name_
			      << desc_ << std::endl;
	       }

	  bool uint_assign_to(unsigned int& val) const
	       {
		    return traits<T>::assign_to(val, value_.get());
	       }
     
     private:
	  ReferenceWrapper<T> value_;
     };

     // ------------------------------------------------------------------------

     //! Sub-class for positional options with multiple values
     template <typename T>
     class PositionalValue< std::vector<T> > : public OptionValueBase
     {
     public:
	  PositionalValue(const char* h_name,
			  std::vector<T>& val,
			  unsigned int count,
			  const char* desc,
			  bool required = true)
	       : OptionValueBase(h_name, desc, required)
	       , value_(val)
	       , count_(0)
	       , exact_count_(false)
	       , max_count_(count)
	       , count_dependent_(NULL)
	       {}

	  PositionalValue(const char* h_name,
			  std::vector<T>& val,
			  const CountDependentOption& opt,
			  const char* desc,
			  bool required = true)
	       : OptionValueBase(h_name, desc, required)
	       , value_(val)
	       , count_(0)
	       , exact_count_(opt.force_exact_count)
	       , max_count_(0)
	       , count_dependent_(opt.dependent)
	       {}
	  	  
	  bool consume(program_option_type& opts)
	       {
		    if (max_count_ == 0) {
			 if (count_dependent_ != NULL) {
			      count_dependent_->uint_assign_to(max_count_);
			 }
			 else {
			      std::cerr << "max_count = 0 && count_dependent == NULL"
					<< std::endl;
			      return false;
			 }
		    }
		    
		    typedef program_option_type::iterator iter_type;
		    // multiple-valued positional consume as many arguments as
		    // possible
		    bool cont_flag(true);
		    iter_type mark(opts.begin());
		    for (iter_type it(opts.begin());
			 cont_flag && it != opts.end() && count_ < max_count_;
			 ++it) {
			 if ((*it)[0] != '-') {
			      std::istringstream ssin(*it);
			      T tmp;
			      ssin >> tmp;
			      if (!ssin.good() && !ssin.eof()) {
				   /* 
				    * not being able to fully consume an
				    * argument is considered an error
				    * (could be failed conversion)
				    */
				   cont_flag = false;
			      }
			      else {
				   value_.get().push_back(tmp);
				   std::swap(*mark++, *it);
				   ++count_;
			      }
			 }
		    }
		    opts.erase(opts.begin(), mark);
		    if (!cont_flag) {
			 return false;
		    }

		    if (exact_count_ > 0 && count_ != max_count_) {
			 std::cerr << "ERROR: " << help_name()
				   << " requires exactly " << max_count_
				   << " arguments!"
				   << std::endl;
			 return false; 
		    }
		    
		    consumed_ = true;
		    return true;
	       }

	  void print_help_line() const
	       {
		    if (count_dependent_ != NULL) {
			 std::ostringstream ssout;
			 ssout << help_name_
			       << " (" << count_dependent_->help_name() << " x)";
			 std::cout << std::left << std::setw(HELP_PAD)
				   << ssout.str()
				   << desc_
				   << std::endl
				   << std::left << std::setw(HELP_PAD)
				   << " "
				   << "-> count depends on "
				   << count_dependent_->help_name()
				   << std::endl;
		    }
		    else if (max_count_ > 1) {
			 std::ostringstream ssout;
			 ssout << help_name_
			       << " (" << max_count_ << "x)";
			 std::cout << std::left << std::setw(HELP_PAD)
				   << ssout.str()
				   << desc_ << std::endl;
		    }
		    else {
			 std::cout << std::left << std::setw(HELP_PAD)
				   << help_name_
				   << desc_ << std::endl;
		    }
	       }

	  bool uint_assign_to(unsigned int&) const { return false; }
     
     private:
	  ReferenceWrapper< std::vector<T> > value_;
	  unsigned int count_;
	  bool exact_count_;
	  unsigned int max_count_;
	  OptionValueBase* count_dependent_;
     };

     // ========================================================================

     //! Helper function to ease the creating of options
     template <typename T>
     OptionValueBase* make_value(const char* short_name,
				 const char* long_name,
				 T& value,
				 const char* desc,
				 bool required)
     {
	  return new NameValue<T>(short_name, long_name, value, desc, required);
     }

     //! Helper function to ease the creating of options
     template <typename T>
     OptionValueBase* make_value(const char* short_name,
				 const char* long_name,
				 std::vector<T>& value,
				 unsigned int count,
				 const char* desc,
				 bool required)
     {
	  return new NameValue< std::vector<T> >(short_name, long_name, value, count, desc, required);
     }

     //! Helper function to ease the creating of options
     template <typename T>
     OptionValueBase* make_value(const char* s_name,
				 const char* l_name,
				 const char* h_name,
				 T& value,
				 const char* desc,
				 bool required)
     {
	  return new NameValue<T>(s_name, l_name, h_name, value, desc, required);
     }

     //! Helper function to ease the creating of options
     template <typename T>
     OptionValueBase* make_value(const char* short_name,
				 const char* long_name,
				 T& value,
				 T value_to_assign,
				 const char* desc,
				 bool required)
     {
	  return new FlagValue<T>(short_name,
				  long_name,
				  value,
				  value_to_assign,
				  desc,
				  required);
     }

     //! Helper function to ease the creating of options
     template <typename T>
     OptionValueBase* make_value(const char* s_name,
				 const char* l_name,
				 const char* h_name,
				 T& value,
				 T value_to_assign,
				 const char* desc,
				 bool required)
     {
	  return new FlagValue<T>(s_name,
				  l_name,
				  h_name,
				  value,
				  value_to_assign,
				  desc,
				  required);
     }

     //! Helper function to ease the creating of options
     template <typename T>
     OptionValueBase* make_value(const char* help_name,
				 T& value,
				 const char* desc,
				 bool required)
     {
	  return new PositionalValue<T>(help_name, value, desc, required);
     }

     //! Helper function to ease the creating of options
     template <typename T>
     OptionValueBase* make_value(const char* help_name,
				 std::vector<T>& value,
				 unsigned int count,
				 const char* desc,
				 bool required)
     {
	  return new PositionalValue< std::vector<T> >(help_name,
						       value,
						       count,
						       desc,
						       required);
     }

     //! Helper function to ease the creating of options
     template <typename T>
     OptionValueBase* make_value(const char* help_name,
				 std::vector<T>& value,
				 const CountDependentOption& dependent,
				 const char* desc,
				 bool required)
     {
	  return new PositionalValue< std::vector<T> >(help_name,
						       value,
						       dependent,
						       desc,
						       required);
     }
} // namespace internal_

using internal_::count_depends_on;

// =============================================================================

//! Class managing all program options
class ProgramOptionManager
{
     //! Custom deleter functor
     template <typename T>
     struct Deleter
     {
	  void operator()(T t)
	       {
		    delete t;
	       }
     };
     typedef internal_::OptionValueBase OptionValueBase;
     typedef Deleter<OptionValueBase*> deleter;
     
public:
     //! Convenience typedef
     typedef std::vector<OptionValueBase*>::iterator iterator;
     //! Convenience typedef
     typedef std::vector<OptionValueBase*>::const_iterator const_iterator;

     //! Constructor
     ProgramOptionManager(const char* prog_name,
			  const char* desc)
	  : prog_name_(prog_name)
	  , desc_(desc)
	  {}
     
     //! Destructor
     ~ProgramOptionManager()
	  {
	       std::for_each(positionals_.begin(), positionals_.end(), deleter());
	       std::for_each(opts_.begin(), opts_.end(), deleter());
	  }

     //! Method to add a flag or valued option
     template <typename T>
     ProgramOptionManager& add_option(const char* short_name,
				      const char* long_name,
				      T& value,
				      const char* desc,
				      bool required = false)
	  {
	       opts_.push_back(internal_::make_value(short_name,
						     long_name,
						     value,
						     desc,
						     required));
	       return *this;
	  }
     template <typename T>
     ProgramOptionManager& add_option(const char* short_name,
				      const char* long_name,
				      std::vector<T>& value,
				      unsigned int count,
				      const char* desc,
				      bool required = false)
	  {
	       opts_.push_back(internal_::make_value(short_name,
						     long_name,
						     value,
						     count,
						     desc,
						     required));
	       return *this;
	  }
     //! Method to add a flag or valued option
     template <typename T>
     ProgramOptionManager& add_option(const char* short_name,
				      const char* long_name,
				      const char* help_name,
				      T& value,
				      const char* desc,
				      bool required = false)
	  {
	       opts_.push_back(internal_::make_value(short_name,
						     long_name,
						     help_name,
						     value,
						     desc,
						     required));
	       return *this;
	  }

     //! Method to add a flag
     template <typename T>
     ProgramOptionManager& add_option(const char* short_name,
				      const char* long_name,
				      T& value,
				      T value_to_assign,
				      const char* desc,
				      bool required = false)
	  {
	       opts_.push_back(internal_::make_value(short_name,
						     long_name,
						     value,
						     value_to_assign,
						     desc,
						     required));
	       return *this;
	  }
     
     //! Method to add a flag
     template <typename T>
     ProgramOptionManager& add_option(const char* short_name,
				      const char* long_name,
				      const char* help_name,
				      T& value,
				      T value_to_assign,
				      const char* desc,
				      bool required = false)
	  {
	       opts_.push_back(internal_::make_value(short_name,
						     long_name,
						     help_name,
						     value,
						     value_to_assign,
						     desc,
						     required));
	       return *this;
	  }

     //! Method to add a positional option with single value
     template <typename T>
     ProgramOptionManager& add_option(const char* help_name,
				      T& value,
				      const char* desc,
				      bool required = true)
	  {
	       positionals_.push_back(internal_::make_value(help_name,
							    value,
							    desc,
							    required));
	       return *this;
	  }

     //! Method to add a positional option with multiple values
     template <typename T>
     ProgramOptionManager& add_option(const char* help_name,
				      std::vector<T>& value,
				      unsigned int count,
				      const char* desc,
				      bool required = true)
	  {
	       positionals_.push_back(internal_::make_value(help_name,
							    value,
							    count,
							    desc,
							    required));
	       return *this;
	  }

     //! Method to add a positional option with multiple values
     /** This overload makes use of a dependent option to set the maximum number
      *  of values 
      */
     template <typename T>
     ProgramOptionManager& add_option(const char* help_name,
				      std::vector<T>& value,
				      internal_::CountDependentOption opt,
				      const char* desc,
				      bool required = true)
	  {
	       opt.dependent = NULL; // just to be sure...
	       
	       for (unsigned int i(0);
		    opt.dependent == NULL && i < opts_.size();
		    ++i) {
		    if (opt.name == opts_[i]->short_name()
			 || opt.name == opts_[i]->long_name()) {
			 opt.dependent = opts_[i];
		    }
	       }
	       for (unsigned int i(0);
		    opt.dependent == NULL && i < positionals_.size();
		    ++i) {
		    if (opt.name == positionals_[i]->help_name()) {
			 opt.dependent = positionals_[i];
		    }
	       }

	       if (opt.dependent == NULL) {
		    std::cerr << "ERROR: dependent option does not exist!"
			      << std::endl;		    
	       }
	       else {
		    positionals_.push_back(internal_::make_value(help_name,
								 value,
								 opt,
								 desc,
								 required));
	       }
	       return *this;
	  }

     //! Print usage line
     void usage();
     //! Print usage line and some more detailed help messages
     void print_help();

     //! Method to call to process the program arguments
     /** \return 0 if the program needs to close normally right after this call, 
      *          >0 if everything is ok and <0 if an error occurred
      *  \note This method will automaticall add a -h/--help flag option
      */
     int process_arguments(int argc, char** argv);
     
private:
     std::string prog_name_;
     std::string desc_;
     std::vector<OptionValueBase*> opts_;
     std::vector<OptionValueBase*> positionals_;
};

#endif //PROGRAM_OPTIONS_HPP_INCLUDED
