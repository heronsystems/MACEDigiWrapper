#ifndef MATH_HELPER_H
#define MATH_HELPER_H

class MathHelper {

public:

    static int calc_checksum(const char *buf, const size_t start, const size_t end) {
        size_t sum = 0;
        for (size_t i = start; i < end; i++) {
            sum += buf[i];
        }
        return (0xff - sum) & 0xff;
    }

};

#endif // MATH_HELPER_H
