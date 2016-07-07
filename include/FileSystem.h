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
#include "Shlwapi.h"
#include "Sul.h"

namespace Sul {
    namespace FileSystem {
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
                virtual std::unique_ptr<char[]>&& rawDataOut() = 0;
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

                Text(char delim = 13) {
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
                virtual std::unique_ptr<char[]>&& rawDataOut() {
                    std::string buf;
                    for (int i = 0; i < _lines.size(); ++i) {
                        buf += _lines[i];
                    }

                    char* ret = new char[buf.length() + 1];
                    for (int j = 0; j < buf.length(); ++j) {
                        ret[j] = (char) buf[j];
                    }
                    ret[buf.length()] = 0;

                    return std::move(std::unique_ptr<char[]>(ret));
                }
                virtual void rawDataIn(std::unique_ptr<char[]>&& data) {
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
                    }

                    return count;
                }
                virtual unsigned int segmentCount() {
                    return _lines.size();
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
        bool Exists(std::string path) {
            std::ifstream in(path);
            bool ret = (bool) in;
            in.close();

            return ret;
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
            if (Exists(reltarget)) {
                throw std::runtime_error("File::Create - The file already exists");
            }

            std::ofstream out(reltarget);
            out << "";
            out.close();
        }

        template <class FileFormat, class StorageType, class AccessType, class ...FormatArgs>
        class File {
        protected:
            Format::FormatBase<StorageType, AccessType>* _file;
            std::string _fullfilepath;

        public:
            File(FormatArgs... args) {
                _file = new FileFormat(args...);
            }
            File(std::string relfilepath, FormatArgs ...args): File(args...) {
                target(relfilepath);
                if (exists()) {
                    open();
                }
            }
            virtual ~File() {
                delete _file;
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
                while (std::getline(in, line)) {
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
            virtual void save(std::string name) {
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

                if (Exists(newpath)) {
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

                if (Exists(newpath)) {
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

                return Exists(_fullfilepath);
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

                return GetFileDirectoryRel(_fullfilepath);
            }
        };
        class TextFile: public File<Format::Text, std::string, unsigned int, char> {
        public:
            TextFile(std::string path, char delim = 13): File(path, delim) {}

            std::string& operator[](unsigned int index) {
                return _file->dataRef(index);
            }
            std::string get(unsigned int index) const {
                return _file->dataOut(index);
            }
            unsigned int lines() {
                return _file->segmentCount();
            }
        };
    }
}
#endif //PROJECT_FILING_H
