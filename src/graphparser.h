/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_GRAPHPARSER_H_
# define FALCON_GRAPHPARSER_H_

# include <memory>
# include "graph.h"
# include "json/parser.h"

namespace falcon {

/*!
 * @class GraphParser
 *
 * Graph builder: build an array of Rules from the given JSon entry
 *
 * Actually, there is no clear of the array of Rule.
 * if your build system has 2 configuration files linked together:
 *
 * @code
 * GraphParser gb;
 *
 * gb.processFile("file1");
 * gb.processFile("file2");
 * @code
 */
class GraphParser {
  public:
    GraphParser();

    std::unique_ptr<Graph> getGraph() const;

    /* If the input is a file */
    void processFile(std::string const& filepath);
    /* In the case we already have a Json object */
    void processJson(JsonVal const* rules);

  private:
    void checkNode(JsonVal const* json, NodeArray& nodeArray);
    /* Keep a hash of the Node in memory in order to avoid deplicated Node */
    NodeMap nodeMap_;
    NodeSet nodeSet_;

    GraphParser(const GraphParser& other) = delete;
    GraphParser& operator=(const GraphParser&) = delete;
};

} // namespace falcon

#endif /* !FALCON_GRAPHPARSER_H_ */
