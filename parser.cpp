#include "parser.h"

using namespace std;

Parser::status Parser::process_file (string filename, DeviceDescription &description)
{
        if (filename == "") {
                cout << "\nno device-description file specified" << endl;
                return status::io_error;
        }

        xmlpp::DomParser parser;

        status read_file = get_content (filename, parser);
        if (read_file != status::success)
                return read_file;

        xmlpp::Node *root_node = parser.get_document()->get_root_node();

        get_metadata (root_node, description);
        get_default  (root_node, description);
        get_warnings (root_node, description);
        get_settings (root_node, description);

        return status::success;
}


Parser::status Parser::get_content (std::string filename, xmlpp::DomParser &parser)
{
        // attempt to open specified file
        try {
                parser.parse_file(filename);
        } catch(const exception& ex) {
                cout << "Exception caught, trying to read file: " << ex.what() << endl;
                return status::io_error;
        }

        // check if content found
        if (!parser)
                return status::no_content;

        return status::success;
}

bool Parser::is_description (string filename, std::string &device_name)
{
        bool desc_file = false;

        xmlpp::DomParser parser;

        if (Parser::status::success == get_content (filename, parser)) {
                xmlpp::Node *xmlnode = parser.get_document()->get_root_node();
                if (xmlnode->get_name() == "AVRdevice") {
                        desc_file = true;

                xmlnode = xmlnode->get_first_child("metadata");
                device_name = (dynamic_cast<const xmlpp::Element*>(xmlnode))->get_attribute_value("name","");
                }
        } else {
                device_name = "NONE";
                desc_file = false;
        }

        return desc_file;
}

Parser::status Parser::get_settings (xmlpp::Node *root_node, DeviceDescription &description)
{
        xmlpp::Node* xml_node;
        xmlpp::Element* tmp_element;
        xmlpp::Node::NodeList options;
        xmlpp::Node::NodeList entries;
        xmlpp::Node::NodeList::iterator option;
        xmlpp::Node::NodeList::iterator entry;

        DeviceDescription::FuseSetting *fuse_option;
        DeviceDescription::OptionEntry *enum_entry;
        list<DeviceDescription::OptionEntry> *enum_list;

        string value_str;

        // get number of fusebytes
        xml_node = root_node->get_first_child("settings");
        xml_node = xml_node->get_first_child("fusebytes");
        tmp_element = dynamic_cast<xmlpp::Element*>(xml_node);
        value_str = tmp_element->get_attribute_value("count", "");
        description.fusebytes_count = atoi(value_str.c_str());

        // get list of XML nodes with options
        xml_node = root_node->get_first_child("settings");
        options = xml_node->get_children("option");

        // loop through available options
        int enum_entry_counter;
        for (option = options.begin(); option != options.end(); ++option) {
                // get XML element with option
                tmp_element = dynamic_cast<xmlpp::Element*>(*option);
                // create option entry
                fuse_option = new DeviceDescription::FuseSetting;
                // define this entry
                fuse_option->fname = tmp_element->get_attribute_value("name", "");
                fuse_option->fenum = tmp_element->get_attribute_value("enum", "");
                fuse_option->fdesc = tmp_element->get_attribute_value("desc", "");
                value_str = tmp_element->get_attribute_value("offset", "");
                fuse_option->offset = atoi(value_str.c_str());
                if (fuse_option->offset == 512) {
                        fuse_option->fmask = 512;
                } else {
                        value_str = tmp_element->get_attribute_value("bitmask", "");
                        fuse_option->fmask = atoi(value_str.c_str());
                }

                // check if there is an enumerator
                if (fuse_option->fenum != "list") {
                        fuse_option->single_option = true;
                } else {
                        fuse_option->single_option = false;
                        // prepare list for this enum entries
                        enum_list = new list<DeviceDescription::OptionEntry>;
                        // get list of XML nodes with enum entries
                        entries = (*option)->get_children("entry");
                        // loop through available enum entries
                        enum_entry_counter = 0;
                        for (entry = entries.begin(); entry != entries.end(); ++entry) {
                                // get XML element with option
                                tmp_element = dynamic_cast<xmlpp::Element*>(*entry);
                                // create enum entry
                                enum_entry = new DeviceDescription::OptionEntry;
                                enum_entry->ename = tmp_element->get_attribute_value("txt", "");
                                value_str = tmp_element->get_attribute_value("val", "");
                                enum_entry->value = atoi(value_str.c_str());
                                // add this entry in enumerator list
                                enum_list->push_back(*enum_entry);
                                // delete this enum entry
                                delete enum_entry;
                                enum_entry_counter++;
                        }
                        // insert pseudo enum entry that mentions number of real entries
                        enum_entry = new DeviceDescription::OptionEntry;
                        enum_entry->ename = "";
                        enum_entry->value = enum_entry_counter;
                        enum_list->push_front(*enum_entry);
                        delete enum_entry;
                        // add this enumerator in name to enum mapping
                        (*description.option_lists)[fuse_option->fname] = enum_list;
                }

                // add this entry in list of settings
                description.fuse_settings->push_back(*fuse_option);
                // delete this entry
                delete fuse_option;
        }

        return Parser::status::success;
}


