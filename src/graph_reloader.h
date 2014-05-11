/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_GRAPH_RELOADER_H_
# define FALCON_GRAPH_RELOADER_H_

# include "graph.h"

namespace falcon {

class GraphReloader {
public:
  GraphReloader(Graph* original, Graph* newGraph);
  ~GraphReloader();

  Graph* getUpdatedGraph();

private:
  Graph* original_;
  Graph* new_;

};

}

#endif /* !FALCON_GRAPH_RELOADER_H_ */
