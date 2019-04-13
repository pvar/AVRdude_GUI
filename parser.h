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
                bool is_valid (xmlpp::Node *root_node);
                std::string get_signature_bytes (xmlpp::Node* signature_node);

                status get_specs (xmlpp::Node *root_node, DeviceDescription &description);
                status get_settings (xmlpp::Node *root_node, DeviceDescription &description);
                status get_warnings (xmlpp::Node *root_node, DeviceDescription &description);
                status get_default (xmlpp::Node *root_node, DeviceDescription &description);

                status get_content (std::string filename, xmlpp::DomParser &parser);

                xmlpp::Node* get_child_with_attr (xmlpp::Node* starting_node, std::string att_name, std::string att_value);

                std::list<DeviceDescription::OptionEntry>* get_enum_list (xmlpp::Node* xml_node);
                std::list<DeviceDescription::FuseSetting>* get_fuse_list (xmlpp::Node* xml_node, unsigned int offset);

                std::string get_txt_value (xmlpp::Node* starting_node);
                std::string get_att_value (xmlpp::Node* xml_node, std::string att_name);
                std::string float_to_string (float number);

                void print_warnings (DeviceDescription &description);
                void print_defaults (DeviceDescription &description);
                void print_settings (DeviceDescription &description);
                void print_options (DeviceDescription &description);
};

#endif
