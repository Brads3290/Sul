//
// Created by Kim on 04/07/2016.
//

#ifndef PROJECT_FILING_H
#define PROJECT_FILING_H

#include <string>
#include <cstdio>
#include <fstream>
#include <memory>
#include <sstream>
#include <ctime>
#include <chrono>
#include "Shlwapi.h"
#include "Sul.h"

namespace Sul {
    namespace FileSystem {
        class BinaryFile;

        namespace Transformation {
            class TransformationBase {
            public:
                virtual std::string transform(std::string&) = 0;
            };
            class CompressionBase {
            public:
                virtual std::string compress(std::string&) = 0;
                virtual std::string uncompress(std::string&) = 0;
            };
            class EncryptionBase {
            public:
                virtual std::string encrypt(std::string&) = 0;
                virtual std::string decrypt(std::string&) = 0;
            };
        }
        namespace Format {
            template <class StorageType, class AccessType>
            class FormatBase {
            public:
                virtual void dataIn(StorageType) = 0;
                virtual StorageType dataOut(AccessType) const = 0;
                virtual StorageType& dataRef(AccessType) = 0;
                virtual std::unique_ptr<char[]> rawDataOut() = 0;
                virtual void rawDataIn(std::unique_ptr<char[]>&&) = 0;
                virtual void clearAll() = 0;
                virtual void clear(AccessType) = 0;
                virtual unsigned int size() = 0;
                virtual unsigned int segmentCount() = 0;
            };

            //Text: Stored as lines, accessed as line number
            class Text: public FormatBase<std::string, unsigned int> {
                std::vector<std::string> _lines;

            public:
                char _delim;

                Text(char delim = '\n') {
                    _delim = delim;
                }

                virtual void dataIn(std::string data) {
                    _lines.push_back(data);
                }
                virtual std::string dataOut(unsigned int lineNum) const {
                    return _lines[lineNum];
                };
                virtual std::string& dataRef(unsigned int lineNum) {
                    return _lines[lineNum];
                }
                virtual std::unique_ptr<char[]> rawDataOut() {
                    std::string buf;
                    for (int i = 0; i < _lines.size(); ++i) {
                        buf += _lines[i];

                        if (i < _lines.size() - 1) {
                            buf += _delim;
                        }
                    }

                    char* ret = new char[buf.length() + 1];
                    for (int j = 0; j < buf.length(); ++j) {
                        ret[j] = (char) buf[j];
                    }
                    ret[buf.length()] = 0;

                    return std::move(std::unique_ptr<char[]>(ret));
                }
                virtual void rawDataIn(std::unique_ptr<char[]>&& data) {
                    std::string strData(data.get());
                    if (strData.length() == 0) {
                        return;
                    }

                    _lines.push_back(data.get());
                }
                virtual void clearAll() {
                    for (int i = 0; i < _lines.size(); ++i) {
                        _lines.erase(_lines.begin());
                    }
                }
                virtual void clear(unsigned int index) {
                    _lines.erase(_lines.begin() + index);
                }
                virtual unsigned int size() {
                    unsigned long int count = 0;
                    for (int i = 0; i < _lines.size(); ++i) {
                        count += _lines[i].length() * sizeof(char);

                        if (i < _lines.size() - 1) {
                            count += sizeof(char); //Add the delimiter
                        }
                    }

                    return count;
                }
                virtual unsigned int segmentCount() {
                    return _lines.size();
                }
            };

            //Binary: Stored as bytes (chars) and accessed as a byte number.
            class Binary: public FormatBase<char, unsigned int> {
                std::vector<char> _data;

