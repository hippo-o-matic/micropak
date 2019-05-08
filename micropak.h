#define MAX_ARCHIVE_BUFFER_SIZE 5000000 //The maximum amount of memory to take for file extraction/compression (default 5 MB)

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <experimental/filesystem> // Change this to <filesystem> when g++ decides it's good enough 

struct archive_entry {
	bool isDir; // Is the entry a directory or not?
	std::string path; // The path to the file 
	long unsigned size; // The size of the data
};

// A class that combines and seperates files to and from .mpak files
namespace micropak {
	namespace fsys = std::experimental::filesystem;

	bool pack(std::string dirname, std::string outname = ""); // Pack a directory (dirname) into a single file (outname).mpak
	bool unpack(std::string filename, std::string outputdir = "."); // Unpack (filename).mpak into a directory (outputdir)

	unsigned short getVersion(std::string filename);
	unsigned short getVersion(std::fstream &stream);

	const unsigned short max_version = 1;
};

bool micropak::pack(std::string dirname, std::string outname) {
	fsys::path inDir = dirname; // Make a filesystem path
	if(!fsys::exists(inDir)) {
		std::cout << "ARCHIVE: Directory to pack doesn't exist" << std::endl;
		return false;
	}
	if(outname == "") {
		outname = dirname.substr(dirname.find_last_of('/') + 1) + ".mpak"; // Name the output file after the directory's name
	}
	// if(fsys::exists(outname)) {
	// 	std::cout << "ARCHIVE: File already exists" << std::endl;
	// 	return false;
	// }

	std::fstream result(outname.c_str(), std::ios::out | std::ios::binary | std::ios::trunc); // Create the resulting file
	if(!result) {
		std::cout << "ARCHIVE: Unable to write to \"" + outname + "\", do you have permissions?" << std::endl;
		return false;
	}

	std::vector<archive_entry> entries; // Vector of entries, like a table of contents before the actual file data
	for(auto &p: fsys::recursive_directory_iterator(fsys::absolute(inDir))){ // Recursively find each item in the directory
		std::string path = p.path().string(); // A string with the path name
		path.erase(0, path.find(dirname) + dirname.length() + 1); // The path we store should just be relative to the archive, not root (+1 gets rid of the slash)

		if(!fsys::is_directory(p.path())){ 
			entries.push_back(archive_entry {
				false,
				path,
				fsys::file_size(p.path())
			}); // Make an entry for the file

		} else { // File turned out to be a directory
			entries.push_back(archive_entry { 
				true,
				path,
				0
			});
		}
	}

	unsigned cursor = 0;
	result.seekp(std::ios::beg);
	result.write(reinterpret_cast<const char*>(&max_version), sizeof(unsigned short)); // Write the version
	cursor += sizeof(unsigned short);

	result.seekp(cursor);
	std::size_t entry_num = entries.size();
	result.write(reinterpret_cast<const char*>(&entry_num), sizeof(std::size_t));
	cursor += sizeof(std::size_t); // Write the amount of entries the archive has

	for(auto it = entries.begin(); it != entries.end(); it++) { // Store all entry metadata at the beginning of the archive
		result.seekp(cursor); // Seek to cursor position
		result.write(reinterpret_cast<const char*>(&it->isDir), sizeof(bool)); // Write the data value (in this case if the entry is a directory)
		cursor += sizeof(bool); // Advance cursor position

		size_t path_size = it->path.size() * sizeof(std::string::value_type); // Find the size of the path in bytes (currently each charachter in a string is 1 byte anyway, but this could change)
		result.seekp(cursor);
		result.write(reinterpret_cast<const char*>(&path_size), sizeof(size_t));
		cursor += sizeof(size_t);

		result.seekp(cursor);
		result.write(it->path.c_str(), path_size); // write the path relative to the root of the archive
		cursor += path_size;

		result.seekp(cursor);
		result.write(reinterpret_cast<const char*>(&it->size), sizeof(long unsigned)); // Write the size in bytes of the entry (0 if a directory)
		cursor += sizeof(long unsigned);
	}

	char* buffer = new char[MAX_ARCHIVE_BUFFER_SIZE];
	for(auto it = entries.begin(); it != entries.end(); it++){ // Get the filedata for each entry and append it
		if(!it->isDir && fsys::exists(dirname + '/' + it->path)){ 
			std::fstream input_file(dirname + '/' + it->path, std::ios::in | std::ios::binary);
			input_file.seekg(std::ios::beg);

			if(input_file) {
				for(float i = 0; i < ((float)it->size/(float)MAX_ARCHIVE_BUFFER_SIZE); i++) { // If the file is too big, seperate it into chunks
					if(i+1 > it->size/MAX_ARCHIVE_BUFFER_SIZE) { // If the data left doesn't fill a chunk, be more precise
						input_file.read(buffer, it->size - (MAX_ARCHIVE_BUFFER_SIZE * i));

						result.seekp(cursor);
						result.write(buffer, it->size - (MAX_ARCHIVE_BUFFER_SIZE * i)); // Put the filedata into the result file
						cursor += it->size - (MAX_ARCHIVE_BUFFER_SIZE * i);
					} else { // Otherwise just write in chunks of the max size
						input_file.read(buffer, MAX_ARCHIVE_BUFFER_SIZE);

						result.seekp(cursor);
						result.write(buffer, MAX_ARCHIVE_BUFFER_SIZE);
						cursor += MAX_ARCHIVE_BUFFER_SIZE;
					}
				}
			} else {
				std::cout << "ARCHIVE: Unable to append \"" + dirname + '/' + it->path + "\" to target file" << std::endl;
			}

			input_file.close();
		}
	}
	delete buffer;

	result.close();
	return true;
}