Parser::status Parser::get_warnings (xmlpp::Node *root_node, DeviceDescription &description)
{
        xmlpp::Node* xml_node;
        xmlpp::Element* tmp_element;
        xmlpp::Node::NodeList warnings;
        xmlpp::Node::NodeList::iterator wcase;
        DeviceDescription::FuseWarning *warning_entry;
        string value_str;

        // get list of XML nodes with warnings
        xml_node = root_node->get_first_child("warnings");
        warnings = xml_node->get_children("case");

        // loop through available warnings
        for (wcase = warnings.begin(); wcase != warnings.end(); ++wcase) {
                // get XML element with warning
                tmp_element = dynamic_cast<xmlpp::Element*>(*wcase);
                // create warning entry
                warning_entry = new DeviceDescription::FuseWarning;
                // get values for warning entry
                value_str = tmp_element->get_attribute_value("byte","");
                warning_entry->fbyte = atoi(value_str.c_str());
                value_str = tmp_element->get_attribute_value("mask","");
                warning_entry->fmask = atoi(value_str.c_str());
                value_str = tmp_element->get_attribute_value("result","");
                warning_entry->fresult = atoi(value_str.c_str());
                warning_entry->warning = tmp_element->get_attribute_value("message","");
                // insert warning in list
                description.warnings->push_back(*warning_entry);
                // delete this wanring entry
                delete warning_entry;
        }

        /*
        // debug print-out
        list<DeviceDescription::FuseWarning>::iterator iter;
        for (iter = description.warnings->begin(); iter != description.warnings->end(); iter++) {
                cout << endl;
                cout << "    byte: " << iter->fbyte << endl;
                cout << "    mask: " << iter->fmask << endl;
                cout << "   value: " << iter->fresult << endl;
                cout << "    text: " << iter->warning << endl;
        }
        */

        return Parser::status::success;
}


Parser::status Parser::get_default (xmlpp::Node *root_node, DeviceDescription &description)
{
        xmlpp::Node* xml_node;
        xmlpp::Element* tmp_element;

        // get XML element with default settings
        xml_node = root_node->get_first_child("defaults");
        tmp_element = dynamic_cast<xmlpp::Element*>(xml_node);

        // get default settings' values
        description.fusebytes_default[0] = stoi(tmp_element->get_attribute_value("lfuse", ""));
        description.fusebytes_default[1] = stoi(tmp_element->get_attribute_value("hfuse", ""));
        description.fusebytes_default[2] = stoi(tmp_element->get_attribute_value("efuse", ""));

        /*
        // debug print-out
        for (int i = 0; i < 3; i++)
                cout << "fuse byte " << i + 1 << " : " << hex << description.fusebytes_default[i] << endl;
        */

        return Parser::status::success;
}


Parser::status Parser::get_metadata (xmlpp::Node *root_node, DeviceDescription &description)
{
        xmlpp::Node* xml_node;
        xmlpp::Element* tmp_element;

        // get XML element with metadata
        xml_node = root_node->get_first_child("metadata");
        tmp_element = dynamic_cast<xmlpp::Element*>(xml_node);

        // get metadata values
        description.max_speed = tmp_element->get_attribute_value("speed", "");
        description.xml_version = tmp_element->get_attribute_value("xml_ver", "");
        description.flash_size = tmp_element->get_attribute_value("flash", "");
        description.sram_size = tmp_element->get_attribute_value("sram", "");
        description.eeprom_size = tmp_element->get_attribute_value("eeprom", "");

        // check if eeprom exists or not
        if (tmp_element->get_attribute_value("eeprom","") == "yes")
                description.eeprom_exists = true;
        else
                description.eeprom_exists = false;

        // get XML element with device signature
        xml_node = root_node->get_first_child("signature");
        tmp_element = dynamic_cast<xmlpp::Element*>(xml_node);

        // get signature string
        description.signature = tmp_element->get_attribute_value("value", "");

        /*
        // debug print-out
        cout << "speed     : " << description.max_speed << endl;
        cout << "flash     : " << description.flash_size << endl;
        cout << "sram      : " << description.sram_size << endl;
        cout << "eeprom    : " << description.eeprom_size << endl;
        cout << "signature : " << description.signature << endl;
        cout << "xml build : " << description.xml_version << endl;
        */

        return Parser::status::success;
}
