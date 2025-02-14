#include "jni.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef THREAD_DUMP_JVM_DEF_H
#define THREAD_DUMP_JVM_DEF_H

#define vmassert(p, ...)
#define assert(p, ...) vmassert(p, __VA_ARGS__)

template <class T>
inline T MAX2(T a, T b) { return (a > b) ? a : b; }
template <class T>
inline T MIN2(T a, T b) { return (a < b) ? a : b; }

const size_t K = 1024;
const size_t M = K * K;

#define NONCOPYABLE(C) \
  C(C const &);        \
  C &operator=(C const &)

#define ATTRIBUTE_PRINTF(fmt, vargs)

typedef uint64_t julong;

class AttachOperation
{
public:
  enum
  {
    name_length_max = 16,  // maximum length of  name
    arg_length_max = 1024, // maximum length of argument
    arg_count_max = 3      // maximum number of arguments
  };

  // name of special operation that can be enqueued when all
  // clients detach
  static char *detachall_operation_name() { return (char *)"detachall"; }

private:
  char _name[name_length_max + 1];
  char _arg[arg_count_max][arg_length_max + 1];

public:
  const char *name() const { return _name; }

  // set the operation name
  void set_name(char *name)
  {
    assert(strlen(name) <= name_length_max, "exceeds maximum name length");
    size_t len = MIN2(strlen(name), (size_t)name_length_max);
    memcpy(_name, name, len);
    _name[len] = '\0';
  }

  // get an argument value
  const char *arg(int i) const
  {
    assert(i >= 0 && i < arg_count_max, "invalid argument index");
    return _arg[i];
  }

  // set an argument value
  void set_arg(int i, char *arg)
  {
    assert(i >= 0 && i < arg_count_max, "invalid argument index");
    if (arg == NULL)
    {
      _arg[i][0] = '\0';
    }
    else
    {
      assert(strlen(arg) <= arg_length_max, "exceeds maximum argument length");
      size_t len = MIN2(strlen(arg), (size_t)arg_length_max);
      memcpy(_arg[i], arg, len);
      _arg[i][len] = '\0';
    }
  }

  // create an operation of a given name
  AttachOperation(char *name)
  {
    set_name(name);
    for (int i = 0; i < arg_count_max; i++)
    {
      set_arg(i, NULL);
    }
  }

  // complete operation by sending result code and any result data to the client
  //  virtual void complete(jint result, bufferedStream* result_stream) = 0;
};

// TimeStamp is used for recording when an event took place.
class TimeStamp
{
private:
  jlong _counter;

public:
  TimeStamp() { _counter = 0; }
  void clear() { _counter = 0; }
  // has the timestamp been updated since being created or cleared?
  bool is_updated() const { return _counter != 0; }
  // update to current elapsed time
  void update();
  // update to given elapsed time
  void update_to(jlong ticks);
  // returns seconds since updated
  // (must not be in a cleared state:  must have been previously updated)
  double seconds() const;
  jlong milliseconds() const;
  // ticks elapsed between VM start and last update
  jlong ticks() const { return _counter; }
  // ticks elapsed since last update
  jlong ticks_since_update() const;
};

class outputStream
{
private:
  NONCOPYABLE(outputStream);

protected:
  int _indentation;    // current indentation
  int _width;          // width of the page
  int _position;       // position on the current line
  int _newlines;       // number of '\n' output so far
  julong _precount;    // number of chars output, less _position
  TimeStamp _stamp;    // for time stamps
  char *_scratch;      // internal scratch buffer for printf
  size_t _scratch_len; // size of internal scratch buffer

  void update_position(const char *s, size_t len);
  static const char *do_vsnprintf(char *buffer, size_t buflen,
                                  const char *format, va_list ap,
                                  bool add_cr,
                                  size_t &result_len) ATTRIBUTE_PRINTF(3, 0);