            public:
                virtual void dataIn(char data) {
                    _data.push_back(data);
                }
                virtual char dataOut(unsigned int index) const {
                    return _data[index];
                }
                virtual char& dataRef(unsigned int index) {
                    return _data[index];
                }
                virtual std::unique_ptr<char[]> rawDataOut() {
                    std::string buf;
                    for (int i = 0; i < _data.size(); ++i) {
                        buf += _data[i];
                    }

                    char* ret = new char[buf.length() + 1];
                    for (int j = 0; j < buf.length(); ++j) {
                        ret[j] = (char) buf[j];
                    }
                    ret[buf.length()] = 0;

                    return std::move(std::unique_ptr<char[]>(ret));
                }
                virtual void rawDataIn(std::unique_ptr<char[]>&& data) {
                    std::string buf = data.get();
                    for (int i = 0; i < buf.length(); ++i) {
                        _data.push_back(buf[i]);
                    }
                }
                virtual void clearAll() {
                    for (int i = 0; i < _data.size(); ++i) {
                        _data.erase(_data.begin());
                    }
                }
                virtual void clear(unsigned int i) {
                    _data.erase(_data.begin() + i);
                }
                virtual unsigned int size() {
                    return _data.size();
                }
                virtual unsigned int segmentCount() {
                    return _data.size();
                }
            };
        }

        void SetDLLPath(std::string);
        std::string GetDLLPath();

        class Base {
            friend void SetDLLPath(std::string);
            friend std::string GetDLLPath();

        protected:
            static DynamicLibrary DLL;
            class CallDLL {
            public:

            };

            template <class Type>
            void LoadProc(Type& ptr, std::string name) {
                ptr = (Type)DLL.getProcAddress(name);
            }

        public:
            Base() {
                if (!DLL.isDLLLoaded()) {
                    DLL.loadDLL();


                }
            }
        };
        DynamicLibrary Base::DLL = DynamicLibrary("SFileSystem.dll");

        void GetFullPathAndName(std::string relpath, std::string& name, std::string& path) {
            DWORD buflen = 0;
            LPTSTR bufOut = nullptr;
            TCHAR* pFileName = nullptr;
            DWORD res = 0;

            do {
                if (res > 0) {
                    buflen = res;
                } else {
                    buflen = 255;
                }

                if (bufOut) {
                    delete bufOut;
                }

                bufOut = new TCHAR[buflen];
                res = GetFullPathName(relpath.c_str(), buflen, bufOut, &pFileName);

                if (!res) {
                    throw std::runtime_error("GetFullPathName failed with error code " + std::to_string(GetLastError()));
                }
            } while (res > buflen);

            if (!pFileName) {
                throw std::runtime_error("GetFullPathAndName - cannot extract file name from path");
            }

            path = bufOut;
            name = pFileName;

            path = path.erase(path.find(name));

            delete bufOut;
            //Don't delete pFileName as it point to an address in bufOut.
        }
        std::string GetFileDirectoryAbs(std::string relpath) {
            std::string name, path;
            GetFullPathAndName(relpath, name, path);

            return path;
        }
        bool FileExists(std::string path) {
            std::ifstream in(path);
            bool ret = (bool) in;
            in.close();

            return ret;
        }
        bool DirExists(std::string path) {
            DWORD ftyp = GetFileAttributesA(path.c_str());
            if (ftyp == INVALID_FILE_ATTRIBUTES) {
                return false;
            }

            return (ftyp & FILE_ATTRIBUTE_DIRECTORY) != 0;

        }
        std::string GetFileDirectoryRel(std::string fullpath) {
            //Get the current directory
            DWORD cdir_buflen = 0;
            LPTSTR cdir_buf = nullptr;
            DWORD res = 0;
            do {
                cdir_buflen = 255;
                if (res > 0) {
                    cdir_buflen = res;
                }

                if (cdir_buf) {
                    delete cdir_buf;
                }

                cdir_buf = new TCHAR[cdir_buflen];
                res = GetCurrentDirectory(cdir_buflen, cdir_buf);
            } while (res > cdir_buflen);
            std::string cdir = cdir_buf;

            delete cdir_buf;

            LPTSTR bufOut = nullptr;
            bufOut = new TCHAR[MAX_PATH];
            BOOL suc = PathRelativePathTo(bufOut, cdir.c_str(), FILE_ATTRIBUTE_DIRECTORY, fullpath.c_str(), FILE_ATTRIBUTE_NORMAL);

            if (!suc) {
                throw std::runtime_error("GetFileDirectoryRel - PathRelativePathTo failed with error code " + std::to_string(GetLastError()));
            }

            std::string ret = bufOut;
            delete bufOut;

            return ret;
        }
        std::string GetFileName(std::string relpath) {
            std::string name, path;
            GetFullPathAndName(relpath, name, path);
            return name;
        }
        std::string GetFileExtension(std::string relpath) {
            std::string fullname = GetFileName(relpath);
            return fullname.substr(fullname.find_last_of("."));
        }
        std::string GetFullPath(std::string relpath) {
            std::string name, path;
            GetFullPathAndName(relpath, name, path);
            return path + name;
        }
        void Create(std::string reltarget) {
            if (FileExists(reltarget)) {
                throw std::runtime_error("File::Create - The file already exists");
            }

            HANDLE f = CreateFile(reltarget.c_str(), GENERIC_ALL, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0);
            if (!f) {
                throw std::runtime_error("FileSystem::Create - CreateFile failed with erorr " + std::to_string(GetLastError()));
            } else {
                CloseHandle(f);
            }
        }

