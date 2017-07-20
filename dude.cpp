#include "dude.h"

using namespace std;

Dude::Dude() // : local_sig_exec_done()
{
        avrdude_thread = nullptr;
        local_sig_exec_done.connect(sigc::mem_fun(*this, &Dude::execution_end));
}

Dude::~Dude()
{
}

Dude::type_sig_exec_done Dude::signal_exec_done()
{
        return sig_exec_done;
}

Dude::type_sig_exec_started Dude::signal_exec_started()
{
        return sig_exec_started;
}

void Dude::setup ( bool auto_erase, bool auto_verify, bool auto_check, string programmer, string microcontroller )
{
        execution_status = no_error;
        raw_exec_output.clear();
        processed_output.clear();
        device.clear();
        protocol.clear();
        options.clear();
        oneliner.clear();

        // set device parameter
        device.append(" -p ");
        // guess family and device name
        if (microcontroller.find ("ATtiny", 0) == 0) {
                device.append("t");
                device.append(microcontroller.substr(6, 32));
        } else if (microcontroller.find ("ATmega", 0) == 0) {
                device.append("m");
                device.append(microcontroller.substr(6, 32));
        } else if (microcontroller.find ("AT90S", 0) == 0) {
                device.append(microcontroller.substr(5, 32));
        } else if (microcontroller.find ("AT90PWM", 0) == 0) {
                device.append("pwm");
                device.append(microcontroller.substr(7, 32));
        } else if (microcontroller.find ("AT90CAN", 0) == 0) {
                device.append("c");
                device.append(microcontroller.substr(7, 32));
        } else if (microcontroller.find ("AT90USB", 0) == 0) {
                device.append("usb");
                device.append(microcontroller.substr(7, 32));
        }

        // set protocol parameter
        protocol.append(" -c ").append(programmer).append(" ");

        // check if should disable automatic erase of flash
        if (!auto_erase)
                options.append("-D ");
        // check if should disable automatic checking of signature
        if (!auto_check)
                options.append("-F ");
        // check if should disable automatic verification of written data
        if (!auto_verify)
                options.append("-V ");
        // disable safemode prompting
        options.append("-s ");
        // disable progress-bars and all unecessary messages
        options.append("-q ");

        // prepare initial part of command to be exetuded
        oneliner.append("avrdude");
        oneliner.append(device);
        oneliner.append(protocol);
        oneliner.append(options);

        //cout << "protocol: " << protocol << endl;
        //cout << "device: " << device << endl;
        //cout << "options: " << options<< endl;
}

void Dude::do_read_signature (void)
{
        // prepare command to be executed
        command.clear();
        command.append(oneliner);
        command.append("-F -u "); // disable signature and fuse checks
        // execute command
        execute ();

        // check output for errors
        check_for_errors();
        // only proceed if execution was successful
        if (execution_status == no_error) {
                // find signature position
                string::size_type sig_pos = raw_exec_output.find ("signature = 0x", 0);
                // extract signature
                processed_output = raw_exec_output.substr(sig_pos + 14, 6);
        }
}

void Dude::do_clear_device (void)
{
        // prepare command to be executed
        command.clear();
        command.append(oneliner);
        command.append("-e "); // chip erase
        command.append("-u "); // disable fuse checking
        // execute command
        execution_begin ();
}

void Dude::execute (void)
{
        // clear output messages from previous execution
        raw_exec_output.clear();
        processed_output.clear();
        // temporary buffer for gathiering output messages
        char line[200];
        // make sure all messages will be fed into stdout
        command.append(" 2>&1");
        // execute command and gather all output
        FILE *stream = popen(command.c_str(), "r");
        if (stream) {
                while (!feof(stream))
                        if (fgets(line, sizeof(line), stream) != NULL)
                                raw_exec_output.append(line);
                pclose(stream);
        }
        // add executed command at the beginning of output string
        raw_exec_output = "$ " + command + "\n" + raw_exec_output;
        // debug print-out
        //cout << raw_exec_output << endl;

        // check if in thread...
        if (avrdude_thread)
                // emit signal for execution completion (notify *this* object)
                local_sig_exec_done.emit();
}

void Dude::execution_begin (void)
{
        if (avrdude_thread) {
                cout << "An avrdude thread is already active... Cannot start an extra one!" << endl;
        } else {
                // create thread for avrdude execution
                avrdude_thread = new thread( [this] { execute(); } );
                // emit signal for execution start (notify caller object)
                sig_exec_started.emit();
        }
}

void Dude::execution_end (void)
{
        //cout << "DUDE: execution finished!" << endl;

        // synchronize threads and delete the one used by avrdude
        if (avrdude_thread->joinable())
                avrdude_thread->join();
        delete avrdude_thread;
        avrdude_thread = nullptr;

        // check output for errors
        check_for_errors();

        // emit signal for execution completion (notify caller object)
        sig_exec_done.emit();
}

