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
 * Graph builder: build an array of Rules from the given JSon entry.
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
    GraphParser(std::string const& filepath);

    std::unique_ptr<Graph> getGraph();

    /* If the input is a file */
    void processFile();
    /* In the case we already have a Json object */
    void processJson(JsonVal const* rules);

  private:
    void checkNode(JsonVal const* json, NodeArray& nodeArray);

    /**
     * Load the depfile of a rule.
     */
    void ruleLoadDepfile(Rule* rule);

    /**
     * Generate mandatory Nodes (node to monitor the graph file) */
    void generateMandatoryNodes();

    std::unique_ptr<Graph> graph_;

    /**
     * Graph file path */
    std::string const& graphFilePath_;

    GraphParser(const GraphParser& other) = delete;
    GraphParser& operator=(const GraphParser&) = delete;
};

} // namespace falcon

#endif /* !FALCON_GRAPHPARSER_H_ */