        class FileBase {
        protected:
            virtual std::unique_ptr<char[]> _getRawData() = 0;
            virtual void _setRawData(std::unique_ptr<char[]>&&) = 0;
            virtual std::string _getFullPath() = 0;
            virtual void _setFullPath(std::string) = 0;

        public:
            template <class TransformTo>
            TransformTo cast() {
                TransformTo ret;
                FileBase* pret = &ret;
                pret->_setRawData(this->_getRawData());
                pret->_setFullPath(this->_getFullPath());

                return ret;
            }

            template <class TransformTo>
            explicit operator TransformTo() {
                return cast<TransformTo>();
            };
        };
        class Directory {
            std::string _fulldirpath;
            std::vector<std::string> _child_files;
            std::vector<std::string> _child_dirs;

        public:
            Directory() {}
            Directory(std::string path) {
                if (path.length() > 0) {
                    target(path);

                    if (exists()) {
                        load();
                    }
                }
            }

            virtual void target(std::string relfilepath) {
                char last = relfilepath[relfilepath.length() - 1];
                if (last == '\\' || last == '/') {
                    relfilepath.pop_back();
                }

                //Get the full file path
                _fulldirpath = GetFullPath(relfilepath);
            }
            virtual void load() {
                //Find all children
                HANDLE dir;
                WIN32_FIND_DATA file_data;
                if ((dir = FindFirstFile((getFullPath() + "/*").c_str(), &file_data)) != INVALID_HANDLE_VALUE) {
                    do {
                        const std::string file_name = file_data.cFileName;
                        const bool is_directory = (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

                        if (is_directory) {
                            if (file_name == "." || file_name == "..") {
                                continue;
                            }

                            _child_dirs.push_back(file_name);
                        } else {
                            _child_files.push_back(file_name);
                        }
                    } while (FindNextFile(dir, &file_data));

                    FindClose(dir);
                }


            }
            virtual void load(std::string path) {
                target(path);
                load();
            }
            virtual void create() {
                BOOL res = CreateDirectory(_fulldirpath.c_str(), NULL);

                if (!res) {
                    throw std::runtime_error("Directory::create - CreateDirectory failed with " + std::to_string(GetLastError()));
                }
            }
            virtual void create(std::string path) {
                target(path);
                create();
            }
            virtual void terminate() {
                BOOL res = RemoveDirectoryA(_fulldirpath.c_str());

                if (!res) {
                    throw std::runtime_error("Directory::terminate - Error deleting file. Error " + std::to_string(errno));
                }
            }
            virtual void name(std::string name) {
                int res = rename(_fulldirpath.c_str(), name.c_str());

                if (res != 0) {
                    throw std::runtime_error("Directory::name - Error deleting file. Error " + std::to_string(errno));
                }
            }
            virtual void move(std::string newpath) {
                if (newpath[newpath.length() - 1] != '/' || newpath[newpath.length() - 1] != '\\') {
                    newpath += '/';
                }

                newpath += getDirectoryName();

                if (FileExists(newpath)) {
                    throw std::runtime_error("Directory::move - the destination already exists");
                }

                BOOL res = MoveFile(_fulldirpath.c_str(), newpath.c_str());

                if (!res) {
                    throw std::runtime_error("Directory::move - MoveFile failed with error " + std::to_string(GetLastError()));
                }

                target(_fulldirpath);
            }
            virtual unsigned int countChildren() {
                return _child_files.size() + _child_dirs.size();
            }
            virtual unsigned int countChildDirs() {
                return _child_dirs.size();
            }
            virtual unsigned int countChildFiles() {
                return _child_files.size();
            }
            virtual BinaryFile getChildFile(unsigned int);
            virtual Directory getChildDir(unsigned int i) {
                return Directory(_child_dirs[i]);
            }

            virtual std::string getFullPath() {
                if (_fulldirpath.length() == 0) {
                    throw std::runtime_error("File::getFullPath - File path is null");
                }

                return _fulldirpath;
            }
            virtual std::string getDirectoryName() {
                if (_fulldirpath.length() == 0) {
                    throw std::runtime_error("File::getFileName - File path is null");
                }
                std::string name, path;
                GetFullPathAndName(_fulldirpath, name, path);
                return name;
            }
            virtual std::string getPathAbs() {
                if (_fulldirpath.length() == 0) {
                    throw std::runtime_error("File::getFileDirectoryAbs - File path is null");
                }
                std::string name, path;
                GetFullPathAndName(_fulldirpath, name, path);
                return path;
            }
            virtual std::string getPathRel() {
                if (_fulldirpath.length() == 0) {
                    throw std::runtime_error("File::getFileDirectoryRel - File path is null");
                }

                return GetFileDirectoryRel(getPathAbs());
            }
            virtual bool exists() {
                return DirExists(_fulldirpath);
            }
        };
        template <class FileFormat, class StorageType, class AccessType, class ...FormatArgs>
        class FileType: public FileBase {
            typedef FileType<FileFormat, StorageType, AccessType, FormatArgs...> This;

