#include "dude.h"

Dude::Dude() // : local_sig_exec_done()
{
        local_sig_exec_done.connect(sigc::mem_fun(*this, &Dude::post_execution));
}

Dude::~Dude()
{
}

Dude::type_sig_exec_done Dude::signal_exec_done()
{
        return sig_exec_done;
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

        /* set device parameter */
        device.append(" -p ");
        /* guess family and device name */
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

        /* set protocol parameter */
        protocol.append(" -c ").append(programmer).append(" ");

        /* check if should disable automatic erase of flash */
        if (!auto_erase)
                options.append("-D ");
        /* check if should disable automatic checking of signature */
        if (!auto_check)
                options.append("-F ");
        /* check if should disable automatic verification of written data */
        if (!auto_verify)
                options.append("-V ");
        /* disable safemode prompting */
        options.append("-s ");
        /* disable progress-bars and all unecessary messages */
        options.append("-q ");

        /* prepare initial part of command to be exetuded */
        oneliner.append("avrdude");
        oneliner.append(device);
        oneliner.append(protocol);
        oneliner.append(options);

        //cout << "protocol: " << protocol << endl;
        //cout << "device: " << device << endl;
        //cout << "options: " << options<< endl;
}

void Dude::get_signature (void)
{
        /* prepare command to be executed */
        Glib::ustring command, tmp_string;
        command.append(oneliner);
        command.append("-F -u "); // disable signature and fuse checks
        /* execute command */
        execute (command);

        /* check output for errors */
        check_for_errors();
        /* only proceed if execution was successful */
        if (exec_error == no_error) {
                /* find signature position */
                Glib::ustring::size_type sig_pos = raw_exec_output.find ("signature = 0x", 0);
                /* extract signature */
                tmp_string = raw_exec_output.substr(sig_pos + 14, 6);
                processed_output = tmp_string.uppercase();
        }
}

void Dude::device_erase (void)
{
        /* prepare command to be executed */
        Glib::ustring command;
        command.append(oneliner);
        command.append("-e "); // chip erase
        command.append("-u "); // disable fuse checking
        /* execute command */
        execute (command);
}

void Dude::execute (Glib::ustring command)
{
        /* clear output messages from previous execution */
        raw_exec_output.clear();
        processed_output.clear();
        /* temporary buffer for gathiering output messages */
        char line[200];
        /* make sure all messages will be fed into stdout */
        command.append(" 2>&1");
        /* execute command and gather all output */
        FILE *stream = popen(command.c_str(), "r");
        if (stream) {
                while (!feof(stream))
                        if (fgets(line, sizeof(line), stream) != NULL)
                                raw_exec_output.append(line);
                pclose(stream);
        }
        /* add executed command at the beginning of output string */
        raw_exec_output = "> " + command + "\n" + raw_exec_output;

        /* emit signal for execution completion -- notify function of this object */
        local_sig_exec_done.emit();
}

void Dude::post_execution (void)
{
        cout << "DUDE: execution finished!" << endl;
        /* check output for errors */
        check_for_errors();

        /* emit signal for execution completion -- notify function of caller object */
        sig_exec_done.emit();
}

void Dude::eeprom_write (Glib::ustring file)
{
        cout << "eeprom write!" << endl;
        /* prepare command to be executed */
        Glib::ustring command;
        command.append(oneliner);
        command.append("-U eeprom:w:\"" + file + "\":a"); // write file to eeprom
        /* execute command */
cout << command << endl;
        execute (command);
}

void Dude::eeprom_read (Glib::ustring file)
{
        cout << "eeprom read!" << endl;
        /* prepare command to be executed */
        Glib::ustring command;
        command.append(oneliner);
        command.append("-U eeprom:r:\"" + file + "\":h"); // copy eeprom to file
        /* execute command */
cout << command << endl;
        execute (command);
}

void Dude::eeprom_verify (Glib::ustring file)
{
        cout << "eeprom verify!" << endl;
        /* prepare command to be executed */
        Glib::ustring command;
        command.append(oneliner);
        command.append("-U eeprom:v:\"" + file + "\":a"); // verify eeprom against file
        /* execute command */
cout << command << endl;
        execute (command);
}

void Dude::flash_write (Glib::ustring file)
{
        cout << "flash write!" << endl;
        /* prepare command to be executed */
        Glib::ustring command;
        command.append(oneliner);
        command.append("-U flash:w:\"" + file + "\":a"); // write file to flash
        /* execute command */
cout << command << endl;
        execute (command);
}

void Dude::flash_read (Glib::ustring file)
{
        cout << "flash read!" << endl;
        /* prepare command to be executed */
        Glib::ustring command;
        command.append(oneliner);
        command.append("-U flash:r:\"" + file + "\":r"); // copy flash to file
        /* execute command */
cout << command << endl;
        execute (command);
}

void Dude::flash_verify (Glib::ustring file)
{
        cout << "flash verify!" << endl;
        /* prepare command to be executed */
        Glib::ustring command;
        command.append(oneliner);
        command.append("-U flash:v:\"" + file + "\":a"); // verify flash against file
        /* execute command */
cout << command << endl;
        execute (command);
}

void Dude::fuse_write (Glib::ustring data)
{
        cout << "fuse write!" << endl;
        /* get number of fuse bytes */

        /* prepare command to be executed */
        Glib::ustring command;
        command.append(oneliner);
        command.append("-U lfuse:w:value:m -U hfuse:w:value:m -U efuse:w:value:m"); // write fuse bytes
        /* execute command */
cout << command << endl;
        execute (command);
}

void Dude::fuse_read (void)
{
        cout << "fuse read!" << endl;
        /* get number of fuse bytes */

        /* prepare command to be executed */
        Glib::ustring command;
        command.append(oneliner);
        command.append("-U lfuse:r:-:d -U hfuse:r:-:d -U efuse:r:-:d -q"); // read fuse bytes
        /* execute command */
cout << command << endl;
        execute (command);
}

void Dude::check_for_errors (void)
{
        gint index, error_code_index = -1;
        gint max_index = er_strings.size();
        Glib::ustring::size_type er_str_pos;
        Glib::ustring::size_type output_len = raw_exec_output.size();

        /* iterate through available error-strings and check if found in execution output string */
        for (index = 0; index < max_index; index++) {
                er_str_pos = raw_exec_output.find (er_strings[index], 0);
                if (er_str_pos < output_len) {
                        error_code_index = index;
                        break;
                }

        }

        /* check if no string found in output */
        if (error_code_index == -1) {
                exec_error = no_error;
                return;
        }

        exec_error = er_codes[error_code_index];
}