void Dude::do_eeprom_write (string file)
{
        // prepare command to be executed
        command.clear();
        command.append(oneliner);
        command.append("-U eeprom:w:\"" + file + "\":a"); // write file to eeprom
        // execute command
        execution_begin ();
}

void Dude::do_eeprom_read (string file)
{
        // prepare command to be executed
        command.clear();
        command.append(oneliner);
        command.append("-U eeprom:r:\"" + file + "\":h"); // copy eeprom to file
        // execute command
        execution_begin ();
}

void Dude::do_eeprom_verify (string file)
{
        // prepare command to be executed
        command.clear();
        command.append(oneliner);
        command.append("-U eeprom:v:\"" + file + "\":a"); // verify eeprom against file
        // execute command
        execution_begin ();
}

void Dude::do_flash_write (string file)
{
        // prepare command to be executed
        command.clear();
        command.append(oneliner);
        command.append("-U flash:w:\"" + file + "\":a"); // write file to flash
        // execute command
        execution_begin ();
}

void Dude::do_flash_read (string file)
{
        // prepare command to be executed
        command.clear();
        command.append(oneliner);
        command.append("-U flash:r:\"" + file + "\":r"); // copy flash to file
        // execute command
        execution_begin ();
}

void Dude::do_flash_verify (string file)
{
        // prepare command to be executed
        command.clear();
        command.append(oneliner);
        command.append("-U flash:v:\"" + file + "\":a"); // verify flash against file
        // execute command
        execution_begin ();
}

void Dude::do_fuse_write (int fusebytes_count, int low, int high, int ext)
{
        // prepare command to be executed
        command.clear();
        command.append(oneliner);
        ostringstream steam_string;
        switch (fusebytes_count) {
                case (1): {
                        // LOW fuse byte
                        steam_string << "-U lfuse:w:" << low << ":m -U";
                        break;
                }
                case (2): {
                        // LOW and HIGH fuse bytes
                        steam_string << "-U lfuse:w:" << low << ":m -U hfuse:w:" << high << ":m";
                        break;
                }
                default: {
                        // LOW, HIGH and EXTENDED fuse bytes
                        steam_string << "-U lfuse:w:" << low << ":m -U hfuse:w:" << high << ":m -U efuse:w:" << ext << ":m";
                        break;
                }
        }
        command.append(steam_string.str());
        // execute command
        execution_begin ();
}

void Dude::do_fuse_read (int fusebytes_count)
{
        // prepare command to be executed
        command.clear();
        command.append(oneliner);
        switch (fusebytes_count) {
                case (1): {
                        // LOW fuse byte
                        command.append("-q -U lfuse:r:-:d");
                        break;
                }
                case (2): {
                        // LOW and HIGH fuse bytes
                        command.append("-q -U lfuse:r:-:d -U hfuse:r:-:d");
                        break;
                }
                default: {
                        // LOW, HIGH and EXTENDED fuse bytes
                        command.append("-q -U lfuse:r:-:d -U hfuse:r:-:d -U efuse:r:-:d");
                        break;
                }
        }
        // execute command
        execute ();

        // clear fuse-bytes' values
        dev_fusebytes[0] = 255;
        dev_fusebytes[1] = 255;
        dev_fusebytes[2] = 255;
        // check output for errors
        check_for_errors();
        if (execution_status != no_error)
                return;
        // extract fuse-byte values from execution output
        int char_counter = 0;
        int lines_extracted = 0;
        string line;
        // parse execution output line by line, starting from the end...
        for (int iter = raw_exec_output.size() - 2; iter > 0; iter--) {
                char_counter++;
                if (raw_exec_output[iter] == '\n') {
                        // update extracted lines' count
                        lines_extracted++;
                        // extract line
                        line = raw_exec_output.substr(iter + 1, char_counter - 1);
                        char_counter = 0;
                        // get value from extracted line
                        stringstream(line) >> dev_fusebytes[fusebytes_count - lines_extracted];
                        // check if received all values
                        if (lines_extracted == fusebytes_count)
                                break;
                }
        }
        //for (int i = 0; i < 3; i++)
        //        cout << " :" << dev_fusebytes[i];
        //cout << endl;
}

void Dude::check_for_errors (void)
{
        uint index; // error_strings is a list... list-indices are unsigned!!
        uint max_index = error_strings.size();
        int error_code_index = -1;
        string::size_type er_str_pos;
        string::size_type output_len = raw_exec_output.size();

        // iterate through available error-strings and check if found in execution output string
        for (index = 0; index < max_index; index++) {
                er_str_pos = raw_exec_output.find (error_strings[index], 0);
                if (er_str_pos < output_len) {
                        error_code_index = index;
                        break;
                }

        }

        // check if no string found in output
        if (error_code_index == -1) {
                execution_status = no_error;
                return;
        }

        execution_status = error_codes[error_code_index];
}
