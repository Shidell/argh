#pragma once

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <cassert>

namespace argh
{
	// Terminology:
	// A command line is composed of 2 types of args:
	// 1. Positional args, i.e. free standing values
	// 2. Options: args beginning with '-'. We identify two kinds:
	//    2.1: Flags: boolean options =>  (exist ? true : false)
	//    2.2: Parameters: a name followed by a non-option value

#if !defined(__GNUC__) || (__GNUC__ >= 5)
	using wstring_stream = std::wstringstream;
#else
	// Until GCC 5, iwstringstream did not have a move constructor.
	// wstringstream_proxy is used instead, as a workaround.
	class wstringstream_proxy
	{
	public:
		wstringstream_proxy() = default;

		// Construct with a value.
		wstringstream_proxy(std::wstring const& value) :
			stream_(value)
		{}

		// Copy constructor.
		wstringstream_proxy(const wstringstream_proxy& other) :
			stream_(other.stream_.str())
		{
			stream_.setstate(other.stream_.rdstate());
		}

		void setstate(std::ios_base::iostate state) { stream_.setstate(state); }

		// Stream out the value of the parameter.
		// If the conversion was not possible, the stream will enter the fail state,
		// and operator bool will return false.
		template<typename T>
		wstringstream_proxy& operator >> (T& thing)
		{
			stream_ >> thing;
			return *this;
		}


		// Get the wstring value.
		std::wstring str() const { return stream_.str(); }

		std::wstringbuf* rdbuf() const { return stream_.rdbuf(); }

		// Check the state of the stream. 
		// False when the most recent stream operation failed
		operator bool() const { return !!stream_; }

		~wstringstream_proxy() = default;
	private:
		std::iwstringstream stream_;
	};
	using wstring_stream = wstringstream_proxy;
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

		template<typename TChar>
		parser(std::initializer_list<TChar const* const> pre_reg_names)
		{
			add_params(pre_reg_names);
		}

		template<typename TChar>
		parser(const TChar* const argv[], int mode = PREFER_FLAG_FOR_UNREG_OPTION)
		{
			parse(argv, mode);
		}

		template<typename TChar>
		parser(int argc, const TChar* const argv[], int mode = PREFER_FLAG_FOR_UNREG_OPTION)
		{
			parse(argc, argv, mode);
		}

		template<typename TString>
		void add_param(TString const& name);

		template<typename TChar>
		void add_params(std::initializer_list<TChar const* const> init_list);

		template<typename TChar>
		void parse(const TChar* const argv[], int mode = PREFER_FLAG_FOR_UNREG_OPTION);

		template<typename TChar>
		void parse(int argc, const TChar* const argv[], int mode = PREFER_FLAG_FOR_UNREG_OPTION);

		template<typename TString>
		std::multiset<TString>				const& flags()			const { return flags_; }

		template<typename TString>
		std::multimap<TString, TString>		const& params()			const { return params_; }

		template<typename TString>
		std::vector<TString>				const& pos_args()		const { return pos_args_; }

		// begin() and end() for using range-for over positional args.
		std::vector<std::wstring>::const_iterator begin()				const { return pos_args_.cbegin(); }
		std::vector<std::wstring>::const_iterator end()					const { return pos_args_.cend(); }
		size_t size()												const { return pos_args_.size(); }

		//////////////////////////////////////////////////////////////////////////
		// Accessors

		// flag (boolean) accessors: return true if the flag appeared, otherwise false.
		template<typename TString>
		bool operator[](TString const& name) const;

		// multiple flag (boolean) accessors: return true if at least one of the flag appeared, otherwise false.
		template<typename TChar>
		bool operator[](std::initializer_list<TChar const* const> init_list) const;

		// returns positional arg wstring by order. Like argv[] but without the options
		template<typename TString>
		TString const& operator[](size_t ind) const;

		// returns a std::istream that can be used to convert a positional arg to a typed value.
		template<typename TString_Stream>
		TString_Stream operator()(size_t ind) const;

		// same as above, but with a default value in case the arg is missing (index out of range).
		template<typename TString_Stream, typename T>
		TString_Stream operator()(size_t ind, T&& def_val) const;

		// parameter accessors, give a name get an std::istream that can be used to convert to a typed value.
		// call .str() on result to get as wstring
		template<typename TString_Stream, typename TString>
		TString_Stream operator()(TString const& name) const;

		// accessor for a parameter with multiple names, give a list of names, get an std::istream that can be used to convert to a typed value.
		// call .str() on result to get as wstring
		// returns the first value in the list to be found.
		template<typename TString_Stream, typename TChar>
		TString_Stream operator()(std::initializer_list<TChar const* const> init_list) const;

		// same as above, but with a default value in case the param was missing.
		// Non-wstring def_val types must have an operator<<() (output stream operator)
		// If T only has an input stream operator, pass the wstring version of the type as in "3" instead of 3.
		template<typename TString_Stream, typename TString, typename T>
		TString_Stream operator()(TString const& name, T&& def_val) const;

		// same as above but for a list of names. returns the first value to be found.
		template<typename TString_Stream, typename TChar, typename T>
		TString_Stream operator()(std::initializer_list<TChar const* const> init_list, T&& def_val) const;

	private:
		template<typename TString_Stream>
		TString_Stream bad_stream() const;

		template<typename TString>
		TString trim_leading_dashes(TString const& name) const;

		template <typename TString>
		bool is_number(TString const& arg) const;

