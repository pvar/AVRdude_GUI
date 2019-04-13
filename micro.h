#ifndef MICRO_H
#define MICRO_H

#include <libxml++/libxml++.h>
#include <dirent.h>
#include <iostream>
#include <fstream>
#include "devdesc.h"
#include "parser.h"

class Micro
{
        public:
                Micro(std::string path);
                virtual ~Micro();

                void parse_data(std::string xml_file);
                void get_device_list ();

                // device description extracted from xml/atdf file
                DeviceDescription *description = nullptr;

                // user defined fuse settings
                int fusebytes_custom[3] = {255, 255, 255};

                // map: device --> description-file
                std::map <std::string, std::string> *device_map = nullptr;

        protected:
                std::string exec_path;

                void save_xml_map (void);
                bool load_xml_map (void);
};

#endif
