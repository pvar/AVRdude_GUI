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


xmlpp::Node* Parser::get_child_with_attr (xmlpp::Node* starting_node, string att_name, string att_value)
{
        xmlpp::Node *target = nullptr;
        xmlpp::Node::NodeList::iterator node_iterator;
        xmlpp::Element::AttributeList::const_iterator att_iterator;
        xmlpp::Node::NodeList children = starting_node->get_children();

        // loop through child-nodes
        for (node_iterator = children.begin(); node_iterator != children.end(); ++node_iterator) {
                if (const xmlpp::Element* nodeElement = dynamic_cast<const xmlpp::Element*>(*node_iterator)) {
                        // loop through attribute-nodes
                        const xmlpp::Element::AttributeList& attributes = nodeElement->get_attributes();
                        for(att_iterator = attributes.begin(); att_iterator != attributes.end(); ++att_iterator) {
                                // break inner loop if target has been pinpointed
                                if ((*att_iterator)->get_name().compare(att_name) == 0)
                                        if ((*att_iterator)->get_value().compare(att_value) == 0) {
                                                //cout << (*att_iterator)->get_name() << " = " << (*att_iterator)->get_value() << endl;
                                                target = *node_iterator;
                                                break;
                                        }
                        }
                        // break outer loop if target has been defined
                        if (target != nullptr)
                                break;
                }
        }
        return target;
}


string Parser::get_txt_value (xmlpp::Node* starting_node)
{
        const xmlpp::TextNode *text = NULL;
        xmlpp::Node::NodeList::iterator iter;
        xmlpp::Node::NodeList children = starting_node->get_children();

        // loop through child-nodes
        for (iter = children.begin(); iter != children.end(); ++iter) {
                text = dynamic_cast<const xmlpp::TextNode*>(*iter);
                if (text)
                        break;
                else
                        continue;
        }
        return text->get_content();
}


string Parser::get_att_value (xmlpp::Node* xml_node, string att_name)
{
        xmlpp::Node::NodeList::iterator node_iterator;
        xmlpp::Element::AttributeList::const_iterator att_iterator;
        string data_string;

//xmlpp::Element* this_element = dynamic_cast<const xmlpp::Element*>(xml_node);
//cout << this_element->get_attribute_value("", "dudegui");

        if (const xmlpp::Element* nodeElement = dynamic_cast<const xmlpp::Element*>(xml_node)) {
                // loop through attribute-nodes
                const xmlpp::Element::AttributeList& attributes = nodeElement->get_attributes();
                for(att_iterator = attributes.begin(); att_iterator != attributes.end(); ++att_iterator) {
                        // break inner loop if target has been pinpointed
                        if ((*att_iterator)->get_name().compare(att_name) == 0) {
                                //cout << (*att_iterator)->get_name() << " = " << (*att_iterator)->get_value() << endl;
                                data_string = (*att_iterator)->get_value();
                                break;
                        }
                }
        }
        return data_string;
}


list<DeviceDescription::OptionEntry>* Parser::get_enum_list (xmlpp::Node* xml_node)
{
        list<DeviceDescription::OptionEntry> *enum_listing = new list<DeviceDescription::OptionEntry>;

        xmlpp::Node::NodeList::iterator node_iterator;
        xmlpp::Element::AttributeList::const_iterator att_iterator;
        xmlpp::Node::NodeList children = xml_node->get_children();
        string tmp_string;
        uint max_value = 0;

        // loop through child-nodes (enum nodes)
        for (node_iterator = children.begin(); node_iterator != children.end(); ++node_iterator) {
                if (const xmlpp::Element* nodeElement = dynamic_cast<const xmlpp::Element*>(*node_iterator)) {
                        // loop through attribute-nodes
                        DeviceDescription::OptionEntry *entry = new DeviceDescription::OptionEntry;
                        const xmlpp::Element::AttributeList& attributes = nodeElement->get_attributes();
                        for(att_iterator = attributes.begin(); att_iterator != attributes.end(); ++att_iterator) {
                                if ((*att_iterator)->get_name().compare("text") == 0) {
                                        entry->ename = (*att_iterator)->get_value();
                                        //cout << entry->ename << endl;
                                } else if ((*att_iterator)->get_name().compare("val") == 0) {
                                        string strvalue = ((*att_iterator)->get_value()).substr(2, 2);
                                        entry->value = stol (strvalue, nullptr, 16);
                                        if (max_value < entry->value)
                                                max_value = entry->value;
                                        //cout << entry->value << endl;
                                }
                        }
                        enum_listing->push_back(*entry);
                        delete entry;
                }
        }
        // insert pseudo enumerator-entry with maximum value
        DeviceDescription::OptionEntry *entry = new DeviceDescription::OptionEntry;
        entry->ename = "PSEUDO_ENTRY";
        entry->value = max_value;
        enum_listing->push_front(*entry);
        delete entry;
        return enum_listing;
}


