#ifndef I2C_SLAVE_H
#define I2C_SLAVE_H


#include <systemc.h>

typedef enum { S_IDLE, S_ADDR, S_DATA } I2C_Slave_State;

SC_MODULE(I2C_Slave) {
    //------ PORTS definitions
    sc_inout<bool>      SCL;
    sc_inout<bool>      SDA;


    //------ Member/state variables
    sc_bv<7>            m_my_addr;      // This Slave address
    I2C_Slave_State     m_state;
    sc_fifo<byte>       rx_fifo;


    // Constructor
    I2C_Slave(sc_module_name nm, const byte& addr) : sc_module(nm),
            m_my_addr(addr),
            rx_fifo("rx_fifo", 100) {

        m_state = S_IDLE;

        SC_THREAD(main_process);

        SC_THREAD(watch_for_start);
        sensitive << m_idle_ev;
    }

    SC_HAS_PROCESS(I2C_Slave);

    // This thread process waits for event generated from main_process() to start checking for start bit
    void watch_for_start() {
        while (true) { 
            wait(m_idle_ev);
            while (true) {
                wait(SDA.negedge_event());
                if (SCL == 1) {
                    // Start bit received. So break waiting
                    break;
                }
            }
        }
    }


    // Called from main_process() thread to deserialize and get a byte of data
    void deserialize(byte& data) {
        sc_bv<8> data_bv;
        for (int i = 0; i < 8; ++i) {
            wait(SCL.posedge_event());
            data_bv[i] = SDA;
        }
        data = data_bv;
    }
        

    void main_process() {
        // Initially slave is in S_IDLE state
        m_state = S_IDLE;
        while (true) {
            switch (m_state) {
                case S_IDLE:
                    // Initially slave is in S_IDLE state
                    m_idle_ev.notify();
                    break;

                case S_ADDR:
                    // When it receives start bit, it goes to get address state
                    deserialize(addr);

                    // If address matches its address, it goes to data state
                    if (addr == m_my_addr) {
                        m_state = S_DATA;
                    } else
                        m_state = S_IDLE;
                    break;

                case S_DATA:
                    // In this phase, it is getting data and sending ACK
                    deserialize(data);
                    send_ack();
                    rx_fifo.write(data);
                    break;
            }
        }
    }
};

#endif
