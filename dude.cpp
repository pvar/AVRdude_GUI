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

void Dude::setup ( gboolean auto_erase, gboolean auto_verify, gboolean auto_check, Glib::ustring programmer, Glib::ustring microcontroller )
{
        exec_error = no_error;
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

void Dude::sig_read (void)
{
        // prepare command to be executed
        Glib::ustring tmp_string;
        command.clear();
        command.append(oneliner);
        command.append("-F -u "); // disable signature and fuse checks
        // execute command
        execute ();

        // check output for errors
        check_for_errors();
        // only proceed if execution was successful
        if (exec_error == no_error) {
                // find signature position
                Glib::ustring::size_type sig_pos = raw_exec_output.find ("signature = 0x", 0);
                // extract signature
                tmp_string = raw_exec_output.substr(sig_pos + 14, 6);
                processed_output = tmp_string.uppercase();
        }
}

void Dude::dev_clear (void)
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
        raw_exec_output = "> " + command + "\n" + raw_exec_output;

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

void Dude::eeprom_write (Glib::ustring file)
{
        cout << "eeprom write!" << endl;
        // prepare command to be executed
        command.clear();
        command.append(oneliner);
        command.append("-U eeprom:w:\"" + file + "\":a"); // write file to eeprom
        // execute command
cout << command << endl;
        execution_begin ();
}

void Dude::eeprom_read (Glib::ustring file)
{
        cout << "eeprom read!" << endl;
        // prepare command to be executed
        command.clear();
        command.append(oneliner);
        command.append("-U eeprom:r:\"" + file + "\":h"); // copy eeprom to file
        // execute command
cout << command << endl;
        execution_begin ();
}

void Dude::eeprom_verify (Glib::ustring file)
{
        cout << "eeprom verify!" << endl;
        // prepare command to be executed
        command.clear();
        command.append(oneliner);
        command.append("-U eeprom:v:\"" + file + "\":a"); // verify eeprom against file
        // execute command
cout << command << endl;
        execution_begin ();
}

void Dude::flash_write (Glib::ustring file)
{
        cout << "flash write!" << endl;
        // prepare command to be executed
        command.clear();
        command.append(oneliner);
        command.append("-U flash:w:\"" + file + "\":a"); // write file to flash
        // execute command
cout << command << endl;
        execution_begin ();
}

void Dude::flash_read (Glib::ustring file)
{
        cout << "flash read!" << endl;
        // prepare command to be executed
        command.clear();
        command.append(oneliner);
        command.append("-U flash:r:\"" + file + "\":r"); // copy flash to file
        // execute command
cout << command << endl;
        execution_begin ();
}

void Dude::flash_verify (Glib::ustring file)
{
        cout << "flash verify!" << endl;
        // prepare command to be executed
        command.clear();
        command.append(oneliner);
        command.append("-U flash:v:\"" + file + "\":a"); // verify flash against file
        // execute command
cout << command << endl;
        execution_begin ();
}

void Dude::fuse_write (Glib::ustring data)
{
        cout << "fuse write!" << endl;
        // get number of fuse bytes

        // prepare command to be executed
        command.clear();
        command.append(oneliner);
        command.append("-U lfuse:w:value:m -U hfuse:w:value:m -U efuse:w:value:m"); // write fuse bytes
        // execute command
cout << command << endl;
        execution_begin ();
}

void Dude::fuse_read (void)
{
        cout << "fuse read!" << endl;
        // get number of fuse bytes

        // prepare command to be executed
        command.clear();
        command.append(oneliner);
        command.append("-U lfuse:r:-:d -U hfuse:r:-:d -U efuse:r:-:d -q"); // read fuse bytes
        // execute command
cout << command << endl;
        execution_begin ();
}

void Dude::check_for_errors (void)
{
        guint index; // er_strings is a list and list-indices are unsigned!
        guint max_index = er_strings.size();
        gint error_code_index = -1;
        Glib::ustring::size_type er_str_pos;
        Glib::ustring::size_type output_len = raw_exec_output.size();

        // iterate through available error-strings and check if found in execution output string
        for (index = 0; index < max_index; index++) {
                er_str_pos = raw_exec_output.find (er_strings[index], 0);
                if (er_str_pos < output_len) {
                        error_code_index = index;
                        break;
                }

        }

        // check if no string found in output
        if (error_code_index == -1) {
                exec_error = no_error;
                return;
        }

        exec_error = er_codes[error_code_index];
}
