#include <Arduino.h>

Print *stdPrint = &SERDBG;
extern "C" {
    int _write(int file, const void *buf, size_t len) {
        Print *out;

        // Send both stdout and stderr to stdPrint
        if (file == stdout->_file || file == stderr->_file) {
            out = stdPrint;
        } else {
            out = (Print *)file;
        }

        if (out == nullptr) {
            return len;
        }

        // Don't check for len == 0 for returning early, in case there's side effects
        return out->write((const uint8_t *)buf, len);
    }
}
