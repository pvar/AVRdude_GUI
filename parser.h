#ifndef PARSER_H
#define PARSER_H

#include <libxml++/libxml++.h>
#include <iostream>
#include <fstream>
#include "devdesc.h"

class Parser
{
        public:
                // parser status
                enum status { success, io_error, doc_error, no_content };

                // check if it's really a device-description file
                bool is_description (std::string filename, std::string &device_name);

                // device-description file parser
                status process_file (std::string filename, DeviceDescription &description);

        protected:
                status get_content (std::string filename, xmlpp::DomParser &parser);

                status get_metadata (xmlpp::Node *root_node, DeviceDescription &description);
                status get_default (xmlpp::Node *root_node, DeviceDescription &description);
                status get_warnings (xmlpp::Node *root_node, DeviceDescription &description);
                status get_settings (xmlpp::Node *root_node, DeviceDescription &description);
};

#endif
