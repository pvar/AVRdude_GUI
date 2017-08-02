#ifndef PARSER_v1_H
#define PARSER_v1_H

#include "parser.h"

class Parser_v1 : public Parser
{
        protected:
                bool is_valid (xmlpp::Node *root_node);
                status get_specs (xmlpp::Node *root_node, DeviceDescription &description);
                status get_settings (xmlpp::Node *root_node, DeviceDescription &description);
                status get_warnings (xmlpp::Node *root_node, DeviceDescription &description);
                status get_default (xmlpp::Node *root_node, DeviceDescription &description);

                std::list<DeviceDescription::OptionEntry>* get_enum_list (xmlpp::Node* xml_node);
                std::list<DeviceDescription::FuseSetting>* get_fuse_list (xmlpp::Node* xml_node, uint offset);

                std::string get_signature_bytes (xmlpp::Node* signature_node);

                xmlpp::Node* get_child_with_attr (xmlpp::Node* starting_node, std::string att_name, std::string att_value);

                std::string get_txt_value (xmlpp::Node* starting_node);

                std::string get_att_value (xmlpp::Node* xml_node, std::string att_name);

                std::string float_to_string (float number);
};

#endif
