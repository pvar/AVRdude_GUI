#ifndef DEVICEDATA_H
#define DEVICEDATA_H

#include <map>
#include <list>

class DeviceDescription {
        public:
                // description of a single fuse warning
                struct FuseWarning {
                        uint fbyte;
                        uint fmask;
                        uint fresult;
                        std::string warning;
                };

                // description of a single fuse setting
                struct FuseSetting {
                        bool single_option; // wether it's a boolean or an enumerator
                        std::string fname;      // setting name
                        std::string fdesc;      // setting description
                        std::string fenum;      // enumerator name or null
                        uint fmask;             // bit-mask
                        uint offset;            // fuse byte
                };

                // description of an one-of-many options in a single fuse setting
                struct OptionEntry {
                        std::string ename;      // entry name
                        uint value;             // entry value
                };

                DeviceDescription();
                virtual ~DeviceDescription();

                // fundamental MCU properties
                bool eeprom_exists;
                std::string max_speed;
                std::string sram_size;
                std::string flash_size;
                std::string eeprom_size;
                std::string signature;
                std::string xml_filename;
                std::string xml_version;

                // fuse settings
                std::list<FuseSetting> *fuse_settings;
                std::map <std::string, std::list<OptionEntry>*> *option_lists;
                int fusebytes_count;

                // default fuse values
                int fusebytes_default[3];

                // fuse warnings
                std::list<FuseWarning> *warnings;
};

#endif
