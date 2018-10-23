#include <systemc.h>
#include "i2c_master_core.h"

typedef unsigned char byte;

SC_MODULE(I2C_Master)
{
    //----- Ports
    sc_in<bool>         clk;
    sc_inout<bool>      SCL;
    sc_inout<bool>      SDA;

    //----- Buffer generated for storing bytes for transmission
    sc_fifo<byte> tx_fifo;

    //----- Instance of I2C Master core block
    I2C_Master_Core i2cm;

    // Constructor
    SC_CTOR(I2C_Master) : i2cm("i2c_master") {
        i2cm.in_port(tx_fifo);
        SC_THREAD(fill_tx_fifo);
    }

    // Test thread to fill the tx_fifo buffer to start I2C transfer
    void fill_tx_fifo()
    {
        ifstream inf("test_file");
        byte c;
        while (!inf.eof()) {
            inf >> c;
            if (inf.eof())
                break;
            tx_fifo.write(c);
            cout << "Wrote to TX fifo: " << c << endl;
        }
        inf.close();
    }
};

