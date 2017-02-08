#ifndef STREAM_HPP
#define STREAM_HPP

#include <cstdio>

class Stream //: public FileLike
{

public:

    Stream(const char *name = NULL);
    virtual ~Stream();

    int putc(int c) {
        fflush(_file);
        return std::fputc(c, _file);
    }
    int puts(const char *s) {
        fflush(_file);
        return std::fputs(s, _file);
    }
    int getc() {
        fflush(_file);
        return std::fgetc(_file);
    }
    char *gets(char *s, int size) {
        fflush(_file);
        return std::fgets(s,size,_file);;
    }
    int printf(const char* format, ...);
    int scanf(const char* format, ...);

    operator std::FILE*() { return _file; }

    virtual const struct rpc_method *get_rpc_methods();

protected:

    virtual int close();
    virtual ssize_t write(const void* buffer, size_t length);
    virtual ssize_t read(void* buffer, size_t length);
    virtual off_t lseek(off_t offset, int whence);
    virtual int isatty();
    virtual int fsync();
    virtual off_t flen();

    virtual int _putc(int c) = 0;
    virtual int _getc() = 0;

    std::FILE *_file;

};

#endif // STREAM_HPP
