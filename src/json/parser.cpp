/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <json/parser.h>
#include <exceptions.h>

#include <stdlib.h>
#include <string.h>
#include <iostream>

using namespace falcon;

JsonVal::JsonVal (int const type)
  : _type (type)
  , _array (), _object (), _data ()
{
}

JsonVal::JsonVal (int const type,
                  char const* data, unsigned int const length)
  : _type (type)
  , _array (), _object (), _data (data, length)
{
}

JsonVal::~JsonVal ()
{
  switch (this->_type)
  {
  case JSON_OBJECT_BEGIN:
    {
      std::map<std::string, JsonVal*>::iterator it;
      std::map<std::string, JsonVal*>::iterator end;
      end = this->_object.end ();
      for (it = this->_object.begin (); it != end; ++it)
      {
        delete it->second;
      }
    }
    break;
  case JSON_ARRAY_BEGIN:
    {
      std::deque<JsonVal*>::iterator it;
      std::deque<JsonVal*>::iterator end;
      end = this->_array.end ();
      for (it = this->_array.begin (); it != end; ++it)
      {
        delete (*it);
      }
    }
    break;
  default:
    break;
  }
}

static void* tree_create_structure (int nesting __attribute__ ((unused)),
                                    int is_object)
{
  JsonVal* val = NULL;
  /* instead of defining a new enum type, we abuse the
   * meaning of the json enum type for array and object */
  if (is_object)
  {
    val = new JsonVal (JSON_OBJECT_BEGIN);
  }
  else
  {
    val = new JsonVal (JSON_ARRAY_BEGIN);
  }

  return val;
}

static void* tree_create_data (int type,
                               const char* data, uint32_t length)
{
  JsonVal* val = NULL;

  val = new JsonVal (type, data, length);

  return val;
}

static int tree_append (void* structure,
                        char* key, uint32_t key_length,
                        void* obj)
{
  JsonVal* parent = (JsonVal*)structure;

  if (key)
  {
    parent->_object.insert
      (
        std::pair<std::string, JsonVal*>
          (std::string (key, key_length), (JsonVal*)obj)
      );
  }
  else
  {
    parent->_array.push_back ((JsonVal*)obj);
  }

  return 0;
}

void JsonVal::print (unsigned const int indent, std::ostream& os) const
{
  std::string base ("  ");
  std::string prefix ("");

  for (unsigned int i = 0; i < indent; i++)
  {
    prefix += base;
  }

  switch (this->_type)
  {
  case JSON_OBJECT_BEGIN:
    {
      std::map<std::string, JsonVal*>::const_iterator it;
      std::map<std::string, JsonVal*>::const_iterator end;
      end = this->_object.cend ();
      os << prefix << "Object Begin (" << this->_object.size () << ")" << std::endl;
      for (it = this->_object.begin (); it != end; ++it)
      {
        os << prefix << base << "Key (" << it->first << ")" << std::endl;
        it->second->print (indent + 1, os);
      }
      os << prefix << "Object End" << std::endl;
    }
    break;
  case JSON_ARRAY_BEGIN:
    {
      std::deque<JsonVal*>::const_iterator it;
      std::deque<JsonVal*>::const_iterator end;
      end = this->_array.cend ();
      os << prefix << "Array Begin (" << this->_array.size () << ")" << std::endl;
      for (it = this->_array.cbegin (); it != end; ++it)
      {
        (*it)->print (indent + 1, os);
      }
      os << prefix << "Array End" << std::endl;
    }
    break;
  case JSON_FALSE:
    os << prefix << "constant: false" << std::endl;;
    break;
  case JSON_TRUE:
    os << prefix << "constant: true" << std::endl;;
    break;
  case JSON_NULL:
    os << prefix << "constant: null" << std::endl;;
    break;
  case JSON_INT:
    os << prefix << "integer: " << this->_data << std::endl;
    break;
  case JSON_STRING:
    os << prefix << "string: " << this->_data << std::endl;
    break;
  case JSON_FLOAT:
    os << prefix << "float: " << this->_data << std::endl;
    break;
  default:
    break;
  }
}

JsonVal const* JsonVal::getObject (std::string const& key) const
{
  JsonVal const* ret = NULL;
  switch (this->_type)
  {
  case JSON_OBJECT_BEGIN:
    {
      std::map<std::string, JsonVal*>::const_iterator it;
      std::map<std::string, JsonVal*>::const_iterator end;
      end = this->_object.cend ();
      for (it = this->_object.begin (); it != end && !ret; ++it)
      {
        if (it->first.compare (key) == 0)
        {
          ret = it->second;
        }
      }
    }
    break;
  case JSON_ARRAY_BEGIN:
    {
      std::deque<JsonVal*>::const_iterator it;
      std::deque<JsonVal*>::const_iterator end;
      end = this->_array.cend ();
      for (it = this->_array.cbegin (); it != end && !ret; ++it)
      {
        ret = (*it)->getObject (key);
      }
    }
    break;
  case JSON_FALSE:
  case JSON_TRUE:
  case JSON_NULL:
  case JSON_INT:
  case JSON_STRING:
  case JSON_FLOAT:
  default:
    break;
  }
  return ret;
}

std::ostream& operator<< (std::ostream& os, JsonVal const& obj)
{
  obj.print (0, os);
  return os;
}






JsonParser::JsonParser ()
{
  json_parser_dom_init (&this->_dom, tree_create_structure,
                        tree_create_data, tree_append);

  json_config config;
  memset (&config, 0, sizeof (config));
  config.max_nesting = 0;
  config.max_data = 0;
  config.allow_c_comments = 1;
  config.allow_yaml_comments = 0;
  json_parser_init (&this->_parser, &config,
                    json_parser_dom_callback, &this->_dom);
}

JsonParser::~JsonParser ()
{
  JsonVal* dom;

  json_parser_free (&this->_parser);
  dom = this->getDom ();
  if (dom)
  {
    delete dom;
  }
  json_parser_dom_free (&this->_dom);
}

void JsonParser::parse (unsigned int const line,
                        char const* s, unsigned int const size)
{
  int ret;
  unsigned int col = 0;

  ret = json_parser_string (&this->_parser, s, size, &col);

  switch (ret)
  {
  case 0:
  case 3:
    break;
  case 7:
    THROW_ERROR (EINVAL, "yaml-style comment in json not allowed");
    break;
  default:
    char msg[256];
    memset (msg, 0, sizeof (msg));
    snprintf (msg, sizeof (msg),
              "Error line (%u) caracter (%u) line (%s)", line, col, s);
    THROW_ERROR (EINVAL, msg);
    break;
  }
}

JsonVal* JsonParser::getDom (void)
{
  return (JsonVal*)this->_dom.root_structure;
}

void JsonParser::parse (unsigned int const line, std::string& s)
{
  this->parse (line, s.c_str (), s.size ());
}
