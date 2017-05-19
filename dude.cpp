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

        //cout << microcontroller << " on " << programmer << endl;

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
        cout << "device name: -" << device << "-" << endl;



        /* keep protocol in private member */
        protocol.append(programmer);

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
