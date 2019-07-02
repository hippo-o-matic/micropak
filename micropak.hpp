/*
Micropak 0.2 - made by hippo_o_matic

Permission is hereby granted, free of charge, to any person obtaining 
a copy of this software and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation the 
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
sell copies of the Software, and to permit persons to whom the Software is 
furnished to do so, subject to the following conditions:

The above notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#define MICROPAK_BUFFER_SIZE 500000000 // The maximum amount of memory to take for file extraction/compression in bytes (default 500 MB due to gzip limitations)
#define MICROPAK_USE_GZIP  // Remove this line if you don't want to use the gzip library (https://github.com/mapbox/gzip-hpp)

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <experimental/filesystem> // Change this to <filesystem> when g++ decides it's good enough 

#ifdef MICROPAK_USE_GZIP
#include "gzip/compress.hpp"
#include "gzip/decompress.hpp"
#endif


/* A decompressed micropak archive looks something like this structurally:
	--Header--
	Version number(unsigned short)
	Compression state(bool)

	Combined file data
	...

	--Footer--
	Number of meta entries(size_t)
	Number of file entries(size_t)
	Vector of meta entries
		Size of name string(size_t)
		Name string
		Size of content string(size_t)
		Content string
	Vector of file entries
		Directory identifier(bool)
		Size of path string(size_t)
		Path string(std::string)
		File size in bytes(size_t)

	Location of beginning of footer(size_t)

	(If compressed)
	Vector of compressed chunk positons(size_t)
	Number of positions(size_t)
 */

// A class that combines and seperates files to and from .mpak files
namespace micropak {
	namespace fsys = std::experimental::filesystem;

	struct archive_entry {
		bool isDir; // Is the entry a directory or not?
		std::string path; // The path to the file 
		size_t size; // The size of the data		
	};

	struct meta_entry { // Additional data stored in the file header
		std::string name;
		std::string value;
	};

	int pack(std::string dirname, 
		std::string outname = "",
#ifdef MICROPAK_USE_GZIP
		bool compress = true,
#else
		bool compress = false,
#endif
		std::vector<micropak::meta_entry> m_entries = {}
	); // Pack a directory (dirname) into a single file (outname).mpak

	std::vector<micropak::meta_entry> unpack(std::string filename, std::string outputdir = ""); // Unpack (filename).mpak into a directory (outputdir)

	unsigned short getVersion(std::string filename);
	unsigned short getVersion(std::ifstream &stream);

	bool getCompressed(std::string filename);
	bool getCompressed(std::ifstream &stream);

	const unsigned short max_version = 1;
	bool verbose = false;
};


