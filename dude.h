#ifndef DUDE_H
#define DUDE_H

#include <glibmm.h>
#include <iostream>

using namespace std;

enum memory_type { flash, eeprom, fuse };

class Dude
{
        public:
                Dude();
                virtual ~Dude();

                void setup( gboolean auto_erase,
                            gboolean auto_verify,
                            gboolean auto_check,
                            Glib::ustring protocol,
                            Glib::ustring device );

                Glib::ustring device_write (Glib::ustring file, gint target);
                Glib::ustring device_read (Glib::ustring file, gint source);
                Glib::ustring device_erase (void);
                Glib::ustring get_signature (void);

        protected:
                Glib::ustring protocol;
                Glib::ustring options;
                Glib::ustring device;

                Glib::ustring execute (Glib::ustring command);
};

#endif
