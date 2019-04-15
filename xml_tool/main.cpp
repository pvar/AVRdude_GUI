#include "main.h"

using namespace std;

int main(int argc, char* argv[])
{
        Translator avr_xml;

        avr_xml.process_files();

        return EXIT_SUCCESS;
}

Translator::~Translator () {}

Translator::Translator ()
{
        pid_t pid = getpid();
        gchar pid_str[20] = {0};
        sprintf(pid_str, "%d", pid);

        string path = "";
        string _link = "/proc/";
        _link.append( pid_str );
        _link.append( "/exe");

        char proc[512];
        int chars_read = readlink(_link.c_str(), proc, 512);
        if (chars_read != -1) {
                proc[chars_read] = 0;
                path = proc;
                string::size_type len = path.find_last_of("/");
                path = path.substr(0, len + 1);
        }
        this->exec_path = path;
}

void Translator::process_files (void)
{
        // prepare path to XML files...
        string xml_dir = ("input/");
        string whole_path = this->exec_path + xml_dir;
        string path_to_file, device_name;
        char *path_xml_files = new char[whole_path.size() + 1];
        int converted = 0;
        // !! if path uses multi-byte characters, result is NOT SAFE because of copy
        copy(whole_path.begin(), whole_path.end(), path_xml_files);
        path_xml_files[whole_path.size()] = '\0';

        // check files in directory (candidate description-files) one by one
        DIR *directory;
        struct dirent *entry;
        if ((directory = opendir ((const gchar *)path_xml_files)) != NULL) {
                while ((entry = readdir(directory)) != NULL) {
                        // get a file in directory
                        const string filename = entry->d_name;
                        // check for appropriate filename length
                        if (filename.size() < 4)
                                continue;
                        // check for expected filename-extension
                        if ((filename.compare(filename.size() - 3, 3, "xml") != 0) &&
                            (filename.compare(filename.size() - 3, 3, "XML") != 0))
                                continue;
                        // add full path to specific filename
                        path_to_file = whole_path + filename;
                        cout << "Converting: " << filename;
                        // attempt conversion to custom XML format
                        if (convert (path_to_file, filename)) {
                                converted++;
                                cout << "\tSUCCESS" << endl;
                        } else {
                                cout << "\tFAIL !!" << endl;
                        }
                }
                closedir (directory);
        }
        cout << endl << "Number of converted files: " << converted << endl;
}

bool Translator::convert (string filename, string filename_part)
{
        xmlpp::DomParser parser;

        // attempt to open file
        if (!openfile (filename, parser))
                return false;

        // get root node of file
        xmlpp::Node *root_node = parser.get_document()->get_root_node();

        // check if content has expected structure
        if (!is_dev_desc (root_node))
                return false;

        xml_filename = filename_part;

        // prepare data structures
        this->warnings = new list<FuseWarning>;
        this->fuse_settings = new list<FuseSetting>;
        this->option_lists = new map <string, list<OptionEntry>*>;
        this->fusebytes_default[0] = 255;
        this->fusebytes_default[1] = 255;
        this->fusebytes_default[2] = 255;

        // attempt to get specifications
        if (!get_specs (root_node))
                return false;

        // attempt to get fuse settings
        if (!get_settings (root_node))
                return false;

        // get default settings -- ignore if missing
        get_def_set (root_node);

        // get fuse warnings -- ignore if missing
        get_warnings (root_node);

        // assemble custom XML file
        if (!create_xml())
                return false;

        // delete data structures
        delete this->warnings;
        map<string, list<OptionEntry>*>::iterator iter = this->option_lists->begin();
        for (; iter!=this->option_lists->end(); ++iter)
                delete iter->second;
        delete this->option_lists;
        delete this->fuse_settings;

        return true;
}

bool Translator::openfile (std::string filename, xmlpp::DomParser &parser)
{
        // attempt to open specified file
        try {
                parser.parse_file(filename);
        } catch(const exception& ex) {
                cout << "Exception caught, trying to read file: " << ex.what() << endl;
                return false;
        }

        // check if content found
        if (parser)
                return true;
        else
                return false;
}

