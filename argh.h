#pragma once

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <cassert>

using std::initializer_list;
using std::multiset;
using std::vector;
using std::map;
using std::set;
using std::transform;
using std::ios_base;
using std::any_of;
using std::ios_base;

#ifdef UNICODE

using std::wstring;
using std::wstringbuf;
using std::wstringstream;
using std::wistringstream;
using std::wostringstream;

typedef	wchar_t				TCHAR;
typedef wstring				TSTRING;
typedef wstringbuf			TSTRINGBUF;
typedef wstringstream		TSTRINGSTREAM;
typedef wistringstream		TISTRINGSTREAM;
typedef wostringstream		TOSTRINGSTREAM;

#else // ASCII

using std::string;
using std::stringbuf;
using std::stringstream;
using std::istringstream;
using std::ostringstream;

typedef	char				TCHAR;
typedef string				TSTRING;
typedef stringbuf			TSTRINGBUF;
typedef stringstream		TSTRINGSTREAM;
typedef istringstream		TISTRINGSTREAM;
typedef ostringstream		TOSTRINGSTREAM;

#endif

namespace argh
{
	// Terminology:
	// A command line is composed of 2 types of args:
	// 1. Positional args, i.e. free standing values
	// 2. Options: args beginning with '-'. We identify two kinds:
	//    2.1: Flags: boolean options =>  (exist ? true : false)
	//    2.2: Parameters: a name followed by a non-option value

#if !defined(__GNUC__) || (__GNUC__ >= 5)
	using string_stream = std::istringstream;
#else
	// Until GCC 5, istringstream did not have a move constructor.
	// stringstream_proxy is used instead, as a workaround.
	class tstringstream_proxy
	{
	public:
		tstringstream_proxy() = default;

		// Construct with a value.
		tstringstream_proxy(TSTRING const& value) :
			stream_(value)
		{}

		// Copy constructor.
		tstringstream_proxy(const tstringstream_proxy& other) :
			stream_(other.stream_.str())
		{
			stream_.setstate(other.stream_.rdstate());
		}

		void setstate(ios_base::iostate state) { stream_.setstate(state); }

		// Stream out the value of the parameter.
		// If the conversion was not possible, the stream will enter the fail state,
		// and operator bool will return false.
		template<typename T>
		tstringstream_proxy& operator >> (T& thing)
		{
			stream_ >> thing;
			return *this;
		}


		// Get the string value.
		TSTRING str() const { return stream_.str(); }

		TSTRINGBUF* rdbuf() const { return stream_.rdbuf(); }

		// Check the state of the stream. 
		// False when the most recent stream operation failed
		operator bool() const { return !!stream_; }

		~stringstream_proxy() = default;
	private:
		TSTRINGSTREAM stream_;
	};
	using TSTRINGSTREAM = stringstream_proxy;
#endif

	class parser
	{
	public:
		enum Mode {
			PREFER_FLAG_FOR_UNREG_OPTION = 1 << 0,
			PREFER_PARAM_FOR_UNREG_OPTION = 1 << 1,
			NO_SPLIT_ON_EQUALSIGN = 1 << 2,
			SINGLE_DASH_IS_MULTIFLAG = 1 << 3,
		};

		parser() = default;

		parser(initializer_list<TCHAR const* const> pre_reg_names)
		{
			add_params(pre_reg_names);
		}

		parser(const TCHAR* const argv[], int mode = PREFER_FLAG_FOR_UNREG_OPTION)
		{
			parse(argv, mode);
		}

		parser(int argc, const TCHAR* const argv[], int mode = PREFER_FLAG_FOR_UNREG_OPTION)
		{
			parse(argc, argv, mode);
		}

		void add_param(TSTRING const& name);
		void add_params(initializer_list<TCHAR const* const> init_list);

		void parse(const TCHAR* const argv[], int mode = PREFER_FLAG_FOR_UNREG_OPTION);
		void parse(int argc, const TCHAR* const argv[], int mode = PREFER_FLAG_FOR_UNREG_OPTION);

		multiset<TSTRING>		const& flags()			const { return flags_; }
		map<TSTRING, TSTRING>	const& params()			const { return params_; }
		vector<TSTRING>			const& pos_args()		const { return pos_args_; }

		// begin() and end() for using range-for over positional args.
		vector<TSTRING>::const_iterator begin()			const { return pos_args_.cbegin(); }
		vector<TSTRING>::const_iterator end()			const { return pos_args_.cend(); }
		size_t size()									const { return pos_args_.size(); }

		//////////////////////////////////////////////////////////////////////////
		// Accessors

		// flag (boolean) accessors: return true if the flag appeared, otherwise false.
		bool operator[](TSTRING const& name) const;

		// multiple flag (boolean) accessors: return true if at least one of the flag appeared, otherwise false.
		bool operator[](initializer_list<TCHAR const* const> init_list) const;