        protected:
            Format::FormatBase<StorageType, AccessType>* _file;
            std::string _fullfilepath;

            virtual std::unique_ptr<char[]> _getRawData() {
                return std::move(_file->rawDataOut());
            }
            virtual void _setRawData(std::unique_ptr<char[]>&& data) {
                _file->rawDataIn(std::move(data));
            }
            virtual std::string _getFullPath() {
                return _fullfilepath;
            };
            virtual void _setFullPath(std::string path) {
                _fullfilepath = path;
            };

        public:
            FileType(std::string relfilepath, FormatArgs ...args) {
                _file = new FileFormat(args...);

                if (relfilepath.length() > 0) {
                    target(relfilepath);
                    if (exists()) {
                        open();
                    }
                }
            }
            FileType(This const& file) {
                _fullfilepath = file._fullfilepath;
                _file = new FileFormat(*((FileFormat*)file._file));
            }
            FileType(This&& file) {
                _file = file._file;
                file._file = nullptr;
            }
            virtual ~FileType() {
                if (_file) {
                    delete _file;
                }
            }

            This operator+(StorageType& data) {
                This ret(*this);
                ret._file->dataIn(data);
                return ret;
            }
            This operator+(This& file) {
                This ret(*this);
                ret._fullfilepath = ""; //When there are two files, the programmer must choose where to save it
                ret._file->rawDataIn(file._file->rawDataOut());
                return ret;
            }
            This& operator+=(StorageType data) {
                _file->dataIn(data);
                return *this;
            }
            This& operator+=(This& file) {
                _file->rawDataIn(file._file->rawDataOut());
                return *this;
            }
            bool operator==(This& file) {
                //If they are different lengths, return false
                if (_file->size() != file._file->size()) {
                    return false;
                }

                auto one = _file->rawDataOut();
                auto two = file._file->rawDataOut();

                for (int i = 0; i < _file->size(); ++i) {
                    if (one[i] != two[i]) {
                        return false;
                    }
                }

                return true;
            }
            This& operator=(This& file) {
                _file->clearAll();
                _file->rawDataIn(file._file->rawDataOut());
                return *this;
            }