bool Translator::get_settings (xmlpp::Node *root_node)
{
        xmlpp::Node* xml_node = root_node;
        xmlpp::Node* tmp_node = nullptr;

        // go to V2 node
        xml_node = xml_node->get_first_child("V2");
        // if no such node, it must be a device without fuses...
        if (!xml_node)
                return false;
        // go to templates node
        xml_node = xml_node->get_first_child("templates");
        // if no such node, it must be a device without fuses...
        if (!xml_node)
                return false;
        // go to node with attribute: class = "FUSE"
        tmp_node = get_child_with_attr(xml_node,  "class", "FUSE");

        // if no such node, check if it's a device with NVM...
        if (!tmp_node) {
                // go to node with attribute: class = "NVM"
                tmp_node = get_child_with_attr(xml_node,  "class", "NVM");
                // if no such node, it must be a device without fuses...
                if (!tmp_node)
                        return false;
                // go to node with attribute: name = "NVM_FUSES"
                tmp_node = get_child_with_attr(tmp_node,  "name", "NVM_FUSES");
                // if no such node, there must be a problem...
                if (!tmp_node)
                        return false;
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
                FuseSetting *entry = new FuseSetting;
                entry->single_option = true;
                entry->fdesc = "FUSE REGISTER:  " + regname;
                entry->offset = 512; // special value that denotes a pseudo-entry
                // prepare list of settings from this node
                list<FuseSetting> *fuse_listing = new list<FuseSetting>;
                fuse_listing = get_fuse_list ((*node_iterator), offset);
                // add pseudo-setting in this list of settings
                fuse_listing->push_front(*entry);
                // add this settings list to the rest
                list<FuseSetting>::iterator pos = (fuse_settings)->begin();
                (fuse_settings)->splice(pos, *fuse_listing);
                delete entry;
        }
        //cout << "FUSE BYTES: " << fuse_byte_counter << endl;
        fusebytes_count = fuse_byte_counter;

        // go up to parent node
        xml_node = xml_node->get_parent();
        // loop through children (registers and enumerator nodes)
        children = xml_node->get_children();
        for (node_iterator = children.begin(); node_iterator != children.end(); ++node_iterator) {
                // skip irrelevant nodes (namely registers)
                if (((*node_iterator)->get_name()).compare("enumerator") != 0)
                        continue;
                // prepare list with this enumerator's entries
                list<OptionEntry> *enum_listing = new list<OptionEntry>;
                enum_listing = get_enum_list ((*node_iterator));
                // get enumerator name
                string enum_name = get_att_value ((*node_iterator), "name");
                // add new entry in map
                (*option_lists)[enum_name] = enum_listing;
        }

        return true;
}


bool Translator::get_warnings (xmlpp::Node *root_node)
{
        //cout << "PARSING WARNINGS..." << endl;

        // Fuse Warnings in XML file...
        // <FuseWarning>[fuse-byte-no],[AND-mask],[result],[warning text]</FuseWarning>

        string raw_warning;
        FuseWarning *warning_entry;

        xmlpp::Node* xml_node = root_node;

        // go to PROGRAMMING node
        xml_node = xml_node->get_first_child("PROGRAMMING");
        // if no such node, it must be a device without fuses...
        if (!xml_node)
                return false;

        // go to ISPInterface node
        xml_node = xml_node->get_first_child("ISPInterface");
        // if no such node, it must be a device without fuses...
        if (!xml_node)
                return false;

        // loop through children and get warnings
        xmlpp::Node::NodeList::iterator node_iterator;
        xmlpp::Node::NodeList children = xml_node->get_children();
        for (node_iterator = children.begin(); node_iterator != children.end(); ++node_iterator) {
                // get name of current sibling...
                const string node_name = (*node_iterator)->get_name();
                // if not a FuseWarning, proceed to next sibling...
                if (node_name.compare("FuseWarning") != 0)
                        continue;
                // get string (raw) representation of warning...
                raw_warning = get_txt_value ((*node_iterator));
                // assemble fuse warning structure
                warning_entry = new FuseWarning;
                stringstream(raw_warning.substr(0,1)) >> hex >> warning_entry->fbyte;
                stringstream(raw_warning.substr(2,4)) >> hex >> warning_entry->fmask;
                stringstream(raw_warning.substr(7,4)) >> hex >> warning_entry->fresult;
                warning_entry->warning = raw_warning.substr(21,1000);
                // inset warning in list
                warnings->push_back(*warning_entry);
                // delete temporary wanring entry
                delete warning_entry;
        }

        // debug print-out...
        //print_warnings(description);

        return true;
}


