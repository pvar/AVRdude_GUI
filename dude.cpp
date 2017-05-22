#include "dude.h"

Dude::Dude()
{
}

Dude::~Dude()
{
}

void Dude::setup ( gboolean auto_erase, gboolean auto_verify, gboolean auto_check, Glib::ustring programmer, Glib::ustring microcontroller )
{
        exec_error = no_error;
        raw_exec_output.clear();
        processed_output.clear();
        device.clear();
        protocol.clear();
        options.clear();

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
        /* disable progress-bars */
        options.append("-q ");

        //cout << "protocol: " << protocol << endl;
        //cout << "device: " << device << endl;
        //cout << "options: " << options<< endl;
}

void Dude::get_signature (void)
{
        Glib::ustring command, tmp_string;

        /* prepare command to be executed */
        command.append("avrdude");
        command.append(device);
        command.append(protocol);
        command.append(options);
        /* extra parameters for this operation (disable signature and fuse checks) */
        command.append("-F -u ");
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
        Glib::ustring command;

        /* prepare command to be executed */
        command.append("avrdude");
        command.append(device);
        command.append(protocol);
        command.append(options);
        /* parameter for executing a chip erase */
        command.append("-e ");
        /* extra parameters for this operation (disable fuse checking) */
        command.append("-u ");
        /* execute command */
        execute (command);
        /* check output for errors */
        check_for_errors();
}

void Dude::execute (Glib::ustring command)
{
        /* clear all output from previous execution */
        raw_exec_output.clear();
        processed_output.clear();
        /* temporary buffer for gathiering output */
        char line[256];
        /* make sure all messages will be fed into stdout */
        command.append(" 2>&1");
        /* execute command and gather all output */
        FILE *stream = popen(command.c_str(), "r");
        if (stream) {
                while (!feof(stream))
                        if (fgets(line, 256, stream) != NULL)
                                raw_exec_output.append(line);
                pclose(stream);
        }
        /* add executed command at the beginning of output string */
        raw_exec_output = "> " + command + "\n" + raw_exec_output;
}

void Dude::check_for_errors (void)
{
        Glib::ustring::size_type output_len = raw_exec_output.size();
        Glib::ustring::size_type substr_pos;

        /* check if signature was not read */
        substr_pos = raw_exec_output.find ("error reading signature data", 0);
        if (substr_pos < output_len) {
                processed_output = "cannot read signature!";
                exec_error = cannot_read_signature;
                return;
        }

        /* check if encountered unexpected singature */
        substr_pos = raw_exec_output.find ("Double check chip, or use -F to override this check.", 0);
        if (substr_pos < output_len) {
                processed_output = "unexpected singature!";
                exec_error = invalid_signature;
                return;
        }

        /* check if an invalid part was declared */
        substr_pos = raw_exec_output.find ("Valid parts are:", 0);
        if (substr_pos < output_len) {
                processed_output = "unknown device!";
                exec_error = unknown_device;
                return;
        }

        /* check if avrdude executable is not present */
        substr_pos = raw_exec_output.find ("command not found", 0);
        if (substr_pos < output_len) {
                processed_output = "command not found!";
                exec_error = command_not_found;
                return;
        }

        /* check if user lacks the necessary permissions */
        /*
        substr_pos = raw_exec_output.find ("command not found", 0);
        if (substr_pos < output_len) {
                processed_output = "insufficient permissions";
                exec_error = insufficient_permissions;
                return;
        }
        */

        /* check for other errors... */

        exec_error = no_error;
}