            virtual void target(std::string relfilepath) {
                //Get the full file path
                _fullfilepath = GetFullPath(relfilepath);
            }
            virtual void open() {
                if (_fullfilepath.length() == 0) {
                    throw std::runtime_error("File::load - File path is null");
                }

                std::ifstream in(_fullfilepath);
                if (!in) {
                    throw std::runtime_error("File::load - The file location does not point to a valid file.");
                }

                std::string buf;
                std::string line;
                bool first = true;
                while (std::getline(in, line, '\n')) {
                    if (first) {
                        first = false;
                        buf += line;
                    } else {
                        buf += (char) '\n' + line;
                    }
                }

                _file->rawDataIn(std::unique_ptr<char[]>(const_cast<char*>(buf.c_str())));
            }
            virtual void open(std::string reltarget) {
                target(reltarget);
                open();
            }
            virtual void create() {
                if (_fullfilepath.length() == 0) {
                    throw std::runtime_error("File::create - File path is null");
                }

                Create(_fullfilepath);
            }
            virtual void create(std::string reltarget) {
                target(reltarget);
                create();
            }
            virtual void createOrOpen() {
                if (!exists()) {
                    create();
                }

                open();
            }
            virtual void createOrOpen(std::string reltarget) {
                target(reltarget);
                createOrOpen();
            }
            virtual void save() {
                save(_fullfilepath);
            }
            virtual void clear() {
                _file->clearAll();
            }
            virtual void save(std::string name) {\
                if (name.length() == 0) {
                    throw std::runtime_error("File::save - No file was specified as the target");
                }

                std::ofstream out(name);
                out << _file->rawDataOut().get();
                out.close();
            }
            virtual void terminate() {
                int res = remove(_fullfilepath.c_str());

                if (res != 0) {
                    throw std::runtime_error("File::terminate - Error deleting file. Error " + std::to_string(errno));
                }
            }
            virtual void name(std::string name) {
                int res = rename(_fullfilepath.c_str(), name.c_str());

                if (res != 0) {
                    throw std::runtime_error("File::name - Error deleting file. Error " + std::to_string(errno));
                }
            }
            virtual void move(std::string newpath) {
                if (newpath[newpath.length() - 1] != '/' || newpath[newpath.length() - 1] != '\\') {
                    newpath += '/';
                }

                newpath += getFileName();

                if (FileExists(newpath)) {
                    throw std::runtime_error("File::move - the destination already exists");
                }

                BOOL res = MoveFile(_fullfilepath.c_str(), newpath.c_str());

                if (!res) {
                    throw std::runtime_error("File::move - MoveFile failed with error " + std::to_string(GetLastError()));
                }

                target(_fullfilepath);
            }
            virtual void copy(std::string newpath) {
                if (newpath[newpath.length() - 1] != '/' || newpath[newpath.length() - 1] != '\\') {
                    newpath += '/';
                }

                newpath += getFileName();

                if (FileExists(newpath)) {
                    throw std::runtime_error("File::copy - the destination already exists");
                }

                BOOL res = CopyFile(_fullfilepath.c_str(), newpath.c_str(), TRUE);

                if (!res) {
                    throw std::runtime_error("File::copy - CopyFile failed with error " + std::to_string(GetLastError()));
                }
            }
            virtual bool exists() {
                if (_fullfilepath.length() == 0) {
                    throw std::runtime_error("File::exists - File path is null");
                }

                return FileExists(_fullfilepath);
            }
            virtual unsigned int size() {
                return _file->size();
            }
            template <class Algorithm, class ...Args>
            void compress(Args... args) {
                std::unique_ptr<Transformation::CompressionBase> compressor = new Algorithm();

                //Concat lines
                std::string file = _file->rawDataOut().get();

                //Apply compression algorithm
                std::string compressed = compressor->compress(file, args...);

                //Erase all lines
                _file->clearAll();

                //Add the compressed string as one line
                _file->rawDataIn(std::unique_ptr<char[]>(const_cast<char*>(compressed.c_str())));
            }
            template <class Algorithm, class ...Args>
            void uncompress(Args... args) {
                std::unique_ptr<Transformation::CompressionBase> compressor = new Algorithm();

                //Ensure all lines are concatenated
                std::string compressed = _file->rawDataOut().get();

                //Erase all lines
                _file->clearAll();

                //Uncompress and process into the file
                _file->rawDataIn(std::unique_ptr<char[]>(const_cast<char*>(compressor->uncompress(compressed, args...).c_str())));
            }
            template <class Algorithm, class ...Args>
            void encrypt(Args... args) {
                std::unique_ptr<Transformation::EncryptionBase> encryptor = new Algorithm();

                //Concat lines
                std::string file = _file->rawDataOut().get();

                //Apply compression algorithm
                std::string encrypted = encryptor->encrypt(file, args...);

                //Erase all lines
                _file->clearAll();

                //Add the compressed string as one line
                _file->rawDataIn(std::unique_ptr<char[]>(const_cast<char*>(encrypted.c_str())));
            }
            template <class Algorithm, class ...Args>
            void decrypt(Args... args) {
                std::unique_ptr<Transformation::EncryptionBase> encryptor = new Algorithm();

                //Get encrypted data
                std::string encrypted = _file->rawDataOut().get();

                //Erase all data
                _file->clearAll();

                //Decrypt and process into the file
                _file->rawDataIn(std::unique_ptr<char[]>(const_cast<char*>(encryptor->decrypt(encrypted, args...).c_str())));
            }
            template <class Algorithm, class ...Args>
            void transform(Args... args) {
                std::unique_ptr<Transformation::TransformationBase> transformer = new Algorithm();

                //Get all data at once
                std::string file = _file->rawDataOut().get();

                //Apply transformation algorithm
                std::string transformed = transformer->transform(file, args...);

                //Erase all data
                _file->clearAll();

                //Add the transformed string back to the file
                _file->rawDataIn(std::unique_ptr<char[]>(const_cast<char*>(transformed.c_str())));
            }

