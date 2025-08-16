#include "statorNode.hpp"

StatorNodeType  sntFromString(std::string type) {
  if (type == "SNT_FACTORY_NODE")
    return (SNT_FACTORY_NODE);
  if (type == "SNT_RECIPE_NODE")
    return (SNT_RECIPE_NODE);
  if (type == "SNT_PART_NODE")
    return (SNT_PART_NODE);
  if (type == "SNT_IN_NODE")
    return (SNT_IN_NODE);
  if (type == "SNT_OUT_NODE")
    return (SNT_OUT_NODE);
  if (type == "SNT_BALANCE_NODE")
    return (SNT_BALANCE_NODE);
  return (SNT_NA);
}
