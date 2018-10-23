#include <systemc.h>
#include "i2c_master.h"

int sc_main(int argc, char* argv[])
{
    I2C_Master master("master");
    I2C_Slave  slave("slave");

    sc_signal<bool> SDA_sig, SCL_sig;

    sc_clock clk("clk", 2.5, SC_NS);

    // Master port bindings
    master.clk(clk);
    master.SCL(SCL_sig);
    master.SDA(SDA_sig);

    // Slave port bindings
    slave.SCL(SCL_sig);
    slave.SDA(SDA_sig);

    sc_start(500, SC_NS);

    return 0;
}