bool micropak::unpack(std::string filename, std::string outputdir){
	fsys::path inDir = filename;
	if(!fsys::exists(inDir)) {
		std::cout << "ARCHIVE: File to unpack doesn't exist!" << std::endl;
		return false;
	}

	if(!(outputdir.back() == '/'))
		outputdir += '/';

	std::fstream input_file(filename.c_str(), std::ios::in | std::ios::binary);
	unsigned int cursor = 0;

	unsigned short version = getVersion(input_file); // Read from the beginning of the file for the format version
	cursor += sizeof(unsigned short); // Move the cursor to the next data block


	if(version > max_version) { // Make sure we're capable of reading this format
		std::cout << "ARCHIVE: Unable to read archive - format is a newer version than reader" << std::endl;
		return false;
	}

	if(version == 1) {
		input_file.seekg(cursor);
		std::size_t entry_num = 0;
		input_file.read(reinterpret_cast<char*>(&entry_num), sizeof(std::size_t)); // Get the amount of entries
		cursor += sizeof(std::size_t);
		std::vector<archive_entry> entries(entry_num);

		// Get the entries from the archive header
		for(auto it = entries.begin(); it != entries.end(); it++) {
			input_file.seekg(cursor);
			input_file.read(reinterpret_cast<char*>(&it->isDir), sizeof(bool));
			cursor += sizeof(bool);

			size_t path_size = 0;
			input_file.seekg(cursor);
			input_file.read(reinterpret_cast<char*>(&path_size), sizeof(size_t));
			cursor += sizeof(size_t);
			
			char* extract = new char[path_size+1];

			input_file.seekg(cursor);
			input_file.read(extract, path_size);
			cursor += path_size;

			extract[path_size] = '\0';
			it->path = extract;
			delete extract;

			input_file.seekg(cursor);
			input_file.read(reinterpret_cast<char*>(&it->size), sizeof(long unsigned));
			cursor += sizeof(long unsigned);
		}

		// Now start creating files and directories based off of this entry
		char* buffer = new char[MAX_ARCHIVE_BUFFER_SIZE];
		for(auto it = entries.begin(); it != entries.end(); it++) {
			fsys::path path = outputdir + it->path;
			path.make_preferred();
			if (it->isDir){ // Create a directory if the entry is one
				fsys::create_directories(path);
			} else {
				std::fstream output_file(path.c_str(), std::ios::out | std::ios::binary);
				output_file.seekp(std::ios::beg);
				unsigned output_cursor = 0;

				if(output_file) {
					for(unsigned i = 0; i < (float)it->size/(float)MAX_ARCHIVE_BUFFER_SIZE; i++) { // If the file is too big, seperate it into chunks
						if(i+1 > it->size/MAX_ARCHIVE_BUFFER_SIZE) { // If the data left doesn't fill a chunk, be more precise
							input_file.seekg(cursor);
							input_file.read(buffer, it->size - (MAX_ARCHIVE_BUFFER_SIZE * i));
							cursor += it->size - (MAX_ARCHIVE_BUFFER_SIZE * i);

							output_file.seekp(output_cursor);
							output_file.write(buffer, it->size - (MAX_ARCHIVE_BUFFER_SIZE * i)); // Put the filedata into the result file
							output_cursor += it->size - (MAX_ARCHIVE_BUFFER_SIZE * i);
							*buffer = '\0'; // Remove the data from the buffer, but don't deallocate yet
						} else { // Otherwise just write in chunks of the max size
							input_file.seekg(cursor);
							input_file.read(buffer, MAX_ARCHIVE_BUFFER_SIZE);
							cursor += it->size;

							output_file.seekp(output_cursor);
							output_file.write(buffer, MAX_ARCHIVE_BUFFER_SIZE);
							output_cursor += MAX_ARCHIVE_BUFFER_SIZE;
							*buffer = '\0';
						}
					}
				} else {
					std::cout << "ARCHIVE: Unable to create file \"" + path.string() + "\"" << std::endl;
				}

				output_file.close();
			}
		}
		delete buffer;
		input_file.close();
		return true;
	}
	
	input_file.close();
	std::cout << "ARCHIVE: The archive didn't match any avalible extraction methods" << std::endl;
	return false; // The version didn't match any avalible extraction methods
}

unsigned short micropak::getVersion(std::string filename) {
	unsigned short version = 0;

	fsys::path inDir = filename;
	if(!fsys::exists(inDir)) {
		std::cout << "ARCHIVE: File to check doesn't exist!" << std::endl;
		return false;
	}

	std::fstream file(filename.c_str(), std::ios::in | std::ios::binary);
	if(file){
		file.seekg(0); 
		file.read(reinterpret_cast<char*>(&version), sizeof(unsigned short));
	}

	return version;
}

unsigned short micropak::getVersion(std::fstream &stream) {
	unsigned short version = 0;

	if(stream){
		stream.seekg(0); 
		stream.read(reinterpret_cast<char*>(&version), sizeof(unsigned short));
	}

	return version;
}
