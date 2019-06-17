#include "micropak.h"
#include <algorithm>

int main(int argc, char* argv[]) {

	std::vector<std::string> options(argv + 1, argv + argc);
	auto p = std::find(options.begin(), options.end(), "-p");
	auto u = std::find(options.begin(), options.end(), "-u");
	auto o = std::find(options.begin(), options.end(), "-o");
	auto no_gzip = std::find(options.begin(), options.end(), "--no-gzip");

	if(p != options.end() && u != options.end()) {
		std::cout << "MICROPAK: Cannot accept both -u and -p at the same time." << std::endl;
		return 0;
	}

	if(p != options.end()) {
		if(o != options.end()) {
			if(no_gzip != options.end()) {
				return micropak::pack(*(p+1), *(o+1), false);
			} else {
				return micropak::pack(*(p+1), *(o+1));
			}
		} else {
			if(no_gzip != options.end()) {
				return micropak::pack(*(p+1), "", false);
			} else {
				return micropak::pack(*(p+1));
			}
		}
	}
	
	if(u != options.end()) {
		if(o != options.end()) {
			if(micropak::unpack(*(u+1), *(o+1)).empty()) {
				return 1;
			} else {
				return 0;
			}
		} else {
			if(micropak::unpack(*(u+1)).empty()) {
				return 1;
			} else {
				return 0;
			}
		}
	}

	
	// Display help message if arguments don't match
	std::cout << 
		"Micropak v0.1\n"
		"A (not very good) basic file archiving program\n"
		"Options:\n"
		"-p <directory> 	 | Recursively packs all files in <directory> to -o <file>. Uses <directory>.mpak if -o <file> not given\n"
		"-u <file> 			 | Unpacks an archive, <file>, using -o <directory> as it's root. Uses <file> as the name if -o <directory> not given\n"
		"-o <file/directory> | Outputs the result of -p or -u to <file/directory>\n"
		"--no-gzip 			 | Packs a file without gzipping it\n"
		"-h, --help 		 | Displays this message :)"
	<< std::endl;
	return 0;
}