#include "micropak.hpp"
#include <algorithm>

// Parses strings into a vector of strings seperated by the seperator character
std::vector<std::string> segment(std::string &in, char seperator) {
	size_t last = 0;
	size_t pos = 0;
	std::vector<std::string> output;

	if (in.empty()) 
		return output;
	
	while (pos != std::string::npos) {
		pos = in.find(seperator, last);
		if(in.find('\\', last) == (pos - 1)) {
			last = pos + 1;
		} else {
			output.push_back(in.substr(last, pos - last));
			last = pos + 1;
		}
	}

	return output;
}

int main(int argc, char* argv[]) {

	std::vector<std::string> options(argv + 1, argv + argc);
	auto p = std::find(options.begin(), options.end(), "-p");
	auto u = std::find(options.begin(), options.end(), "-u");
	auto o = std::find(options.begin(), options.end(), "-o");
	auto m = std::find(options.begin(), options.end(), "-m");
	auto v = std::find(options.begin(), options.end(), "-v");
	auto no_gzip = std::find(options.begin(), options.end(), "--no-gzip");

	std::vector<micropak::meta_entry> m_entries = {};

	if(p != options.end() && u != options.end()) {
		std::cout << "MICROPAK: Cannot accept both -u and -p at the same time." << std::endl;
		return 0;
	}

	if(o != options.end() && (o+1) == options.end()) {
		std::cout << "No output location specified after -o" << std::endl;
		return 0;
	}

	if(p != options.end() && (p+1) == options.end()) {
		std::cout << "No directory specified after -p" << std::endl;
		return 0;
	}

	if(u != options.end() && (p+1) == options.end()) {
		std::cout << "No directory specified after -u" << std::endl;
		return 0;
	}

	if(m != options.end()) {
		std::vector<std::string> list = segment(*(m + 1), ',');
		for(unsigned i=0; i < (list.size()); i++) {
			m_entries.push_back(
				{segment(list[i],'=')[0],
				segment(list[i],'=')[1]}
			);
		}
	}

	if(v != options.end()) {
		micropak::verbose = true;
	}

	if(p != options.end()) {
		if(o != options.end()) {
			if(no_gzip != options.end()) {
				return micropak::pack(*(p+1), *(o+1), false, m_entries);
			} else {
				return micropak::pack(*(p+1), *(o+1), true, m_entries);
			}
		} else {
			if(no_gzip != options.end()) {
				return micropak::pack(*(p+1), "", false, m_entries);
			} else {
				return micropak::pack(*(p+1), "", true, m_entries);
			}
		}
	}
	
	if(u != options.end()) {
		std::vector<micropak::meta_entry> result;

		if(o == options.end()) {
			result = micropak::unpack(*(u+1));
		} else {
			result = micropak::unpack(*(u+1), *(o+1));
		}

		if(result.empty()) {
			return 0;
		} else {
			std::cout << result.size() << " metatags found:" << std::endl;
			for(auto it = result.begin(); it != result.end(); it++) {
				std::cout << (*it).name << " = " << (*it).value << std::endl;
			}
			return 1;
		}
	}

	
	// Display help message if arguments don't match
	std::cout << 
		"Micropak v0.2\n"
		"A (not very good) basic file archiving program\n"
		"Options:\n"
		"-p <directory>	| Recursively packs all files in <directory> to -o <file>. Uses <directory>.mpak if -o <file> not given\n"
		"-u <file>	| Unpacks an archive, <file>, using -o <directory> as it's root. Uses <file> as the name if -o <directory> not given\n"
		"-o <file/directory>	| Outputs the result of -p or -u to <file/directory>\n"
		"-m <name=content,name=content>	| Attaches given meta tag pairs, the name and content of each tag seperated by '=', and each pair seperated by ','\n"
		"-v	| Enables verbose mode, reports back what is being done for debugging purposes\n"
		"--no-gzip	| Packs a file without gzipping it\n"
		"-h, --help	| Displays this message :)"
	<< std::endl;
	return 0;
}