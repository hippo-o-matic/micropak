#include "micropak.h"

int main(int argc, char* argv[]) {
	std::string option = argv[1];

	if(argc == 3){ // Output path not specified
		if(option == "-p") {
			micropak::pack(argv[2]);
			return 1;
		} else if(option == "-u") {
			micropak::unpack(argv[2]);
			return 1;
		}
	} else if(argc == 4){ // Output path specified
		if(option == "-p") {
			micropak::pack(argv[2], argv[3]);
			return 1;
		} else if(option == "-u") {
			micropak::unpack(argv[2], argv[3]);
			return 1;
		}
	} else { // Display help message if argument amount doesn't match
		std::cout << 
			"Micropak v0.1\n"
			"A (not very good) basic file archiving program\n"
			"Options:\n"
			"-p <directory> <file> | Recursively packs all files in <directory> to <file>. Uses <directory>.mpak if <file> not given\n"
			"-u <file> <directory> | Unpacks an archive, <file>, using <directory> as it's root. Uses <file> as the name if <directory> not given\n"
			"-h, --help | Displays this message :)" 
		<< std::endl;
	}
	
	return 0;
}