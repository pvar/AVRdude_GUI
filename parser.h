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

                // device-description file parser
                status check_file (std::string filename);
                // device-description file parser
                status process_file (std::string filename, DeviceDescription &description);

        protected:
                virtual bool is_valid (xmlpp::Node *root_node) = 0;
                virtual status get_specs (xmlpp::Node *root_node, DeviceDescription &description) = 0;
                virtual status get_settings (xmlpp::Node *root_node, DeviceDescription &description) = 0;
                virtual status get_warnings (xmlpp::Node *root_node, DeviceDescription &description) = 0;
                virtual status get_default (xmlpp::Node *root_node, DeviceDescription &description) = 0;

                status get_content (std::string filename, xmlpp::DomParser &parser);
                void print_warnings (DeviceDescription &description);
                void print_defaults (DeviceDescription &description);
                void print_settings (DeviceDescription &description);
                void print_options (DeviceDescription &description);
};

#endif
