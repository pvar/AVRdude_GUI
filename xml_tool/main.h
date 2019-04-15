#ifndef MAIN_H
#define MAIN_H

#include <libxml++/libxml++.h>
#include <unistd.h>
#include <dirent.h>
#include <iostream>
#include <iomanip>
#include <fstream>

class Translator
{
        public:
                Translator();
                virtual ~Translator();

                void process_files (void);

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
                        std::string fname;  // setting name
                        std::string fdesc;  // setting description
                        std::string fenum;  // enumerator name or null
                        uint fmask;         // bit-mask
                        uint offset;        // fuse byte
                };

                // description of an one-of-many options in a single fuse setting
                struct OptionEntry {
                        std::string ename;      // entry name
                        uint value;             // entry value
                };

        protected:
                // fundamental MCU properties
                bool eeprom_exists;
                std::string device_name;
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

                std::string exec_path;

                bool create_xml (void);
                bool convert (std::string filename, std::string filename_part);
                bool openfile (std::string filename, xmlpp::DomParser &parser);
                bool get_settings (xmlpp::Node *root_node);
                bool get_warnings (xmlpp::Node *root_node);
                bool get_def_set (xmlpp::Node *root_node);
                bool get_specs (xmlpp::Node *root_node);
                bool is_description (std::string filename, std::string &device_name);
                bool is_dev_desc (xmlpp::Node *root_node);

                std::list<FuseSetting>* get_fuse_list (xmlpp::Node* xml_node, unsigned int offset);
                std::list<OptionEntry>* get_enum_list (xmlpp::Node* xml_node);

                std::string get_signature_bytes (xmlpp::Node* signature_node);
                std::string get_txt_value (xmlpp::Node* starting_node);
                std::string get_att_value (xmlpp::Node* xml_node, std::string att_name);
                std::string float_to_string (float number);

                xmlpp::Node* get_child_with_attr (xmlpp::Node* starting_node, std::string att_name, std::string att_value);
};

#endif
