#include "dude.h"

Dude::Dude()
{
}

Dude::~Dude()
{
}

void Dude::setup ( gboolean auto_erase, gboolean auto_verify, gboolean auto_check, Glib::ustring programmer, Glib::ustring microcontroller )
{
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
        device.append(" -c ").append(programmer).append(" ");

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


Glib::ustring Dude::get_signature (void)
{
        Glib::ustring command, output, actual_signature;

        /* prepare command to be executed */
        command.append("avrdude");
        command.append(device);
        command.append(protocol);
        command.append(options);
        /* parameters to disable some extra checks for this operation */
        command.append("-F -u ");
        /* execute command and get output */
        output = execute (command);
        /* find signature location in output */
        Glib::ustring::size_type sig_pos = output.find ("signature = 0x", 0);
        /* extract signature */
        actual_signature = output.substr (sig_pos + 14, 6);

        //cout << "\n\nraw output: " << output << "\n\n" << endl;
        //cout << "executed command: " << command << endl;
        //cout << "extracted signature: " << actual_signature << endl;

        return actual_signature.uppercase();
}


Glib::ustring Dude::device_erase (void)
{
        Glib::ustring command, output;

        /* execute command and get output */
        output = execute (command);

        return output;
}

Glib::ustring Dude::execute (Glib::ustring command)
{
        char line[256];
        Glib::ustring output;

        command.append(" 2>&1");
        FILE *stream = popen(command.c_str(), "r");
        if (stream) {
                while (!feof(stream))
                        if (fgets(line, 256, stream) != NULL)
                                output.append(line);
                pclose(stream);
        }
        return output;
}
