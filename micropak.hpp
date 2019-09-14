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

#ifndef MICROPAK_H
#define MICROPAK_H

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
	extern bool verbose;
};

#endif