            virtual std::string getFullPath() {
                if (_fullfilepath.length() == 0) {
                    throw std::runtime_error("File::getFullPath - File path is null");
                }

                return _fullfilepath;
            }
            virtual std::string getFileName() {
                if (_fullfilepath.length() == 0) {
                    throw std::runtime_error("File::getFileName - File path is null");
                }
                std::string name, path;
                GetFullPathAndName(_fullfilepath, name, path);
                return name;
            }
            virtual std::string getFileExtension() {
                std::string name = getFileName();
                return name.substr(name.find_last_of('.') + 1);
            }
            virtual std::string getFileDirectoryAbs() {
                if (_fullfilepath.length() == 0) {
                    throw std::runtime_error("File::getFileDirectoryAbs - File path is null");
                }
                std::string name, path;
                GetFullPathAndName(_fullfilepath, name, path);
                return path;
            }
            virtual std::string getFileDirectoryRel() {
                if (_fullfilepath.length() == 0) {
                    throw std::runtime_error("File::getFileDirectoryRel - File path is null");
                }

                return GetFileDirectoryRel(getFileDirectoryAbs());
            }

            virtual Directory getParentDirectory() {
                return Directory(getFileDirectoryAbs());
            }
        };
        class BinaryFile: public FileType<Format::Binary, char, unsigned int> {
        public:
            BinaryFile(): FileType("") {}
            BinaryFile(std::string path): FileType(path) {}
            BinaryFile(FileType<Format::Binary, char, unsigned int>& file): FileType(file) {}
            BinaryFile(FileType<Format::Binary, char, unsigned int>&& file): FileType(file) {}
            BinaryFile(BinaryFile const& file): FileType(file) {}
            BinaryFile(BinaryFile&& file): FileType(file) {}

            virtual char& operator[](unsigned int i) {
                if (i > _file->segmentCount() - 1) {
                    throw std::runtime_error("BinaryFile[int] - The provided index doesn't exist");
                }

                return _file->dataRef(i);
            }
            virtual char get(unsigned int i) const {
                if (i > _file->segmentCount() - 1) {
                    throw std::runtime_error("BinaryFile[int] - The provided index doesn't exist");
                }

                return _file->dataOut(i);
            }

