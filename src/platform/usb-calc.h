// Pure USB descriptor computations.
//
// No SFR access and no I/O, so test/unit/test_usb_calc.c can compile this
// header with a host compiler (make unit-test).  Keep it that way.

#ifndef USB_CALC_H
#define USB_CALC_H

// Offset of string descriptor `index` in `descriptors`, the `length`-byte
// concatenation of string descriptors registered with
// usb_string_descriptor(), each starting with its own length byte.
// Returns -1 if the table has no descriptor `index`.
static inline int
usb_string_descriptor_offset(const unsigned char *descriptors, int length, int index)
{
    int i;
    int j;

    i = index;
    j = 0;
    while (i && j < length) {
        j += descriptors[j];
        i--;
    }
    if (i || j >= length) {
        return -1;
    }
    return j;
}

#endif
