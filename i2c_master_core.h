#include <systemc.h>

typedef unsigned char byte;
typedef enum { M_IDLE, M_ADDR, M_DATA } I2C_Master_State;

SC_MODULE(I2C_Master_Core)
{
    //------ PORTS
    sc_in<bool>         ref_clk;
    sc_fifo_in<byte>    in_port;
    sc_out<bool>        SCL;
    sc_out<bool>        SDA;


    //------ Internal state variables
    I2C_Master_State    m_state;
    int                 m_slave_addr;
    bool                bus_claimed;
    void set_cur_slave_addr(int addr) { m_slave_addr = addr; }


    // Returns true if BUS is idle/free (SDA and SCL high for 10 ref_clk cycles)
    bool is_bus_free() {
        //  Check for 10 clock cycles whether the SDA and SCL are high. If so, bus is free
        wait(ref_clk.posedge_event());

        // Check if SDA and SCL are high for 10 clock cycles to infer that I2C bus is idle
        if (SDA == 1 && SCL == 1) {
            sc_time t1 = sc_time_stamp());
            wait(ref_clk.posedge_event());
            sc_time ref_clk_period = sc_time_stamp() - t1; // 1 clock cycle of ref_clk used to calculate
                                                           // the ref_clk_period
            // bus is free condition at this instant
            wait(ref_clk_period * 9, SDA | SCL); // wait for change in SCL or SDA 9 more clock cycles
            if (sc_time_stamp() - t1 == ref_clk_period * 10) {  // if it came out of wait due to timeout
                // of 10 ref_clk_period, then BUS is free/idle
                // bus found free
                m_bus_claimed = true;
                return true;
            }
        }
        return false;
    }


    // Assumption is that SCL has been stopped before calling this
    //   - and so this function uses ref_clk for delay 
    void release_bus() {
        wait(ref_clk.posedge_event());
        // Making SCL and SDA as 1 makes the BUS free
        SCL = 1;
        SDA = 1;
        m_bus_claimed = false;
        wait(ref_clk.posedge_event());
    }


    //--- tx_data() Process : if data is available in tx_fifo, then start the Master side transfer
    void tx_data() {
        int cnt = 0;
        while (true) {
            byte c;
            // If there is data to be sent, then start the master process
            if (in_port.nb_read(c) == false) {
                wait(ref_clk.posedge_event());
                cnt++;
                if (cnt == 3) {
                    if (m_bus_claimed) {
                        // Release the bus by sending stop bit
                        send_stop_bit();
                        // Release the bus
                        assert(m_bus_claimed == false);
                        in_port.read(c);   // Do a blocking read from FIFO
                    }
                    cnt = 0;
                } else
                    continue;  // Continue to do non-blocking read of FIFO for 3 times
            }
            cout << "@ " << sc_time_stamp() << name() << ": data to transmit = " << c << endl;


            do {

            //--- Master process ----
            // If bus is not claimed, check if bus is idle and claim bus. And send start bit and slave addr
            if (m_bus_claimed != true) {

                // If BUS is idle, claim the BUS
                if (is_bus_free()) {
                    claim_bus();
                }
                
                // Start the SCL clock generation
                m_clk_gen_ev.notify();

                // Send the start bit
                send_start_bit();

                // Send the data
                sc_bv<8> slave_addr = m_slave_addr; // 7-bit address
                sc_bit control_bit = 0;
                slave_addr[8] = control_bit; // 8th bit is control bit = 0 to indicate write operation (tx operation)
                serilaize(slave_addr);

            }

            serialize(c);

            // Check if acknowledgement is received
            check_acknowlement();

            // If acknowledgement not received, try for few more times and if failed, send stop bit
            if (m_ack_recvd)
                break;
            else {
                m_retry_cnt++;
                // Code to retry again..
                if (m_retry_cnt == 3) {
                    m_retry_cnt = 0;
                    break;
                }
            }

            } while (true);

            // If multiple retries failed, then send stop bit and stop further transmission
            if (m_retry_cnt == 3) {
                send_stop_bit();
                m_retry_cnt = 0;
                break;
            }

            // Go back to while (true) and wait for next data to be sent
        }
    }

    void send_start_bit() {
        if (SCL == 0) {
            SDA = 1;
            wait(SCL.posedge_event());
            
            // Make transtion of SDA from 1 to 0 during positive level of SCL
            wait(m_scl_period / 3.0);
            SDA = 0;
        }
    }


    void send_stop_bit() {
        if (SCL == 0) {
            SDA = 0;
            wait(SCL.posedge_event());
            
            // Make transtion of SDA from 0 to 1 during positive level of SCL
            wait(m_scl_period / 3.0);
            SDA = 1;

            // Send signal to generate_scl() process to stop SCL clock generation
            m_clk_gen_stop = true;
        }
    }

    void serialize(const sc_bv<8>& tx_byte) {
        for (int i = 0; i < 8; ++i) {
            wait(SCL.negedge_event());
            SDA.write(tx_byte[i]);
        }
        wait(SCL.posedge_event());  // Consume the SCL posedge event too, so that ACK can be expected at next posedge
    }


    void check_acknowlement() {
        wait(SCL.posedge_event());
        if (SDA == 0) {   // ACK received if SDA is 0 during posedge of SCL
            m_ack_recvd = true;
        } else
            m_ack_recvd = false;
    }

    void generate_scl() {
        while (true) {
            // Wait for tx_data() process to signal SCL generation event
            wait(m_clk_gen_ev);

            // Generate the SCL clock with period = m_scl_period
            while (true) {
                SCL.write(0);
                wait(m_scl_period/2.0);
                SCL.write(1);
                wait(m_scl_period/2.0);
                if (m_clk_gen_stop) {
                    m_clk_gen_stop = false;
                    release_bus();
                    break;
                }
            }
        }
    }

    SC_CTOR(I2C_Master_Core) {
        bus_claimed = false;
        m_state = M_IDLE;
        SC_THREAD(tx_data);
    }

};
