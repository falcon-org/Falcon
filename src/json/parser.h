/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_JSON_PARSER_H_
# define FALCON_JSON_PARSER_H_

# include <map>
# include <deque>
# include <string>
# include <ostream>
# include <istream>

# include "json/json.h"

namespace falcon
{
  class JsonVal
  {
    public:
      JsonVal (int const type);
      JsonVal (int const type, char const* data, unsigned int const length);

      ~JsonVal ();

      void print (unsigned int const indent, std::ostream& os) const;
      JsonVal const* getObject (std::string const& key) const;

      int _type;

      std::deque<JsonVal*> _array;
      std::map<std::string, JsonVal*> _object;
      std::string _data;
  };

  class JsonParser
  {
    public:
      JsonParser ();
      ~JsonParser ();

      void parse (unsigned int const line,
                  char const* s, unsigned int const size);
      void parse (unsigned int const line, std::string& s);

      JsonVal* getDom (void);
    private:
      json_parser _parser;
      json_parser_dom _dom;
  };
}

std::ostream& operator<< (std::ostream& os, falcon::JsonVal const& obj);

#endif /* !FALCON_JSON_PARSER_H_ */
