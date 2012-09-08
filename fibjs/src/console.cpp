/*
 * console.cpp
 *
 *  Created on: Apr 7, 2012
 *      Author: lion
 */

#include "ifs/console.h"
#include "ifs/assert.h"
#include "ifs/encoding.h"
#include <log4cpp/Category.hh>
#include <sstream>

#ifdef _WIN32
#include <windows.h>

static LARGE_INTEGER systemFrequency;

inline int64_t Ticks()
{
	LARGE_INTEGER t;

	if(systemFrequency.QuadPart == 0)
	QueryPerformanceFrequency(&systemFrequency);

	QueryPerformanceCounter(&t);

	return t.QuadPart * 1000000 / systemFrequency.QuadPart;
}

#else
#include <sys/time.h>

inline int64_t Ticks()
{
	struct timeval tv;
	if (gettimeofday(&tv, NULL) < 0)
		return 0;
	return (tv.tv_sec * 1000000ll) + tv.tv_usec;
}

#endif

#include "File.h"
#include "BufferedStream.h"

namespace fibjs
{

static obj_ptr<BufferedStream_base> s_in, s_out;

result_t console_base::get_in(obj_ptr<BufferedStream_base>& retVal)
{
	if (s_in == NULL)
	{
		s_in = new BufferedStream(new File(stdin, true));
		s_in->set_EOL("\n");
	}

	retVal = s_in;

	return 0;
}

result_t console_base::get_out(obj_ptr<BufferedStream_base>& retVal)
{
	if (s_out == NULL)
	{
		s_out = new BufferedStream(new File(stdout, true));
		s_out->set_EOL("\n");
	}

	retVal = s_out;

	return 0;
}

std::string Format(const char* fmt, const v8::Arguments& args, int idx = 1)
{
	const char* s = fmt;
	const char* s1;
	char ch;
	int argc = args.Length();
	std::stringstream strBuffer;

	if (argc == 1)
		return fmt;

	while (1)
	{
		if (idx >= argc)
		{
			strBuffer << s;
			break;
		}

		s1 = s;
		while ((ch = *s++) && (ch != '%'))
			;

		strBuffer.write(s1, s - s1 - 1);

		if (ch == '%')
		{
			switch (ch = *s++)
			{
			case 's':
			case 'd':
				strBuffer << *v8::String::Utf8Value(args[idx++]);
				break;
			case 'j':
			{
				std::string s;
				encoding_base::jsonEncode(args[idx++], s);
				strBuffer << s;
			}
				break;
			default:
				strBuffer << '%';
			case '%':
				strBuffer << ch;
				break;
			}
		}
		else
			break;
	}

	while (idx < argc)
		strBuffer << ' ' << *v8::String::Utf8Value(args[idx++]);

	return strBuffer.str();
}

result_t console_base::log(const char* fmt, const v8::Arguments& args)
{
	asyncLog(log4cpp::Priority::INFO, Format(fmt, args));
	return 0;
}

result_t console_base::info(const char* fmt, const v8::Arguments& args)
{
	asyncLog(log4cpp::Priority::INFO, Format(fmt, args));
	return 0;
}

result_t console_base::warn(const char* fmt, const v8::Arguments& args)
{
	asyncLog(log4cpp::Priority::WARN, Format(fmt, args));
	return 0;
}

result_t console_base::error(const char* fmt, const v8::Arguments& args)
{
	asyncLog(log4cpp::Priority::ERROR, Format(fmt, args));
	return 0;
}

result_t console_base::dir(v8::Handle<v8::Object> obj)
{
	std::string s;
	encoding_base::jsonEncode(obj, s);
	asyncLog(log4cpp::Priority::INFO, s);
	return 0;
}

static std::map<std::string, int64_t> s_timers;

result_t console_base::time(const char* label)
{
	s_timers[std::string(label)] = Ticks();

	return 0;
}

result_t console_base::timeEnd(const char* label)
{
	long t = (long) (Ticks() - s_timers[label]);

	s_timers.erase(label);

	std::stringstream strBuffer;

	strBuffer << label << ": " << (t / 1000.0) << "ms";

	asyncLog(log4cpp::Priority::INFO, strBuffer.str());

	return 0;
}

inline const char* ToCString(const v8::String::Utf8Value& value)
{
	return *value ? *value : "<string conversion failed>";
}

result_t console_base::trace(const char* label)
{
	std::stringstream strBuffer;

	strBuffer << "console.trace: " << label;
	strBuffer << traceInfo();

	asyncLog(log4cpp::Priority::WARN, strBuffer.str());

	return 0;
}

#ifdef assert
#undef assert
#endif

result_t console_base::assert(bool value, const char* msg)
{
	return assert_base::ok(value, msg);
}

}
;