bool Translator::get_def_set (xmlpp::Node *root_node)
{
        xmlpp::Node* xml_node = root_node;
        xmlpp::Node* tmp_node = root_node;

        // go to FUSE node
        xml_node = xml_node->get_first_child("FUSE");
        // if no such node, it must be a device without fuses...
        if (!xml_node)
                return false;

        // go to LIST node
        xml_node = xml_node->get_first_child("LIST");
        // if no such node, it must be a device without fuses...
        if (!xml_node)
                return false;

        // get list of fuse-byte names
        string name_list = get_txt_value (xml_node);
        name_list = name_list.substr(1, name_list.length() - 2);

        // extract individual byte names
        int byte_count = 0;
        string byte_names[3];
        while (true) {
                // locate separator string
                size_t name_end = name_list.find(":");
                // extract name
                byte_names[byte_count] = name_list.substr(0, name_end);
                // check if it was the last name in the list
                if (name_end == string::npos)
                        break;
                // delete name and separator from list
                name_list.erase(0, name_end + 1);

                byte_count++;
        }

        xmlpp::Node::NodeList children;
        xmlpp::Node::NodeList::iterator node_iterator;
        string node_name, bit_val;
        int bit_order;

        // get value for each fuse-byte
        for (int i = 0; i < (byte_count + 1); i++) {
                // go up to FUSE node
                // (fuse-byte description nodes are children of FUSE node)
                xml_node = xml_node->get_parent();
                // go to <FUSE-BYTE-NAME> node
                tmp_node = xml_node->get_first_child(byte_names[i]);
                // if no such node, proceed to next fuse-byte...
                if (!tmp_node)
                        continue;
                else
                        xml_node = tmp_node;
                // loop through all children with name "FUSEx", where x in [0..7]
                children = xml_node->get_children();
                for (node_iterator = children.begin(); node_iterator != children.end(); ++node_iterator) {
                        // get name of current sibling...
                        node_name = (*node_iterator)->get_name();
                        // if does not contain "FUSE", proceed to next sibling...
                        if (node_name.find("FUSE") == string::npos)
                                continue;
                        // if more than 5 letters in name, proceed to next sibling...
                        if (node_name.length() > 5)
                                continue;
                        // erase constant part (FUSE)
                        node_name.erase(0,4);
                        // transform to integer
                        bit_order = atoi(node_name.c_str());
                        // get DEFAULT node
                        tmp_node = (*node_iterator)->get_first_child("DEFAULT");
                        // if not found, no default values mentioned in XML!
                        if (tmp_node == nullptr)
                                return false;
                        bit_val = get_txt_value (tmp_node);
                        // apply bit value on fuse-byte
                        if (bit_val.find("0") == string::npos)
                                fusebytes_default[i] |= 1 << bit_order;
                        else
                                fusebytes_default[i] &= ~(1 << bit_order);
                }
        }

        //print_defaults(description);

        return true;
}