  // calls do_vsnprintf and writes output to stream; uses an on-stack buffer.
  void do_vsnprintf_and_write_with_automatic_buffer(const char *format, va_list ap, bool add_cr) ATTRIBUTE_PRINTF(2, 0);
  // calls do_vsnprintf and writes output to stream; uses the user-provided buffer;
  void do_vsnprintf_and_write_with_scratch_buffer(const char *format, va_list ap, bool add_cr) ATTRIBUTE_PRINTF(2, 0);
  // calls do_vsnprintf, then writes output to stream.
  void do_vsnprintf_and_write(const char *format, va_list ap, bool add_cr) ATTRIBUTE_PRINTF(2, 0);

public:
  // creation
  outputStream(int width = 80);
  outputStream(int width, bool has_time_stamps);

  // indentation
  outputStream &indent();
  void inc() { _indentation++; };
  void dec() { _indentation--; };
  void inc(int n) { _indentation += n; };
  void dec(int n) { _indentation -= n; };
  int indentation() const { return _indentation; }
  void set_indentation(int i) { _indentation = i; }
  void fill_to(int col);
  void move_to(int col, int slop = 6, int min_space = 2);

  // sizing
  int width() const { return _width; }
  int position() const { return _position; }
  julong count() const { return _precount + _position; }
  void set_count(julong count) { _precount = count - _position; }
  void set_position(int pos) { _position = pos; }

  // printing
  void print(const char *format, ...) ATTRIBUTE_PRINTF(2, 3);
  void print_cr(const char *format, ...) ATTRIBUTE_PRINTF(2, 3);
  void vprint(const char *format, va_list argptr) ATTRIBUTE_PRINTF(2, 0);
  void vprint_cr(const char *format, va_list argptr) ATTRIBUTE_PRINTF(2, 0);
  void print_raw(const char *str) { write(str, strlen(str)); }
  void print_raw(const char *str, int len) { write(str, len); }
  void print_raw_cr(const char *str)
  {
    write(str, strlen(str));
    cr();
  }
  void print_raw_cr(const char *str, int len)
  {
    write(str, len);
    cr();
  }
  void print_data(void *data, size_t len, bool with_ascii);
  void put(char ch);
  void sp(int count = 1);
  void cr();
  void cr_indent();
  void bol()
  {
    if (_position > 0)
      cr();
  }

  // Time stamp
  TimeStamp &time_stamp() { return _stamp; }
  void stamp();
  void stamp(bool guard, const char *prefix, const char *suffix);
  void stamp(bool guard)
  {
    stamp(guard, "", ": ");
  }
  // Date stamp
  void date_stamp(bool guard, const char *prefix, const char *suffix);
  // A simplified call that includes a suffix of ": "
  void date_stamp(bool guard)
  {
    date_stamp(guard, "", ": ");
  }

  // portable printing of 64 bit integers
  void print_jlong(jlong value);
  void print_julong(julong value);

  // flushing
  virtual void flush() {}
  virtual void write(const char *str, size_t len) = 0;
  virtual void rotate_log(bool force, outputStream *out = NULL) {} // GC log rotation
  virtual ~outputStream() {}                                       // close properly on deletion

  // Caller may specify their own scratch buffer to use for printing; otherwise,
  // an automatic buffer on the stack (with O_BUFLEN len) is used.
  void set_scratch_buffer(char *p, size_t len)
  {
    _scratch = p;
    _scratch_len = len;
  }

  void dec_cr()
  {
    dec();
    cr();
  }
  void inc_cr()
  {
    inc();
    cr();
  }
};

class bufferedStream : public outputStream
{
protected:
  char *buffer;
  size_t buffer_pos;
  size_t buffer_max;
  size_t buffer_length;
  bool buffer_fixed;
  bool truncated;

public:
  bufferedStream(size_t initial_bufsize = 256, size_t bufmax = 1024 * 1024 * 10);
  bufferedStream(char *fixed_buffer, size_t fixed_buffer_size, size_t bufmax = 1024 * 1024 * 10);
  ~bufferedStream();
  virtual void write(const char *c, size_t len);
  size_t size() { return buffer_pos; }
  const char *base() { return buffer; }
  void reset()
  {
    buffer_pos = 0;
    _precount = 0;
    _position = 0;
  }
  char *as_string();
};

#endif