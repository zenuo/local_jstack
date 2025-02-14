#include "jvm_def.h"

outputStream::outputStream(int width)
{
    _width = width;
    _position = 0;
    _newlines = 0;
    _precount = 0;
    _indentation = 0;
    _scratch = NULL;
    _scratch_len = 0;
}

void outputStream::update_position(const char *s, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        char ch = s[i];
        if (ch == '\n')
        {
            _newlines += 1;
            _precount += _position + 1;
            _position = 0;
        }
        else if (ch == '\t')
        {
            int tw = 8 - (_position & 7);
            _position += tw;
            _precount -= tw - 1; // invariant:  _precount + _position == total count
        }
        else
        {
            _position += 1;
        }
    }
}

bufferedStream::bufferedStream(size_t initial_size, size_t bufmax) : outputStream()
{
    buffer_length = initial_size;
    buffer = (char *)malloc(buffer_length * sizeof(char));
    buffer_pos = 0;
    buffer_fixed = false;
    buffer_max = bufmax;
    truncated = false;
}

bufferedStream::bufferedStream(char *fixed_buffer, size_t fixed_buffer_size, size_t bufmax) : outputStream()
{
    buffer_length = fixed_buffer_size;
    buffer = fixed_buffer;
    buffer_pos = 0;
    buffer_fixed = true;
    buffer_max = bufmax;
    truncated = false;
}

void bufferedStream::write(const char *s, size_t len)
{

    if (truncated)
    {
        return;
    }

    if (buffer_pos + len > buffer_max)
    {
        flush(); // Note: may be a noop.
    }

    size_t end = buffer_pos + len;
    if (end >= buffer_length)
    {
        if (buffer_fixed)
        {
            // if buffer cannot resize, silently truncate
            len = buffer_length - buffer_pos - 1;
            truncated = true;
        }
        else
        {
            // For small overruns, double the buffer.  For larger ones,
            // increase to the requested size.
            if (end < buffer_length * 2)
            {
                end = buffer_length * 2;
            }
            // Impose a cap beyond which the buffer cannot grow - a size which
            // in all probability indicates a real error, e.g. faulty printing
            // code looping, while not affecting cases of just-very-large-but-its-normal
            // output.
            const size_t reasonable_cap = MAX2(100 * M, buffer_max * 2);
            if (end > reasonable_cap)
            {
                // In debug VM, assert right away.
                assert(false, "Exceeded max buffer size for this string.");
                // Release VM: silently truncate. We do this since these kind of errors
                // are both difficult to predict with testing (depending on logging content)
                // and usually not serious enough to kill a production VM for it.
                end = reasonable_cap;
                size_t remaining = end - buffer_pos;
                if (len >= remaining)
                {
                    len = remaining - 1;
                    truncated = true;
                }
            }
            if (buffer_length < end)
            {
                // buffer = REALLOC_C_HEAP_ARRAY(char, buffer, end, mtInternal);
                buffer = (char *)realloc(buffer, end * sizeof(char));
                buffer_length = end;
            }
        }
    }
    if (len > 0)
    {
        memcpy(buffer + buffer_pos, s, len);
        buffer_pos += len;
        update_position(s, len);
    }
}

char *bufferedStream::as_string()
{
    // char* copy = NEW_RESOURCE_ARRAY(char, buffer_pos+1);
    char *copy = (char *)malloc((buffer_pos + 1) * sizeof(char));
    strncpy(copy, buffer, buffer_pos);
    copy[buffer_pos] = 0; // terminating null
    return copy;
}

bufferedStream::~bufferedStream()
{
    if (!buffer_fixed)
    {
        free(buffer);
    }
}