bool Translator::get_specs (xmlpp::Node *root_node)
{
        xmlpp::Node* xml_node = root_node;
        xmlpp::Node* tmp_node = nullptr;
        string txtvalue;
        gfloat numvalue;

        // go to ADMIN node
        xml_node = xml_node->get_first_child("ADMIN");
        // go to SPEED node
        xml_node = xml_node->get_first_child("SPEED");
        // get node content
        txtvalue = get_txt_value(xml_node);
        txtvalue.resize(txtvalue.size() - 3);
        max_speed = txtvalue + " MHz";
        //cout << "speed: " << max_speed << endl;
        // go up to ADMIN node
        xml_node = xml_node->get_parent();
        // go to BUILD node
        xml_node = xml_node->get_first_child("BUILD");
        // get node content
        txtvalue = get_txt_value(xml_node);
        xml_version = txtvalue;
        //cout << "build: " << xml_version << endl;
        // go up to ADMIN node
        xml_node = xml_node->get_parent();
        // go to SIGNATURE node
        xml_node = xml_node->get_first_child("SIGNATURE");
        // get node content
        signature = get_signature_bytes(xml_node);
        //cout << "signature: \"" << signature << "\"" << endl;
        // go up to AVRPART node
        xml_node = xml_node->get_parent();
        xml_node = xml_node->get_parent();
        // go to MEMORY node
        xml_node = xml_node->get_first_child("MEMORY");
        // go to PROG_FLASH node
        xml_node = xml_node->get_first_child("PROG_FLASH");
        // get node content
        txtvalue = get_txt_value(xml_node);
        numvalue = ::atof(txtvalue.c_str()) / 1024;
        txtvalue = float_to_string(numvalue);
        flash_size = txtvalue + " KB";
        //cout << "flash: " << flash_size << endl;
        // go up to MEMORY node
        xml_node = xml_node->get_parent();
        // go to EEPROM node
        tmp_node = xml_node->get_first_child("EEPROM");
        // if no such node, it must be a device without EEPROM
        if (!tmp_node) {
                eeprom_exists = false;
                txtvalue = "0 Bytes";
                sram_size = txtvalue;
        } else {
                eeprom_exists = true;
                xml_node = tmp_node;
                // get node content
                txtvalue = get_txt_value(xml_node);
                numvalue = ::atof(txtvalue.c_str()) / 1024;
                // check for zero eeprom size
                if (numvalue == 0)
                        eeprom_exists = false;
                // express size in KBytes
                if (numvalue > 1)
                        txtvalue = float_to_string(numvalue) + " KB";
                else
                        txtvalue += " Bytes";
                eeprom_size = txtvalue;
                //cout << "eeprom: " << eeprom_size << endl;
                // go up to MEMORY node
                xml_node = xml_node->get_parent();
        }
        // go to INT_SRAM node
        tmp_node = xml_node->get_first_child("INT_SRAM");
        // if no such node, it must be a device without SRAM!
        if (!tmp_node) {
                txtvalue = "0 Bytes";
                sram_size = txtvalue;
        } else {
                xml_node = tmp_node;
                // go to SIZE node
                xml_node = xml_node->get_first_child("SIZE");
                // get node content
                txtvalue = get_txt_value(xml_node);
                numvalue = ::atof(txtvalue.c_str()) / 1024;
                if (numvalue > 1)
                        txtvalue = float_to_string(numvalue) + " KB";
                else
                        txtvalue += " Bytes";
                sram_size = txtvalue;
                //cout << "sram_size: " << eeprom_size << endl;
        }

        return true;
}

string Translator::get_signature_bytes (xmlpp::Node* signature_node)
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


string Translator::get_txt_value (xmlpp::Node* starting_node)
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


string Translator::get_att_value (xmlpp::Node* xml_node, string att_name)
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


string Translator::float_to_string (float number)
{
        string value(16, '\0');
        int chars_written = snprintf(&value[0], value.size(), "%.1f", number);
        value.resize(chars_written);

        return (string)value;
}

bool Translator::is_dev_desc (xmlpp::Node *root_node)
{
        xmlpp::Node *tmp_node;

        // check if a valid description file
        const string node_name = root_node->get_name();
        if (node_name.compare("AVRPART") != 0) {
                return false;
        } else {
                tmp_node = root_node->get_first_child("ADMIN");
                tmp_node = tmp_node->get_first_child("PART_NAME");
                device_name = get_txt_value(tmp_node);
                return true;
        }
}