int micropak::pack(std::string dirname, std::string outname, bool compress, std::vector<micropak::meta_entry> m_entries) {
	fsys::path inDir = dirname; // Make a filesystem path
	if(!fsys::exists(inDir)) {
		std::cout << "MICROPAK: Directory to pack doesn't exist" << std::endl;
		return 0;
	}
	if(outname == "") {
		outname = dirname + ".mpak"; // Name the output file after the directory's name
	}

	std::remove(outname.c_str());
	std::ofstream output(outname.c_str(), std::ios::binary | std::ios::app); // Create the outputing file, ios::app seeks to eof before writes
	if(!output) {
		std::cout << "MICROPAK: Unable to write to \"" + outname + "\", do you have permissions to write?" << std::endl;
		return 0;
	}

	std::vector<archive_entry> entries; // Vector of entries, like a table of contents before the actual file data
	for(auto &p: fsys::recursive_directory_iterator(fsys::absolute(inDir))){ // Recursively find each item in the directory
		std::string path = p.path().string(); // A string with the path name
		path.erase(0, fsys::absolute(inDir).string().size() + 1); // The path we store should just be relative to the archive, not root (+1 gets rid of the slash)

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
	if(verbose) 
			printf("Scanning directory- %lu elements found\n", entries.size());


	// Write a small header with version and compression info
	//////////////////////////////////////
	output.write(reinterpret_cast<const char*>(&max_version), sizeof(unsigned short)); // Write the version
	output.write(reinterpret_cast<const char*>(&compress), sizeof(bool)); // Write whether or not the rest of the file is compressed

	// Write the archive data
	//////////////////////////////////////		
	char* buffer = new char[MICROPAK_BUFFER_SIZE];
	for(auto it = entries.begin(); it != entries.end(); it++){ // Get the filedata for each entry and append it
		if(!it->isDir && fsys::exists(dirname + '/' + it->path)){ 
			std::ifstream input(dirname + '/' + it->path, std::ios::binary);
			unsigned int i_cursor = std::ios::beg; // Create a cursor for the input file

			if(input) {
				for(float i = 0; i < ((float)it->size/(float)MICROPAK_BUFFER_SIZE); i++) { // If the file is too big, seperate it into chunks
					if(verbose) {
						printf("Attaching file %lu of %lu- \"%s\" (chunk %.0g of %.0g)\n",
						it - entries.begin() + 1,
						entries.size(),
						(dirname + '/' + it->path).c_str(),
						i + 1,
						std::ceil((float)it->size/(float)MICROPAK_BUFFER_SIZE));
					}
				
					if(i+1 > it->size/MICROPAK_BUFFER_SIZE) { // If the data left doesn't fill a chunk, be more precise
						input.seekg(std::ios::beg);
						input.read(buffer, it->size - (MICROPAK_BUFFER_SIZE * i));
						i_cursor += it->size - (MICROPAK_BUFFER_SIZE * i);
										
						output.write(buffer, it->size - (MICROPAK_BUFFER_SIZE * i)); // Put the filedata into the output file
					} else { // Otherwise just write in chunks of the max size
						input.seekg(std::ios::beg);
						input.read(buffer, MICROPAK_BUFFER_SIZE);
						i_cursor += MICROPAK_BUFFER_SIZE;

						output.write(buffer, MICROPAK_BUFFER_SIZE);
					}
				}
			} else {
				std::cout << "MICROPAK: Unable to append \"" + dirname + '/' + it->path + "\" to target file" << std::endl;
			}
		}
	}

	// Write the archive footer with entries and meta tags
	//////////////////////////////////////
	size_t footer_start = output.tellp(); // Remember where the footer starts

	std::size_t meta_num = m_entries.size();
	output.write(reinterpret_cast<const char*>(&meta_num), sizeof(std::size_t)); // Write the amount of meta tags the archive has

	std::size_t entry_num = entries.size();
	output.write(reinterpret_cast<const char*>(&entry_num), sizeof(std::size_t)); // Write the amount of entries the archive has

	for(auto it = m_entries.begin(); it != m_entries.end(); it++){
		size_t name_size = it->name.size() * sizeof(std::string::value_type);
		output.write(reinterpret_cast<const char*>(&name_size), sizeof(size_t)); // Write the size of the name string in bytes
		output.write(it->name.c_str(), name_size); // Write the content of the tag

		size_t value_size = it->value.size() * sizeof(std::string::value_type);
		output.write(reinterpret_cast<const char*>(&value_size), sizeof(size_t)); // Write the size of the value string in bytes
		output.write(it->value.c_str(), value_size); 
	}

	for(auto it = entries.begin(); it != entries.end(); it++) { // Store all entry data at the end of the archive, now that we know compressed file sizes
		output.write(reinterpret_cast<const char*>(&it->isDir), sizeof(bool)); // Write the data value (in this case if the entry is a directory)
	
		size_t path_size = it->path.size() * sizeof(std::string::value_type); // Find the size of the path in bytes (currently each charachter in a string is 1 byte anyway, but this could change)
		output.write(reinterpret_cast<const char*>(&path_size), sizeof(size_t));

		output.write(it->path.c_str(), path_size); // write the path relative to the root of the archive
		output.write(reinterpret_cast<const char*>(&it->size), sizeof(size_t)); // Write the size in bytes of the entry (0 if a directory)
	}

	// Finally, write the location that the footer starts relative to the end
	footer_start = (size_t)output.tellp() - footer_start + sizeof(size_t);
	output.write(reinterpret_cast<const char*>(&footer_start), sizeof(size_t));
	// Wait, not finally! We still have to compress the darn thing! (If you want)

	// Compress the archive, excluding the header
	//////////////////////////////////////
	if(compress) {
#ifndef MICROPAK_USE_GZIP
		std::cout << "MICROPAK: Cannot compress when gzip support disabled, please set compress argument to false" << std::endl;
		delete buffer;
		return 0;
#endif
#ifdef MICROPAK_USE_GZIP
		size_t output_size = output.tellp(); // Save the uncompressed file size
		output.close();

		std::vector<size_t> chunk_positions; // Record the position of each chunk after compression so when we decompress them later they stay in that max chunk size
		chunk_positions.push_back(std::ios::beg + sizeof(unsigned short) + sizeof(bool)); // First chunk starts after header

		std::ifstream uncompressed_file(outname.c_str(), std::ios::binary); // Open the output file in an input stream
		std::remove((outname + ".gz").c_str());
		std::ofstream compressed_file((outname + ".gz").c_str(), std::ios::binary | std::ios::app); // Make a file to store the compressed data in

		uncompressed_file.seekg(std::ios::beg);
		uncompressed_file.read(buffer, sizeof(unsigned short) + sizeof(bool)); // Copy the header from the archive to the compressed one
		compressed_file.write(buffer, sizeof(unsigned short) + sizeof(bool));

		uncompressed_file.seekg(std::ios::beg + sizeof(unsigned short) + sizeof(bool)); // Seek past the header
		
		for(unsigned i = 0; i < ((float)output_size/(float)MICROPAK_BUFFER_SIZE); i++) { // If the file is too big, seperate it into chunks
			if(verbose) {
				printf("Compressing archive, chunk %u of %.0g\n",
				i + 1,
				std::ceil((float)output_size/(float)MICROPAK_BUFFER_SIZE));
			}
			
			if(i+1 > output_size/MICROPAK_BUFFER_SIZE) { // If the data left doesn't fill a chunk, be more precise
				uncompressed_file.read(buffer, output_size - (MICROPAK_BUFFER_SIZE * i)); // Get the chunk to compress
				std::string compressed_buffer = gzip::compress(buffer, output_size - (MICROPAK_BUFFER_SIZE * i));
				compressed_file.write(compressed_buffer.data(), compressed_buffer.size() * sizeof(std::string::value_type)); // Write the compressed data
				chunk_positions.push_back(compressed_file.tellp()); // Get the position the chunk ends at

			} else { // Otherwise just write in chunks of the max size
				uncompressed_file.read(buffer, MICROPAK_BUFFER_SIZE); // Get the chunk to compress
				// uncompressed_file.seekg(static_cast<size_t>(uncompressed_file.tellg()) + MICROPAK_BUFFER_SIZE, std::ios::cur); // Advance the read position
				std::string compressed_buffer = gzip::compress(buffer, MICROPAK_BUFFER_SIZE);
				compressed_file.write(compressed_buffer.data(), compressed_buffer.size() * sizeof(std::string::value_type)); // Write the compressed data
				chunk_positions.push_back(compressed_file.tellp()); // Get the position the chunk ends at
			}	
		}

		for(auto it = chunk_positions.begin(); it != chunk_positions.end(); it++) { // Write the positions of the end of each chunk
			compressed_file.write(reinterpret_cast<const char*>(&(*it)), sizeof(size_t));
		}
		size_t chunk_num = chunk_positions.size();
		compressed_file.write(reinterpret_cast<const char*>(&chunk_num), sizeof(size_t)); // Finally (for real this time), write the number of chunks to extract when decompressing
		std::remove(outname.c_str()); // Remove the non-compressed archive
#endif
	}

	delete buffer;
	return 1;
}



// Unpacks (and decompresses if necessary) an archive <filename>, and puts it at <outputdir>.
// Returns a vector of <micropak::meta_entry>'s the file contained
std::vector<micropak::meta_entry> micropak::unpack(std::string filename, std::string outputdir){
	fsys::path inDir = filename;
	if(!fsys::exists(inDir)) {
		std::cout << "MICROPAK: File to unpack doesn't exist!" << std::endl;
		return {};
	}

	if(!(outputdir.back() == '/') && !outputdir.empty()) // Format the path correctly, add a slash at the end
		outputdir += '/';

	if(outputdir.empty()) {
		outputdir = filename.substr(filename.find(".mpak")); // If the archive has any suffixes, remove them
		outputdir = outputdir.substr(outputdir.find(".gz"));
	}

	std::ifstream input(filename.c_str(), std::ios::binary);

	unsigned short version = getVersion(input); // Read from the beginning of the file for the format version
	input.seekg(sizeof(unsigned short), std::ios::cur); // Move the cursor to the next data block

	if(version > max_version) { // Make sure we're capable of reading this format
		std::cout << "MICROPAK: Unable to read archive - format is a newer version than reader" << std::endl;
		return {};
	}

	//EXTRACTION METHOD v1
	//////////////////////
	if(version == 1) {
		fsys::create_directories(outputdir); // Make the output path if it doesn't exist yet

		if(getCompressed(input)) { // If the file is compressed
#ifndef MICROPAK_USE_GZIP // Make sure we can decompress it
			std::cout << "MICROPAK: Unable to read archive - the gzip library is disabled for this application and archive is compressed" << std::endl;
			return {};
#endif
#ifdef MICROPAK_USE_GZIP
			size_t chunk_num = 0;

			input.seekg(-(sizeof(size_t)), std::ios::end);
			input.read(reinterpret_cast<char*>(&chunk_num), sizeof(size_t));

			std::vector<size_t> chunk_positions(chunk_num);
			input.seekg(-((chunk_num + 1) * sizeof(size_t)), std::ios::cur); // Seek to the beginning of the chunk positions, + 1 for passing the chunk position number first
			// Get the chunk positions from the file
			for(auto it = chunk_positions.begin(); it != chunk_positions.end(); it++) { 
				input.read(reinterpret_cast<char*>(&(*it)), sizeof(size_t));
			}

			char* buffer = new char[MICROPAK_BUFFER_SIZE];

			std::remove((filename + ".temp").c_str());
			std::ofstream decompressed(std::string(filename + ".temp").c_str(), std::ios::binary | std::ios::app);
			
			input.seekg(std::ios::beg);
			input.read(buffer, sizeof(unsigned short) + sizeof(bool));
			decompressed.write(buffer, sizeof(unsigned short) + sizeof(bool)); // Copy the header of the compressed archive to the new one
			
			input.seekg(sizeof(unsigned short) + sizeof(bool), std::ios::beg); // Seek past the header, to the compressed data

			// Begin extracting, decompressing, and restoring data
			for(auto it = chunk_positions.begin(); it != --chunk_positions.end(); it++) {
				if(verbose) {
					printf("Decompressing archive, chunk %lu of %lu\n",
					it - chunk_positions.begin() + 1,
					chunk_positions.size() - 1);
				}

				input.seekg(*it);
				input.read(buffer, *(it + 1) - *it); // Subtract the starting position with the next one to get a size
				std::string decompressed_data = gzip::decompress(buffer, *(it + 1) - *it); // Decompress the chunk
				decompressed.write(decompressed_data.data(), decompressed_data.size() * sizeof(std::string::value_type) - 3); // Write decompressed data to another file
			}

			input.close(); // Now switch the filestreams so we can continue the rest of unpacking
			decompressed.close();
			input.open(std::string(filename + ".temp").c_str());
#endif
		}


		// Find and seek to the start of the footer
		input.seekg(-sizeof(size_t), std::ios::end);
		size_t footer_start = 0;
		input.read(reinterpret_cast<char*>(&footer_start), sizeof(size_t)); 
		input.seekg(-footer_start, std::ios::end);

		size_t meta_num = 0;
		input.read(reinterpret_cast<char*>(&meta_num), sizeof(size_t)); // Get the amount of meta entries

		size_t entry_num = 0;
		input.read(reinterpret_cast<char*>(&entry_num), sizeof(std::size_t)); // Get the amount of entries

		std::vector<meta_entry> m_entries(meta_num);
		std::vector<archive_entry> entries(entry_num);

		for(auto it = m_entries.begin(); it != m_entries.end(); it++){ // Extract the meta tags
			size_t name_size = 0;
			input.read(reinterpret_cast<char*>(&name_size), sizeof(size_t)); // Get the size of the name string

			char* name_extract = new char[name_size+1]; // Create a buffer for the char data
			input.read(name_extract, name_size); // Read data into buffer
			
			name_extract[name_size] = '\0'; // Null-terminate the buffer data
			it->name = name_extract; // Assign the buffer data to storage

			// Do the same for the value assigned to the name
			size_t value_size = 0;
			input.read(reinterpret_cast<char*>(&value_size), sizeof(size_t));

			char* value_extract = new char[value_size+1];
			input.read(value_extract, value_size); 

			value_extract[value_size] = '\0'; 
			it->value = value_extract; 

			delete name_extract;
			delete value_extract;
		}

		// Get the entries from the archive footer
		for(auto it = entries.begin(); it != entries.end(); it++) {
			input.read(reinterpret_cast<char*>(&it->isDir), sizeof(bool));

			size_t path_size = 0;
			input.read(reinterpret_cast<char*>(&path_size), sizeof(size_t));
			
			char* extract = new char[path_size+1];
			input.read(extract, path_size);

			extract[path_size] = '\0';
			it->path = extract;
			delete extract;

			input.read(reinterpret_cast<char*>(&it->size), sizeof(size_t));
		}

		// Now start creating files and directories based off of these entries
		input.seekg(sizeof(unsigned short) + sizeof(bool)); // Go to where data starts

		char* buffer = new char[MICROPAK_BUFFER_SIZE];
		for(auto it = entries.begin(); it != entries.end(); it++) {
			fsys::path path = outputdir + it->path;
			path.make_preferred();
			if (it->isDir){ // Create a directory if the entry is one
				fsys::create_directories(path);
			} else {
				std::remove(path.c_str());
				std::ofstream output(path.c_str(), std::ios::binary | std::ios::app);
				output.seekp(std::ios::beg);

				if(output) {
					// If the file is too big, seperate it into chunks
					for(unsigned i = 0; i < (float)it->size/(float)MICROPAK_BUFFER_SIZE; i++) {
						if(verbose) {
							printf("Creating file %lu of %lu- \"%s\" (chunk %u of %.0g)\n",
							it - entries.begin() + 1,
							entries.size(),
							path.c_str(),
							i + 1,
							std::ceil((float)it->size/(float)MICROPAK_BUFFER_SIZE));
						}
						
						if(i+1 > it->size/MICROPAK_BUFFER_SIZE) { // If the data left doesn't fill a chunk, be more precise
							input.read(buffer, it->size - (MICROPAK_BUFFER_SIZE * i));
							output.write(buffer, it->size - (MICROPAK_BUFFER_SIZE * i)); // Put the filedata into the output file
							*buffer = '\0'; // Remove the data from the buffer, but don't deallocate yet
						} else { // Otherwise just write in chunks of the max size
							input.read(buffer, MICROPAK_BUFFER_SIZE);
							output.write(buffer, MICROPAK_BUFFER_SIZE);
							*buffer = '\0';
						}
					}
				} else {
					std::cout << "MICROPAK: Unable to create file \"" + path.string() + "\"" << std::endl;
				}
			}
		}

		std::remove((filename + ".temp").c_str());
		delete buffer;
		return m_entries;
	}
	
	std::cout << "MICROPAK: The archive didn't match any avalible extraction methods" << std::endl;
	return {}; // The version didn't match any avalible extraction methods
}



unsigned short micropak::getVersion(std::string filename) {
	unsigned short version = 0;

	fsys::path inDir = filename;
	if(!fsys::exists(inDir)) {
		std::cout << "MICROPAK: File to check doesn't exist!" << std::endl;
		return 0;
	}

	std::ifstream file(filename.c_str(), std::ios::binary);
	if(file){
		file.seekg(0); 
		file.read(reinterpret_cast<char*>(&version), sizeof(unsigned short));
	}

	return version;
}

unsigned short micropak::getVersion(std::ifstream &stream) {
	unsigned short version = 0;

	if(stream){
		stream.seekg(0); 
		stream.read(reinterpret_cast<char*>(&version), sizeof(unsigned short));
	}

	return version;
}

bool micropak::getCompressed(std::string filename) {
	bool isCompressed;

	fsys::path inDir = filename;
	if(!fsys::exists(inDir)) {
		std::cout << "MICROPAK: File to check doesn't exist!" << std::endl;
		return 0;
	}

	std::ifstream file(filename.c_str(), std::ios::binary);
	if(file){
		file.seekg(sizeof(unsigned short)); 
		file.read(reinterpret_cast<char*>(&isCompressed), sizeof(bool));
	}

	return isCompressed;
}

bool micropak::getCompressed(std::ifstream &stream) {
	bool isCompressed;

	if(stream){
		stream.seekg(sizeof(unsigned short)); 
		stream.read(reinterpret_cast<char*>(&isCompressed), sizeof(bool));
	}

	return isCompressed;
}