		// returns positional arg string by order. Like argv[] but without the options
		TSTRING const& operator[](size_t ind) const;

		// returns a std::istream that can be used to convert a positional arg to a typed value.
		TSTRINGSTREAM operator()(size_t ind) const;

		// same as above, but with a default value in case the arg is missing (index out of range).
		template<typename T>
		TSTRINGSTREAM operator()(size_t ind, T&& def_val) const;

		// parameter accessors, give a name get an std::istream that can be used to convert to a typed value.
		// call .str() on result to get as string
		TSTRINGSTREAM operator()(TSTRING const& name) const;

		// accessor for a parameter with multiple names, give a list of names, get an std::istream that can be used to convert to a typed value.
		// call .str() on result to get as string
		// returns the first value in the list to be found.
		TSTRINGSTREAM operator()(initializer_list<TCHAR const* const> init_list) const;

		// same as above, but with a default value in case the param was missing.
		// Non-string def_val types must have an operator<<() (output stream operator)
		// If T only has an input stream operator, pass the string version of the type as in "3" instead of 3.
		template<typename T>
		TSTRINGSTREAM operator()(TSTRING const& name, T&& def_val) const;

		// same as above but for a list of names. returns the first value to be found.
		template<typename T>
		TSTRINGSTREAM operator()(initializer_list<TCHAR const* const> init_list, T&& def_val) const;

	private:
		TSTRINGSTREAM bad_stream() const;
		TSTRING trim_leading_dashes(TSTRING const& name) const;
		bool is_number(TSTRING const& arg) const;
		bool is_option(TSTRING const& arg) const;
		bool got_flag(TSTRING const& name) const;
		bool is_param(TSTRING const& name) const;

	private:
		vector<TSTRING> args_;
		map<TSTRING, TSTRING> params_;
		vector<TSTRING> pos_args_;
		multiset<TSTRING> flags_;
		set<TSTRING> registeredParams_;
		TSTRING empty_;
	};


	//////////////////////////////////////////////////////////////////////////

	inline void parser::parse(const TCHAR* const argv[], int mode)
	{
		int argc = 0;
		for (auto argvp = argv; *argvp; ++argc, ++argvp);
		parse(argc, argv, mode);
	}

	//////////////////////////////////////////////////////////////////////////

	inline void parser::parse(int argc, const TCHAR* const argv[], int mode /*= PREFER_FLAG_FOR_UNREG_OPTION*/)
	{
		// convert to strings
		args_.resize(argc);
		transform(argv, argv + argc, args_.begin(), [](const TCHAR* const arg) { return arg;  });

		// parse line
		for (auto i = 0u; i < args_.size(); ++i)
		{
			if (!is_option(args_[i]))
			{
				pos_args_.emplace_back(args_[i]);
				continue;
			}

			auto name = trim_leading_dashes(args_[i]);

			if (!(mode & NO_SPLIT_ON_EQUALSIGN))
			{
				auto equalPos = name.find('=');
				if (equalPos != std::string::npos)
				{
					params_.insert({ name.substr(0, equalPos), name.substr(equalPos + 1) });
					continue;
				}
			}

			// if the option is unregistered and should be a multi-flag
			if (1 == (args_[i].size() - name.size()) &&         // single dash
				argh::parser::SINGLE_DASH_IS_MULTIFLAG & mode && // multi-flag mode
				!is_param(name))                                  // unregistered
			{
				TSTRING keep_param;

				if (!name.empty() && is_param(TSTRING(1ul, name.back()))) // last char is param
				{
					keep_param += name.back();
					name.resize(name.size() - 1);
				}

				for (auto const& c : name)
				{
					flags_.emplace(TSTRING{ c });
				}

				if (!keep_param.empty())
				{
					name = keep_param;
				}
				else
				{
					continue; // do not consider other options for this arg
				}
			}

			// any potential option will get as its value the next arg, unless that arg is an option too
			// in that case it will be determined a flag.
			if (i == args_.size() - 1 || is_option(args_[i + 1]))
			{
				flags_.emplace(name);
				continue;
			}

			// if 'name' is a pre-registered option, then the next arg cannot be a free parameter to it is skipped
			// otherwise we have 2 modes:
			// PREFER_FLAG_FOR_UNREG_OPTION: a non-registered 'name' is determined a flag. 
			//                               The following value (the next arg) will be a free parameter.
			//
			// PREFER_PARAM_FOR_UNREG_OPTION: a non-registered 'name' is determined a parameter, the next arg
			//                                will be the value of that option.

			assert(!(mode & argh::parser::PREFER_FLAG_FOR_UNREG_OPTION)
				|| !(mode & argh::parser::PREFER_PARAM_FOR_UNREG_OPTION));

			bool preferParam = mode & argh::parser::PREFER_PARAM_FOR_UNREG_OPTION;

			if (is_param(name) || preferParam)
			{
				params_.insert({ name, args_[i + 1] });
				++i; // skip next value, it is not a free parameter
				continue;
			}
			else
			{
				flags_.emplace(name);
			}
		};
	}