		template <typename TString>
		bool is_option(TString const& arg) const;

		template <typename TString>
		bool got_flag(TString const& name) const;

		template <typename TString>
		bool is_param(TString const& name) const;

	private:
		std::vector<std::wstring> args_;
		std::multimap<std::wstring, std::wstring> params_;
		std::vector<std::wstring> pos_args_;
		std::multiset<std::wstring> flags_;
		std::set<std::wstring> registeredParams_;
		std::wstring empty_;
	};


	//////////////////////////////////////////////////////////////////////////

	template<typename TChar>
	inline void parser::parse(const TChar* const argv[], int mode)
	{
		int argc = 0;
		for (auto argvp = argv; *argvp; ++argc, ++argvp);
		parse(argc, argv, mode);
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename TChar>
	inline void parser::parse(int argc, const TChar * const argv[], int mode /*= PREFER_FLAG_FOR_UNREG_OPTION*/)
	{
		// convert to wstrings
		args_.resize(argc);
		std::transform(argv, argv + argc, args_.begin(), [](const wchar_t* const arg) { return arg;  });

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
				if (equalPos != std::wstring::npos)
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
				std::wstring keep_param;

				if (!name.empty() && is_param(std::wstring(1ul, name.back()))) // last wchar_t is param
				{
					keep_param += name.back();
					name.resize(name.size() - 1);
				}

				for (auto const& c : name)
				{
					flags_.emplace(std::wstring{ c });
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

	template<typename TString_Stream>
	inline TString_Stream parser::bad_stream() const
	{
		TString_Stream bad;
		bad.setstate(std::ios_base::failbit);
		return bad;
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename TString>
	inline bool parser::is_number(TString const& arg) const
	{
		// inefficient but simple way to determine if a wstring is a number (which can start with a '-')
		TString istr(arg);
		double number;
		istr >> number;
		return !(istr.fail() || istr.bad());
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename TString>
	inline bool parser::is_option(TString const& arg) const
	{
		assert(0 != arg.size());
		if (is_number(arg))
			return false;
		return '-' == arg[0];
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename TString>
	inline TString parser::trim_leading_dashes(TString const& name) const
	{
		auto pos = name.find_first_not_of('-');
		return TString::npos != pos ? name.substr(pos) : name;
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename TString>
	inline bool argh::parser::got_flag(TString const& name) const
	{
		return flags_.end() != flags_.find(trim_leading_dashes(name));
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename TString>
	inline bool argh::parser::is_param(TString const& name) const
	{
		return registeredParams_.count(name);
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename TString>
	inline bool parser::operator[](TString const& name) const
	{
		return got_flag(name);
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename TChar>
	inline bool parser::operator[](std::initializer_list<TChar const* const> init_list) const
	{
		return std::any_of(init_list.begin(), init_list.end(), [&](TChar const* const name) { return got_flag(name); });
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename TString>
	inline TString const& parser::operator[](size_t ind) const
	{
		if (ind < pos_args_.size())
			return pos_args_[ind];
		return empty_;
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename TString_Stream, typename TString>
	inline TString_Stream parser::operator()(TString const& name) const
	{
		auto optIt = params_.find(trim_leading_dashes(name));
		if (params_.end() != optIt)
			return TString_Stream(optIt->second);
		return bad_stream();
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename TString_Stream, typename TChar>
	inline TString_Stream parser::operator()(std::initializer_list<TChar const* const> init_list) const
	{
		for (auto& name : init_list)
		{
			auto optIt = params_.find(trim_leading_dashes(name));
			if (params_.end() != optIt)
				return TString_Stream(optIt->second);
		}
		return bad_stream();
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename TString_Stream, typename TString, typename T>
	inline TString_Stream parser::operator()(TString const& name, T && def_val) const
	{
		auto optIt = params_.find(trim_leading_dashes(name));
		if (params_.end() != optIt)
			return TString_Stream(optIt->second);

		std::owstringstream ostr;
		ostr << def_val;
		return TString_Stream(ostr.str()); // use default
	}

	//////////////////////////////////////////////////////////////////////////

	// same as above but for a list of names. returns the first value to be found.
	template<typename TString_Stream, typename TChar, typename T>
	inline TString_Stream parser::operator()(std::initializer_list<TChar const* const> init_list, T && def_val) const
	{
		for (auto& name : init_list)
		{
			auto optIt = params_.find(trim_leading_dashes(name));
			if (params_.end() != optIt)
				return TString_Stream(optIt->second);
		}
		std::owstringstream ostr;
		ostr << def_val;
		return TString_Stream(ostr.str()); // use default
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename TString_Stream>
	inline TString_Stream parser::operator()(size_t ind) const
	{
		if (pos_args_.size() <= ind)
			return bad_stream();

		return TString_Stream(pos_args_[ind]);
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename TString_Stream, typename T>
	inline TString_Stream parser::operator()(size_t ind, T && def_val) const
	{
		if (pos_args_.size() <= ind)
		{
			TString_Stream ostr;
			ostr << def_val;
			return TString_Stream(ostr.str());
		}

		return TString_Stream(pos_args_[ind]);
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename TString>
	inline void parser::add_param(TString const& name)
	{
		registeredParams_.insert(trim_leading_dashes(name));
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename TChar>
	inline void parser::add_params(std::initializer_list<TChar const* const> init_list)
	{
		for (auto& name : init_list)
			registeredParams_.insert(trim_leading_dashes(name));
	}
}

