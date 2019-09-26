#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <algorithm>
#include <numeric>

#define BYTES_PER_CHUNK 0x100000

namespace dsl
{
	class ISerializable
	{
	public:
		virtual void serialize(std::ostream&) = 0;
		virtual void deserialize(std::istream&) = 0;
	};

	namespace Serializer
	{
		void serialize(ISerializable& serializable, std::string&& filename, int mode = (std::ios::binary | std::ios::out | std::ios::trunc))
		{
			std::ofstream file;
			file.open(filename, mode);

			serializable.serialize(file);
			file.close();
		}

		void deserialize(ISerializable& serializable, std::string&& filename, int mode = (std::ios::binary | std::ios::in))
		{
			std::ifstream file;
			file.open(filename, mode);

			serializable.deserialize(file);
			file.close();
		}
	};
}

namespace fc
{
	using fsize = long long;

	struct File : dsl::ISerializable
	{
		std::string path;			// The path to file.
		fsize size;					// The size of the file in bytes.
		std::streamoff offset;		// The stream offset of the file from the beginning.

		File() : path(), size(), offset() { }

		bool operator==(File& file)
		{
			return (this->path == file.path);
		}

		bool operator==(std::string& file)
		{
			return (this->path == file);
		}

		void serialize(std::ostream& out)
		{
			size_t len = path.size();
			out.write((char*)&len, sizeof(size_t));
			out.write(path.c_str(), len);
			out.write((char*)&size, sizeof(fsize));
			out.write((char*)&offset, sizeof(std::streamoff));
		}

		void deserialize(std::istream& in)
		{
			size_t len = 0;
			in.read((char*)&len, sizeof(size_t));

			if (path.size() < len)
			{
				path.resize(len, '\0');
			}

			in.read(&path[0], len);
			in.read((char*)&size, sizeof(fsize));
			in.read((char*)&offset, sizeof(std::streamoff));
		}
	};

	class FileCompressor
	{
	private:
		std::vector<fc::File> files;
	
	public:
		void add(std::string file)
		{
			if (!std::any_of(files.begin(), files.end(), [&](fc::File& f) { return f == file; }))
			{
				fc::File nFile = { };
				nFile.path = file;

				files.push_back(nFile);
			}
		}

		void add(fc::File file)
		{
			if (!std::any_of(files.begin(), files.end(), [&](fc::File& f) { return f == file; }))
			{
				files.push_back(file);
			}
		}

		void compress(std::string outFile)
		{
			std::ofstream out;
			out.open(outFile, std::ios::binary | std::ios::out | std::ios::trunc);

			std::for_each(
				files.begin(),
				files.end(),
				[&](fc::File& file) {
					std::ifstream inFile;
					inFile.open(file.path, std::ios::binary | std::ios::in | std::ios::ate);

					if (!inFile.is_open()) return;

					file.size = inFile.tellg();
					file.offset = out.tellp();

					char* buffer = new char[BYTES_PER_CHUNK];

					inFile.seekg(0, std::ios::beg);
					for (fsize fSize = 0; fSize < file.size; fSize += BYTES_PER_CHUNK)
					{
						fsize sz = (file.size - fSize) < BYTES_PER_CHUNK ? (file.size - fSize) : BYTES_PER_CHUNK;

						inFile.read(buffer, sz);
						out.write(buffer, sz);
					}

					inFile.close();
					delete[] buffer;
				});

			std::for_each(
				files.begin(),
				files.end(),
				[&](fc::File& file) {
					file.serialize(out);
				});

			fsize size = std::accumulate(
				files.begin(),
				files.end(),
				(fsize)0,
				[&](fsize val, fc::File& file) {
					return val + (fsize)file.path.size() + (fsize)sizeof(size_t) + (fsize)sizeof(fsize) + (fsize)sizeof(std::streamoff);
				});
			out.write((char*)&size, sizeof(fsize));
			out.close();
		}

		void decompress(std::string file)
		{
			fsize size = 0;
			std::ifstream in;

			std::clog << "Loading file: " << file << std::endl;

			in.open(file, std::ios::binary | std::ios::in | std::ios::ate);
			in.seekg(-(long long)sizeof(fsize), std::ios::cur);

			in.read((char*)&size, sizeof(fsize));
			in.seekg(-(long long)sizeof(fsize) - size, std::ios::end);

			fsize processed = 0;
			while (processed < size)
			{
				fc::File fl = { };
				fl.deserialize(in);
				
				std::ofstream out;
				out.open(fl.path, std::ios::binary | std::ios::out | std::ios::trunc);

				char* buffer = new char[BYTES_PER_CHUNK];

				in.seekg(fl.offset, std::ios::beg);
				for (fsize fSize = 0; fSize < fl.size; fSize += BYTES_PER_CHUNK)
				{
					std::clog
						<< "Extracting " << fl.path
						<< " (" << fSize / BYTES_PER_CHUNK << " / " << fl.size / BYTES_PER_CHUNK << ")\n";

					fsize sz = (fl.size - fSize) < BYTES_PER_CHUNK ? (fl.size - fSize) : BYTES_PER_CHUNK;

					in.read(buffer, sz);
					out.write(buffer, sz);
				}

				out.close();
				processed += (fsize)fl.path.size() + (fsize)sizeof(size_t) + (fsize)sizeof(fsize) + (fsize)sizeof(std::streamoff);

				delete[] buffer;
			}

			std::clog << "Successfully extracted the file: " << file << std::endl;

			in.close();
		}
	};
}