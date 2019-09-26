/* File Compression
********************************************************************
Created by  :  CynusW
Date        :  30 Aug 2019
Description :  This program compresses multiple files into one.
********************************************************************
*/

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>
#include "fc.h"

#define SIZE(x) (sizeof x / sizeof x[0])
#define TOLOWER(x) (std::transform(x.begin(), x.end(), x.begin(), [](char c) { return std::tolower(c); }))

namespace fs = std::filesystem;

/*
The namespace 'cl' is an important part of the program since it is for processing command line arguments.
*/
namespace cl
{
	/*
	Represents a Flag/Option argument. It is like std::pair but the members were renamed so it can be more convenient.
	*/
	struct Flag
	{
		std::string name;		// The name of the flag; can be long or short
		std::string value;	// The value the flag takes in string
	};

	/*
	This is used for knowing the supported flags in the program. If a flag was not supported, we can just skip it.
	Another thing is to know whether the flag takes a value or not.
	*/
	struct FlagInfo
	{
		std::string name;			// The shorter name of the flag	(-h)
		std::string longName;		// The long name of the flag	(--help)
		bool useValue;			// Whether the flag uses a value or not
	};

	/*
	The variable 'programDirectory' is of course the directory of the program.
	The variable 'arguments' and 'flags' are used for storing the command line arguments in different categories.
	The variable 'flags' contain given arguments preceded by one or two hypens (-). It is also called options.
	The variable 'arguments' contain the rest of the given arguments.

	*****************************************************************************************************
	These variables don't contain anything until the ProcessArguments function is called successfully.
	*****************************************************************************************************
	*/
	fs::path programPath;
	std::vector<std::string> arguments;
	std::vector<Flag> flags;

	/*
	The ProcessArguments function is self-explanatory; it proccesses the given arguments.
	The third parameter is used for filtering out unsupported flags and determining the type of flags.
	*/
	void ProcessArguments(int argc, char* argv[], const FlagInfo* fiInfos, int fiSize)
	{
		// The first argument will always be the program name.
		cl::programPath = fs::path(argv[0]);

		for (int i = 1; i < argc; i++)
		{
			std::string arg = argv[i];

			// If the argument is a flag/option, filter it, process it and push it to the 'flags' variable.
			if (arg.front() == '-')
			{
				Flag flag = { };

				if (arg[1] == '-')
				{
					arg = arg.substr(2);
				}

				else
				{
					arg = arg.substr(1);
				}

				// Transform all the characters in the string to lowercase.
				TOLOWER(arg);

				for (int j = 0; j < fiSize; j++)
				{
					const FlagInfo& fiInfo = fiInfos[j];

					if (arg == fiInfo.name || arg == fiInfo.longName)
					{
						flag.name = arg;
					}
					else continue;

					if (fiInfo.useValue)
					{
						// Assign the next argument as the flag value if there's any.
						if ((i + 1) < argc)
						{
							if (argv[i + 1][0] != '-')
							{
								flag.value = argv[++i];
							}
						}
					}

					flags.push_back(flag);
					break;
				}
			} else arguments.push_back(arg);
		}
	}

	/*
	A function to determine whether there is a specified flag in the given arguments.
	*/
	bool hasFlag(const FlagInfo& fiInfo)
	{
		for (const Flag& flag : flags)
		{
			if (flag.name == fiInfo.name || flag.name == fiInfo.longName)
			{
				return true;
			}
		}

		return false;
	}

	/*
	A function to determine whether there is a specified flag in the given arguments by name.
	*/
	bool hasFlag(const std::string&& name)
	{
		for (const Flag& flag : flags)
		{
			if (flag.name == name)
			{
				return true;
			}
		}

		return false;
	}
}

inline void DisplayHelp()
{
	std::cerr
		<< "Usage: " << cl::programPath.filename() << " [-o dest] file1 file2 [...fileN]\n"
		<< "       " << cl::programPath.filename() << " -dc [-o dest] archiveFile\n"
		<< "Options:\n"
		<< "  -dc --decompress		Set mode to decompression.\n"
		<< "  -h  --help			Shows this message.\n"
		<< "  -o  --out			The destination of the new file.\n";
}

int main(int argc, char* argv[])
{
	/*
	The fiInfos array contains all the flags supported in the program.
	This is will used for getting all the informations needed about the supported flags.
	*/
	cl::FlagInfo fiInfos[] = {
		{ "dc", "decompress", false },
		{ "h", "help", false },
		{ "o", "out", true }
	};

	cl::ProcessArguments(argc, argv, fiInfos, SIZE(fiInfos));

	if (cl::arguments.size() < 2 && !cl::hasFlag(fiInfos[0]) ||
		((cl::arguments.size() < 1) && cl::hasFlag(fiInfos[0])) ||
		cl::hasFlag(fiInfos[1]))
	{
		DisplayHelp();
		return 1;
	}

	fc::FileCompressor fc;
	if (cl::hasFlag(fiInfos[0]))
	{
		fc.decompress(cl::arguments[0]);
	}

	else
	{
		std::for_each(cl::arguments.begin(), cl::arguments.end(),
			[&](std::string& file) {
				fc.add(file);
			});

		if (cl::hasFlag(fiInfos[2]))
		{
			std::string& out = std::find_if(cl::flags.begin(), cl::flags.end(),
				[&](cl::Flag& flag) { return flag.name == "o"; })[0].value;
			fc.compress(out);
		}

		else fc.compress("out.zp2");
	}

	return 0;
}