            virtual void push_byte(char byte) {
                _file->dataIn(byte);
            }
            virtual char pop_byte() {
                char ret = _file->dataOut(_file->segmentCount() - 1);
                _file->clear(_file->segmentCount() - 1);
                return ret;
            }
        };
        class File: public BinaryFile {
        public:
            File(): BinaryFile("") {}
            File(std::string path): BinaryFile(path) {}
            File(FileType<Format::Binary, char, unsigned int>& file): BinaryFile(file) {}
            File(FileType<Format::Binary, char, unsigned int>&& file): BinaryFile(file) {}
            File(File const& file): BinaryFile(file) {}
            File(File&& file): BinaryFile(file) {}
        };
        class TextFile: public FileType<Format::Text, std::string, unsigned int, char> {
        public:
            TextFile(): FileType("", 13) {}
            TextFile(std::string path, char delim = 13): FileType(path, delim) {}
            TextFile(FileType<Format::Text, std::string, unsigned int, char>& file): FileType(file) {}
            TextFile(FileType<Format::Text, std::string, unsigned int, char>&& file): FileType(file) {}
            TextFile(TextFile const& file): FileType(file) {}
            TextFile(TextFile&& file): FileType(file) {}

            virtual std::string& operator[](unsigned int index) {
                if (index > lines() - 1) {
                    throw std::runtime_error("TextFile[int] - The provided index doesn't exist");
                }

                return _file->dataRef(index);
            }
            virtual  std::string get(unsigned int index) const {
                if (index > lines() - 1) {
                    throw std::runtime_error("TextFile[int] - The provided index doesn't exist");
                }

                return _file->dataOut(index);
            }
            virtual unsigned int lines() const {
                return _file->segmentCount();
            }
            virtual void push_line(std::string line) {
                _file->dataIn(line);
            }
            virtual std::string pop_line() {
                if (_file->segmentCount() == 0) {
                    throw std::runtime_error("TextFile::pop - No more data to pop");
                }

                std::string ret = _file->dataOut(_file->segmentCount() - 1);
                _file->clear(_file->segmentCount() - 1);
                return ret;
            }
        };
        class DLLFile: public BinaryFile {
            HMODULE hDLL = nullptr;

        public:
            DLLFile(): BinaryFile("") {}
            DLLFile(std::string path): BinaryFile(path) {}
            DLLFile(FileType<Format::Binary, char, unsigned int>& file): BinaryFile(file) {}
            DLLFile(FileType<Format::Binary, char, unsigned int>&& file): BinaryFile(file) {}
            DLLFile(DLLFile const& file): BinaryFile(file) {}
            DLLFile(DLLFile&& file): BinaryFile(file) {}

            template <class Ret, class ...Args>
            std::function<Ret(Args...)> getProc(std::string name) {
                if (!hDLL) {
                    throw std::runtime_error("DLLFile::getProc - The DLL file must be loaded before trying to access procedures within it.");
                }

#pragma clang diagnostic push
#pragma ide diagnostic ignored "CannotResolve"
                typedef Ret(*fn_type)(Args...);
#pragma clang diagnostic pop

                FARPROC ret = GetProcAddress(hDLL, name.c_str());

                if (!ret) {
                    throw std::runtime_error("DLLFile::getProc - GetProcAddress failed with error " + std::to_string(GetLastError()));
                }

                return (fn_type) ret;
            };

            void loadLibrary() {
                hDLL = LoadLibrary((getFileDirectoryAbs() + getFileName()).c_str());

                if (!hDLL) {
                    throw std::runtime_error("DLLFile::loadLibrary - Windows API function failed with error code " + std::to_string(GetLastError()));
                }
            }
            void freeLibrary() {
                if (hDLL) {
                    FreeLibrary(hDLL);
                    hDLL = nullptr;
                }
            }
            HMODULE getHandle() {
                if (hDLL) {
                    return hDLL;
                }

                throw std::runtime_error("DLLFile::getHandle - Handle has not been created.");
            }
            bool HandleIsLoaded() {
                return (bool) hDLL;
            }
        };
        class ConfigFile: public TextFile {
            std::map<std::string, std::string> _map;
            std::string _commentDelim = "#";
            std::string _assignDelim = "=";

        public:
            ConfigFile(): TextFile("", 13) {}
            ConfigFile(std::string path, char delim = 13): TextFile(path, delim) {}
            ConfigFile(FileType<Format::Text, std::string, unsigned int, char>& file): TextFile(file) {}
            ConfigFile(FileType<Format::Text, std::string, unsigned int, char>&& file): TextFile(file) {}
            ConfigFile(ConfigFile const& file): TextFile(file) {}
            ConfigFile(ConfigFile&& file): TextFile(file) {}