	//////////////////////////////////////////////////////////////////////////

	inline TSTRINGSTREAM parser::bad_stream() const
	{
		TSTRINGSTREAM bad;
		bad.setstate(ios_base::failbit);
		return bad;
	}

	//////////////////////////////////////////////////////////////////////////

	inline bool parser::is_number(TSTRING const& arg) const
	{
		// inefficient but simple way to determine if a string is a number (which can start with a '-')
		TISTRINGSTREAM istr(arg);
		double number;
		istr >> number;
		return !(istr.fail() || istr.bad());
	}

	//////////////////////////////////////////////////////////////////////////

	inline bool parser::is_option(TSTRING const& arg) const
	{
		assert(0 != arg.size());
		if (is_number(arg))
			return false;
		return '-' == arg[0];
	}

	//////////////////////////////////////////////////////////////////////////

	inline TSTRING parser::trim_leading_dashes(TSTRING const& name) const
	{
		auto pos = name.find_first_not_of('-');
		return TSTRING::npos != pos ? name.substr(pos) : name;
	}

	//////////////////////////////////////////////////////////////////////////

	inline bool argh::parser::got_flag(TSTRING const& name) const
	{
		return flags_.end() != flags_.find(trim_leading_dashes(name));
	}

	//////////////////////////////////////////////////////////////////////////

	inline bool argh::parser::is_param(TSTRING const& name) const
	{
		return registeredParams_.count(name);
	}

	//////////////////////////////////////////////////////////////////////////

	inline bool parser::operator[](TSTRING const& name) const
	{
		return got_flag(name);
	}

	//////////////////////////////////////////////////////////////////////////

	inline bool parser::operator[](std::initializer_list<TCHAR const* const> init_list) const
	{
		return std::any_of(init_list.begin(), init_list.end(), [&](TCHAR const* const name) { return got_flag(name); });
	}

	//////////////////////////////////////////////////////////////////////////

	inline TSTRING const& parser::operator[](size_t ind) const
	{
		if (ind < pos_args_.size())
			return pos_args_[ind];
		return empty_;
	}

	//////////////////////////////////////////////////////////////////////////

	inline TSTRINGSTREAM parser::operator()(TSTRING const& name) const
	{
		auto optIt = params_.find(trim_leading_dashes(name));
		if (params_.end() != optIt)
			return TSTRINGSTREAM(optIt->second);
		return bad_stream();
	}

	//////////////////////////////////////////////////////////////////////////

	inline TSTRINGSTREAM parser::operator()(std::initializer_list<TCHAR const* const> init_list) const
	{
		for (auto& name : init_list)
		{
			auto optIt = params_.find(trim_leading_dashes(name));

			if (params_.end() != optIt)
				return TSTRINGSTREAM(optIt->second);
		}
		return bad_stream();
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename T>
	TSTRINGSTREAM parser::operator()(TSTRING const& name, T && def_val) const
	{
		auto optIt = params_.find(trim_leading_dashes(name));
		if (params_.end() != optIt)
			return TSTRINGSTREAM(optIt->second);

		TOSTRINGSTREAM ostr;
		ostr << def_val;
		return TSTRINGSTREAM(ostr.str()); // use default
	}

	//////////////////////////////////////////////////////////////////////////

	// same as above but for a list of names. returns the first value to be found.
	template<typename T>
	TSTRINGSTREAM parser::operator()(initializer_list<TCHAR const* const> init_list, T && def_val) const
	{
		for (auto& name : init_list)
		{
			auto optIt = params_.find(trim_leading_dashes(name));
			if (params_.end() != optIt)
				return TSTRINGSTREAM(optIt->second);
		}
		TOSTRINGSTREAM ostr;
		ostr << def_val;
		return TSTRINGSTREAM(ostr.str()); // use default
	}

	//////////////////////////////////////////////////////////////////////////

	inline TSTRINGSTREAM parser::operator()(size_t ind) const
	{
		if (pos_args_.size() <= ind)
			return bad_stream();

		return TSTRINGSTREAM(pos_args_[ind]);
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename T>
	TSTRINGSTREAM parser::operator()(size_t ind, T && def_val) const
	{
		if (pos_args_.size() <= ind)
		{
			TOSTRINGSTREAM ostr;
			ostr << def_val;
			return TSTRINGSTREAM(ostr.str());
		}

		return TSTRINGSTREAM(pos_args_[ind]);
	}

	//////////////////////////////////////////////////////////////////////////

	inline void parser::add_param(TSTRING const& name)
	{
		registeredParams_.insert(trim_leading_dashes(name));
	}

	//////////////////////////////////////////////////////////////////////////

	inline void parser::add_params(std::initializer_list<TCHAR const* const> init_list)
	{
		for (auto& name : init_list)
			registeredParams_.insert(trim_leading_dashes(name));
	}
}
