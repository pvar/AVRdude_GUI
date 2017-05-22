#ifndef DUDE_H
#define DUDE_H

#include <glibmm.h>
#include <iostream>

using namespace std;

enum memory_type {
                 flash,
                 eeprom,
                 fuse
                 };

enum error_codes {
                no_error,
                invalid_signature,
                unknown_device,
                cannot_read_signature,
                command_not_found,
                insufficient_permissions
                };

class Dude
{
        public:
                Dude();
                virtual ~Dude();

                void setup (gboolean auto_erase,
                            gboolean auto_verify,
                            gboolean auto_check,
                            Glib::ustring programmer,
                            Glib::ustring microcontroller);

                Glib::ustring raw_exec_output;
                Glib::ustring processed_output;
                error_codes exec_error;

                void device_erase (void);
                void get_signature (void);
                Glib::ustring device_write (Glib::ustring file, gint target);
                Glib::ustring device_read (Glib::ustring file, gint source);

        protected:
                Glib::ustring protocol;
                Glib::ustring options;
                Glib::ustring device;

                void execute (Glib::ustring command);
                void check_for_errors (void);
};

#endif
