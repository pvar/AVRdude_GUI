#ifndef PARSER_v1_H
#define PARSER_v1_H

#include "parser.h"

class Parser_v1 : public Parser
{
        public:
                // check if device-description file
                bool is_description (std::string filename, std::string &device_name);

        protected:
                bool is_valid (xmlpp::Node *root_node);
                std::string get_signature_bytes (xmlpp::Node* signature_node);
                status get_specs (xmlpp::Node *root_node, DeviceDescription &description);
                status get_settings (xmlpp::Node *root_node, DeviceDescription &description);
                status get_warnings (xmlpp::Node *root_node, DeviceDescription &description);
                status get_default (xmlpp::Node *root_node, DeviceDescription &description);
};

#endif