list<Translator::OptionEntry>* Translator::get_enum_list (xmlpp::Node* xml_node)
{
        list<OptionEntry> *enum_listing = new list<OptionEntry>;

        xmlpp::Node::NodeList::iterator node_iterator;
        xmlpp::Element::AttributeList::const_iterator att_iterator;
        xmlpp::Node::NodeList children = xml_node->get_children();
        string tmp_string;
        uint max_value = 0;

        // loop through child-nodes (enum nodes)
        for (node_iterator = children.begin(); node_iterator != children.end(); ++node_iterator) {
                if (const xmlpp::Element* nodeElement = dynamic_cast<const xmlpp::Element*>(*node_iterator)) {
                        // loop through attribute-nodes
                        OptionEntry *entry = new OptionEntry;
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
        OptionEntry *entry = new OptionEntry;
        entry->ename = "PSEUDO_ENTRY";
        entry->value = max_value;
        enum_listing->push_front(*entry);
        delete entry;
        return enum_listing;
}


list<Translator::FuseSetting>* Translator::get_fuse_list (xmlpp::Node* xml_node, unsigned int offset)
{
        list<FuseSetting> *fuse_listing = new list<FuseSetting>;

        xmlpp::Node::NodeList::iterator node_iterator;
        xmlpp::Element::AttributeList::const_iterator att_iterator;

        xmlpp::Node::NodeList children = xml_node->get_children();
        string tmp_string;

        // loop through child-nodes (bitfield nodes)
        for (node_iterator = children.begin(); node_iterator != children.end(); ++node_iterator) {
                if (const xmlpp::Element* nodeElement = dynamic_cast<const xmlpp::Element*>(*node_iterator)) {
                        // loop through attribute-nodes
                        FuseSetting *entry = new FuseSetting;
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

xmlpp::Node* Translator::get_child_with_attr (xmlpp::Node* starting_node, string att_name, string att_value)
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

bool Translator::create_xml (void)
{
        xmlpp::Document newXML;
        xmlpp::Node *new_node;
        xmlpp::Element *new_element;
        xmlpp::Element *tmp_element;
        xmlpp::Element *tmp_element_ex;

        // create root node
        new_node = newXML.create_root_node ("AVRdevice", "dudegui:device.description.file", "");
        //((xmlpp::Element *)(new_node))->set_attribute("name", device_name, "");

        // add child node for metadata
        new_element = new_node->add_child ("metadata", "");
        // add attributes
        new_element->set_attribute("name", device_name, "");
        new_element->set_attribute("speed", max_speed, "");
        new_element->set_attribute("flash", flash_size, "");
        new_element->set_attribute("sram", sram_size, "");
        new_element->set_attribute("eeprom", eeprom_size, "");
        new_element->set_attribute("xml_name", xml_filename, "");
        new_element->set_attribute("xml_ver", xml_version, "");
        if (eeprom_exists)
                new_element->set_attribute("has_eeprom", "yes", "");
        else
                new_element->set_attribute("has_eeprom", "no", "");

        // add child node for signature
        new_element = new_node->add_child ("signature", "");
        new_element->set_attribute("value", signature, "");

        // add child node for default settings
        new_element = new_node->add_child ("defaults", "");
        new_element->set_attribute("lfuse", to_string(fusebytes_default[0]), "");
        new_element->set_attribute("hfuse", to_string(fusebytes_default[1]), "");
        new_element->set_attribute("efuse", to_string(fusebytes_default[2]), "");

        // add child node for settings description
        new_element = new_node->add_child ("settings", "");
        // add descendant for number of fuse bytes
        tmp_element = ((xmlpp::Node *)(new_element))->add_child ("fusebytes", "");
        tmp_element->set_attribute("count", to_string(fusebytes_count), "");
        // add a descendant for each option
        int i = 0;
        for (FuseSetting setting : *fuse_settings) {
                i++;
                tmp_element = ((xmlpp::Node *)(new_element))->add_child ("option" + to_string(i), "");
                tmp_element->set_attribute("bitmask", to_string(setting.fmask), "");
                tmp_element->set_attribute("offset", to_string(setting.offset), "");
                tmp_element->set_attribute("name", setting.fname, "");
                tmp_element->set_attribute("desc", setting.fdesc, "");
                if (setting.single_option) {
                        tmp_element->set_attribute("enum", "", "");
                } else {
                        tmp_element->set_attribute("enum", "list", "");
                        for (OptionEntry entry : *((*option_lists)[setting.fenum])) {
                                if (entry.ename.compare("PSEUDO_ENTRY") == 0)
                                        continue;
                                tmp_element_ex = ((xmlpp::Node *)(tmp_element))->add_child ("entry", "");
                                tmp_element_ex->set_attribute("val", to_string(entry.value), "");
                                tmp_element_ex->set_attribute("txt", entry.ename, "");
                        }
                }
        }
        // add child node for warnings description
        new_element = new_node->add_child ("warnings", "");
        i = 0;
        // add a descendant for each warning
        for (FuseWarning warn : *warnings) {
                i++;
                tmp_element = ((xmlpp::Node *)(new_element))->add_child ("case" + to_string(i), "");
                tmp_element->set_attribute("byte", to_string(warn.fbyte), "");
                tmp_element->set_attribute("mask", to_string(warn.fmask), "");
                tmp_element->set_attribute("result", to_string(warn.fresult), "");
                tmp_element->set_attribute("message", warn.warning, "");
        }

        string outfile = "./output/" + xml_filename;
        newXML.write_to_file_formatted(outfile);

        return true;
}