list<DeviceDescription::FuseSetting>* Parser::get_fuse_list (xmlpp::Node* xml_node, unsigned int offset)
{
        list<DeviceDescription::FuseSetting> *fuse_listing = new list<DeviceDescription::FuseSetting>;

        xmlpp::Node::NodeList::iterator node_iterator;
        xmlpp::Element::AttributeList::const_iterator att_iterator;

        xmlpp::Node::NodeList children = xml_node->get_children();
        string tmp_string;

        // loop through child-nodes (bitfield nodes)
        for (node_iterator = children.begin(); node_iterator != children.end(); ++node_iterator) {
                if (const xmlpp::Element* nodeElement = dynamic_cast<const xmlpp::Element*>(*node_iterator)) {
                        // loop through attribute-nodes
                        DeviceDescription::FuseSetting *entry = new DeviceDescription::FuseSetting;
                        entry->fenum = ""; // must initialize to empty
                        entry->offset = offset;
                        const xmlpp::Element::AttributeList& attributes = nodeElement->get_attributes();
                        for(att_iterator = attributes.begin(); att_iterator != attributes.end(); ++att_iterator) {
                                if ((*att_iterator)->get_name().compare("name") == 0) {
                                        entry->fname = (*att_iterator)->get_value();
                                        //cout << entry->fname << endl;
                                } else if ((*att_iterator)->get_name().compare("text") == 0) {
                                        entry->fdesc = (*att_iterator)->get_value();
                                        //cout << entry->fdesc << endl;
                                } else if ((*att_iterator)->get_name().compare("enum") == 0) {
                                        entry->fenum = (*att_iterator)->get_value();
                                        //cout << entry->fenum << endl;
                                } else if ((*att_iterator)->get_name().compare("mask") == 0) {
                                        string strvalue = ((*att_iterator)->get_value()).substr(2, 2);
                                        entry->fmask = stol (strvalue, nullptr, 16);
                                        //cout << entry->fmask << endl;
                                }
                        }
                        if ((entry->fenum).size() == 0)
                                entry->single_option = true;
                        else
                                entry->single_option = false;
                        fuse_listing->push_back(*entry);
                        delete entry;
                }
        }
        return fuse_listing;
}

