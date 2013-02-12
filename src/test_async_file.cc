#include "async_file.h"
#include "defs.h"
#include <iostream>

int main(int argc, char** argv)
{
    TASCAR::ringbuffer_t rb(32,1);
    DEBUG(rb.read_skip(5));
    DEBUG(rb.write_zeros(5));
    DEBUG(rb.read_skip(5));
    DEBUG(rb.read_skip(5));
    DEBUG(rb.write_zeros(30));
    DEBUG(rb.write_zeros(50));
    DEBUG(rb.write_zeros(50));
    DEBUG(rb.read_space());
    DEBUG(rb.write_space());
    DEBUG(rb.get_current_location());
    DEBUG(rb.read_skip(12));
    DEBUG(rb.get_current_location());
    DEBUG(rb.read_space());
    DEBUG(rb.write_space());
    rb.set_locate( 170 );
    DEBUG(rb.relocation_requested());
    DEBUG(rb.get_requested_location());
    rb.lock_relocate();
    DEBUG(&rb);
    DEBUG(rb.write_zeros(2));
    DEBUG(rb.read_skip(3));
    rb.unlock_relocate();
    DEBUG(rb.write_zeros(2));
    DEBUG(rb.read_skip(3));
    DEBUG(rb.get_current_location());
    return 0;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
