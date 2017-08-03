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

        get_specs (root_node, description);
        get_settings (root_node, description);
        get_warnings (root_node, description);
        get_default (root_node, description);

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

        // check if content has expected structure
        xmlpp::Node *root_node = parser.get_document()->get_root_node();
        if (is_valid (root_node))
                return status::success;
        else
                return status::doc_error;
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


list<DeviceDescription::FuseSetting>* Parser::get_fuse_list (xmlpp::Node* xml_node, uint offset)
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


void Parser::print_warnings (DeviceDescription &description)
{
        list<DeviceDescription::FuseWarning>::iterator iter;
        for (iter = description.warnings->begin(); iter != description.warnings->end(); iter++) {
                cout << endl;
                cout << "    byte: " << iter->fbyte << endl;
                cout << "    mask: " << iter->fmask << endl;
                cout << "   value: " << iter->fresult << endl;
                cout << "    text: " << iter->warning << endl;
        }
}


void Parser::print_defaults (DeviceDescription &description)
{
        for (int i = 0; i < 3; i++)
                cout << "fuse byte " << i + 1 << " : " << hex << description.fusebytes_default[i] << endl;
}


void Parser::print_settings (DeviceDescription &description)
{
        list<DeviceDescription::FuseSetting>::iterator iter;
        for (iter = (description.fuse_settings)->begin(); iter != (description.fuse_settings)->end(); ++iter) {
                cout << endl;
                cout << (*iter).fname << " ";
                cout << "mask[" << (*iter).fmask << "] ";
                cout << "offset[" << (*iter).offset << "] ";
                cout << "enum[" << (*iter).fenum << "] ";
                cout << (*iter).fdesc << endl;
        }
}


void Parser::print_options (DeviceDescription &description)
{
        map<string, list<DeviceDescription::OptionEntry>*>::iterator iter;
        for (iter = (description.option_lists)->begin(); iter != (description.option_lists)->end(); ++iter) {
                cout << iter->first << " => " << endl;
                list<DeviceDescription::OptionEntry>::iterator iter2;
                for (iter2 = (*iter->second).begin(); iter2 != (*iter->second).end(); ++iter2)
                                cout << (*iter2).ename << endl;
        }
}