            virtual void open() override {
                TextFile::open();

                //Add some extra functionality to parse the config file
                for (unsigned int i = 0; i < this->lines(); ++i) {
                    std::string line = TextFile::get(i);

                    if (line.find(_commentDelim) == 0) {
                        continue; //A comment line
                    } else {
                        _map[line.substr(0, line.find(_assignDelim) + 1)] = line.substr(line.find(_assignDelim) + 1);
                    }
                }
            }
            virtual void save() override {
                //Add extra functionality to write the config back to the file
                for (unsigned int i = 0; i < this->lines(); ++i) {
                    std::string line = TextFile::get(i);

                    if (line.find(_commentDelim) == 0) {
                        continue; //A comment line
                    } else {
                        std::string key = line.substr(0, line.find(_assignDelim) + 1);
                        std::string val = line.substr(line.find(_assignDelim) + 1);

                        if (_map.find(key) != _map.end()) {
                            TextFile::operator[](i) = key + _assignDelim + val;
                            _map.erase(key);
                        }
                    }
                }
                for (auto j = _map.begin(); j != _map.end(); ++j) {
                    TextFile::push_line(j->first + _assignDelim + j->second);
                }

                TextFile::save();
            }
            virtual std::string& operator[](std::string key) {
                if (key.length() == 0) {
                    throw std::runtime_error("ConfigFile[] - The provided key was empty");
                }

                return _map[key];
            }
            virtual std::string const& get(std::string key) {
                if (key.length() == 0) {
                    throw std::runtime_error("ConfigFile::get() - The provided key was empty");
                }

                return _map[key];
            }

            const std::string &getCommentDelimeter() const {
                return _commentDelim;
            }

            void setCommentDelimeter(const std::string &_commentDelim) {
                ConfigFile::_commentDelim = _commentDelim;
            }

            const std::string &getAssignmentDelimeter() const {
                return _assignDelim;
            }

            void setAssignmentDelimeter(const std::string &_assignDelim) {
                ConfigFile::_assignDelim = _assignDelim;
            }
        };
        class LogFile: public TextFile {
            int _mday;
            std::string _filename;

        public:
            LogFile(): TextFile("", 13) {}
            LogFile(std::string path, char delim = 13): TextFile("", delim) {
                target(path);
                if (exists()) {
                    open();
                }
            }
            LogFile(FileType<Format::Text, std::string, unsigned int, char>& file): TextFile(file) {}
            LogFile(FileType<Format::Text, std::string, unsigned int, char>&& file): TextFile(file) {}
            LogFile(LogFile const& file): TextFile(file) {}
            LogFile(LogFile&& file): TextFile(file) {}

            virtual void target(std::string name) override {
                std::time_t today = time(0);
                tm* now = localtime(&today);
                _mday = now->tm_mday;

                std::string timestr;
                timestr += std::to_string(now->tm_year + 1900) + "_";
                timestr += std::to_string(now->tm_mon + 1) + "_";
                timestr += std::to_string(now->tm_mday) + "_";

                _filename = name;

                TextFile::target(timestr + name);
            }
            virtual void log(std::string message) {
                if (_filename.length() == 0) {
                    throw std::runtime_error("LogFile::log - cannot log to file as no file is specified");
                }

                std::time_t now = time(0);
                auto p = localtime(&now);

                if (p->tm_mday != _mday) { //Different day to the file currently being used. Add new file.
                    target(_filename); //Will add today's datestamp to the file
                    this->clear();
                    this->createOrOpen();
                    _mday = p->tm_mday;
                }

                if (!exists()) {
                    createOrOpen();
                }

                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
                std::time_t t = time(0);
                tm* ptr_t = localtime(&t);

                std::string timestr = std::to_string(ptr_t->tm_hour) + ":" + std::to_string(ptr_t->tm_min) + ":" + std::to_string(ptr_t->tm_sec) + "." + std::to_string((ms % 1000).count());

                push_line("[" + timestr + "] " + message);
            }
        };
        //CSV File
        //JSON File
        //XML File


        BinaryFile Directory::getChildFile(unsigned int i) {
            return BinaryFile(_child_files[i]);
        }
    }
}
#endif //PROJECT_FILING_H