string Parser::float_to_string (float number)
{
        string value(16, '\0');
        int chars_written = snprintf(&value[0], value.size(), "%.1f", number);
        value.resize(chars_written);

        return (string)value;
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
        //cout << "PARSING FUSES..." << endl;

        xmlpp::Node* xml_node = root_node;
        xmlpp::Node* tmp_node = nullptr;

        // go to V2 node
        xml_node = xml_node->get_first_child("V2");
        // if no such node, it must be a device without fuses...
        if (!xml_node)
                return Parser::status::no_content;
        // go to templates node
        xml_node = xml_node->get_first_child("templates");
        // if no such node, it must be a device without fuses...
        if (!xml_node)
                return Parser::status::no_content;
        // go to node with attribute: class = "FUSE"
        tmp_node = get_child_with_attr(xml_node,  "class", "FUSE");

        // if no such node, check if it's a device with NVM...
        if (!tmp_node) {
                // go to node with attribute: class = "NVM"
                tmp_node = get_child_with_attr(xml_node,  "class", "NVM");
                // if no such node, it must be a device without fuses...
                if (!tmp_node)
                        return Parser::status::no_content;
                // go to node with attribute: name = "NVM_FUSES"
                tmp_node = get_child_with_attr(tmp_node,  "name", "NVM_FUSES");
                // if no such node, there must be a problem...
                if (!tmp_node)
                        return Parser::status::doc_error;
                // use default node pointer
                xml_node = tmp_node;
        } else {
                // go to registers node
                tmp_node = tmp_node->get_first_child("registers");
                // use default node pointer
                xml_node = tmp_node;
        }

        int fuse_byte_counter = 0;
        // loop through children (reg nodes)
        xmlpp::Node::NodeList children = xml_node->get_children();
        xmlpp::Node::NodeList::iterator node_iterator;
        for (node_iterator = children.begin(); node_iterator != children.end(); ++node_iterator) {
                // get offset value for this reg node
                string regoffset = get_att_value((*node_iterator), "offset");
                // if no offset attribute was found, proceed to next iteration
                if (regoffset.size() < 1)
                        continue;
                // increase fuse-byte counter
                fuse_byte_counter++;
                unsigned int offset = ::atoi((regoffset.substr(2, 2)).c_str());
                // get name for this reg node
                string regname = get_att_value((*node_iterator), "name");
                // prepare pseudo-setting with fuse-byte name
                DeviceDescription::FuseSetting *entry = new DeviceDescription::FuseSetting;
                entry->single_option = true;
                entry->fdesc = "FUSE REGISTER:  " + regname;
                entry->offset = 512; // special value that denotes a pseudo-entry
                // prepare list of settings from this node
                list<DeviceDescription::FuseSetting> *fuse_listing = new list<DeviceDescription::FuseSetting>;
                fuse_listing = get_fuse_list ((*node_iterator), offset);
                // add pseudo-setting in this list of settings
                fuse_listing->push_front(*entry);
                // add this settings list to the rest
                list<DeviceDescription::FuseSetting>::iterator pos = (description.fuse_settings)->begin();
                (description.fuse_settings)->splice(pos, *fuse_listing);
                delete entry;
        }
        //cout << "FUSE BYTES: " << fuse_byte_counter << endl;
        description.fusebytes_count = fuse_byte_counter;

        // debug print-out...
        //print_settings(description);

        // go up to parent node
        xml_node = xml_node->get_parent();
        // loop through children (registers and enumerator nodes)
        children = xml_node->get_children();
        for (node_iterator = children.begin(); node_iterator != children.end(); ++node_iterator) {
                // skip irrelevant nodes (namely registers)
                if (((*node_iterator)->get_name()).compare("enumerator") != 0)
                        continue;
                // prepare list with this enumerator's entries
                list<DeviceDescription::OptionEntry> *enum_listing = new list<DeviceDescription::OptionEntry>;
                enum_listing = get_enum_list ((*node_iterator));
                // get enumerator name
                string enum_name = get_att_value ((*node_iterator), "name");
                // add new entry in map
                (*description.option_lists)[enum_name] = enum_listing;
        }

        // debug print-out...
        //print_options(description);

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

        // get XML element with warnings
        xml_node = root_node->get_first_child("warnings");
        warnings = xml_node->get_children();

        for (wcase = warnings.begin(); wcase != warnings.end(); ++wcase) {
                // skip irrelevant children
                if ((*wcase)->get_name() == "text")
                        continue;
                // get element with current warning
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

string Parser::get_signature_bytes (xmlpp::Node* signature_node)
{
        const xmlpp::TextNode *text = NULL;
        xmlpp::Node::NodeList::iterator iter;
        xmlpp::Node::NodeList children = signature_node->get_children();
        string signatrure;

        signatrure = "0x";
        for (iter = children.begin(); iter != children.end(); ++iter) {
                text = dynamic_cast<const xmlpp::TextNode*>(*iter);
                if (!text)
                        signatrure += (get_txt_value((*iter))).substr(1, 2);
        }
        return signatrure;
}
